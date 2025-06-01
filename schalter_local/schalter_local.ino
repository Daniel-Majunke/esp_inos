#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>

// WLAN-Konfiguration
String ssid = "";      
String password = "";  
String topics = "";    
const char* ap_ssid = "ESP_Setup";
const char* ap_password = "esp123456";

// MQTT-Server-Konfiguration
const char* mqtt_server = "192.168.178.28";  // IP-Adresse des Servers
const int mqtt_port = 1883;                // Port des MQTT-Brokers

// Relais
int relayPin = 5;      // Pin, an dem das Relais angeschlossen ist
bool relayState = LOW; // Aktueller Zustand des Relais

WiFiClient espClient;
Preferences preferences;
PubSubClient client(espClient);
WebServer server(80);
String htmlPage;

bool apMode = false;   // Flag für Access Point Modus

// WLAN-Verbindung herstellen
void setup_wifi() {
  Serial.println();
  Serial.print("Verbinden mit ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startAttemptTime = millis();

  // Versuche 10 Sekunden lang, eine Verbindung herzustellen
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("");
    Serial.println("Verbindung fehlgeschlagen. Starte Access Point.");
    startAccessPoint();
  } else {
    Serial.println("");
    Serial.println("WLAN verbunden");
    Serial.println("IP-Adresse: ");
    Serial.println(WiFi.localIP());
  }
}
void startAccessPoint() {
  Serial.println("Starting Access Point...");
  WiFi.softAP(ap_ssid, ap_password);
  delay(1000);
  server.on("/", handleRoot);
  server.on("/connect", handleSave);
  server.begin();
  Serial.println("Access Point started with SSID: " + String(ap_ssid));
}


void generateHtmlPage() {
  int n = WiFi.scanNetworks();
  Serial.println("Generating HTML page with available networks...");
  htmlPage = "<!DOCTYPE HTML><html><head><title>WLAN-Konfiguration</title></head><body><h1>WLAN-Konfiguration</h1>";
  htmlPage += "<form action=\"/connect\" method=\"post\">";
  htmlPage += "SSID: <select name=\"ssid\">";
  for (int i = 0; i < n; ++i) {
    htmlPage += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
    Serial.println("Found network: " + WiFi.SSID(i));
  }
  htmlPage += "</select><br>";
  htmlPage += "Passwort: <input type=\"password\" name=\"password\"><br>";
  htmlPage += "<input type=\"submit\" value=\"Verbinden\">";
  htmlPage += "</form></body></html>";
}
void handleRoot() {
  Serial.println("Serving HTML page on root request...");
  generateHtmlPage();
  server.send(200, "text/html", htmlPage);
}

// Verarbeitung des Formulars und Speichern der Daten
void handleSave() {
  if (server.method() == HTTP_POST) {
    ssid = server.arg("ssid");
    password = server.arg("password");

    // WLAN-Daten speichern
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    String page = "<html><body><h1>Daten gespeichert.</h1>"
                  "<p>Das Gerät startet neu und verbindet sich mit dem WLAN.</p>"
                  "</body></html>";
    server.send(200, "text/html", page);

    delay(2000);
    ESP.restart(); // Neustart des ESP32
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

// MQTT-Nachrichten empfangen
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nachricht empfangen [");
  Serial.print(topic);
  Serial.print("] ");
  String payloadStr = "";
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadStr += (char)payload[i];
  }
  Serial.println();

  setRelayState(payloadStr); // Relaiszustand setzen

  // Bestätigung zurücksenden
  String newTopic = "response/" + String(topic);
  const char* payloadPublish = "success";
  boolean retained = false;
  client.publish(newTopic.c_str(), payloadPublish, retained);
  Serial.println("Antwort gesendet an: " + newTopic);
}

// Verbindung mit dem MQTT-Broker herstellen und Topic abonnieren
void reconnect() {
  while (!client.connected()) {
    Serial.print("Versuche Verbindung zum MQTT-Server aufzubauen...");
    if (client.connect(topics.c_str())) {
      Serial.println("Verbunden");
      Serial.println("Abonniere Topic: " + topics);

      client.subscribe(topics.c_str()); // Abonniere das gespeicherte Topic
    } else {
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println("; neuer Versuch in 5 Sekunden");
      delay(5000);
    }
  }
}

// Relaiszustand setzen
void setRelayState(String state) {
  if (state == "on") {
    digitalWrite(relayPin, HIGH); // Relais einschalten
    Serial.println("Relais eingeschaltet.");
  } else if (state == "off") {
    digitalWrite(relayPin, LOW);  // Relais ausschalten
    Serial.println("Relais ausgeschaltet.");
  } else {
    Serial.println("Ungültiger Befehl erhalten: " + state);
  }
}

void setup() {
  delay(3000);
  Serial.begin(115200);
  Serial.println("Start");

  pinMode(relayPin, OUTPUT);            // Setze den Relais-Pin als Ausgang
  digitalWrite(relayPin, relayState);

  // WLAN-Daten aus dem nichtflüchtigen Speicher laden
  preferences.begin("wifi-config", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();

  // MQTT-Topic aus dem nichtflüchtigen Speicher laden
  preferences.begin("mqtt-config", true);
  topics = preferences.getString("topics", "");
  preferences.end();

  Serial.println("Geladenes Topic: " + topics);

  setup_wifi();                             // WLAN-Verbindung herstellen
  client.setServer(mqtt_server, mqtt_port); // MQTT-Server und Port einstellen
  client.setCallback(callback);             // Callback für empfangene Nachrichten
}

void loop() {
  if (apMode) {
    server.handleClient(); // Webserver für Access Point
  } else {
    if (!client.connected()) {
      reconnect(); // Verbindung wiederherstellen, falls verloren
    }
    client.loop(); // MQTT-Client-Loop
    // Hier könntest du weitere Funktionen hinzufügen
  }
}

