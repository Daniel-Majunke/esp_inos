#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>


// WLAN-Konfiguration
String ssid = "";      // Dein WLAN-Name
String password = "";  // Dein WLAN-Passwort
String topics = "";    // Dein MQTT-Topic

// MQTT-Server-Konfiguration
const char* mqtt_server = "autarkes-leben.eu";  // Domain des MQTT-Servers
const int mqtt_port = 8883;                     // Korrekter Port des MQTT-Brokers (z.B. für MQTT über SSL)

// Relais
int relayPin = 5;      // Pin, an dem das Relais angeschlossen ist
bool relayState = LOW; // Aktueller Zustand des Relais

WiFiClientSecure espClient;   // Verwende WiFiClientSecure für SSL-Verbindungen
Preferences preferences;
PubSubClient client(espClient);
WebServer server(80);

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

// Access Point starten
void startAccessPoint() {
  apMode = true;
  WiFi.softAP("ESP32_Setup_AP");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point gestartet. IP-Adresse: ");
  Serial.println(IP);

  // Webserver-Endpunkte definieren
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
  Serial.println("Webserver gestartet.");
}

// Root-Seite anzeigen (Formular für WLAN-Daten)
void handleRoot() {
  String page = "<html><body><h1>WLAN konfigurieren</h1>"
                "<form action=\"/save\" method=\"post\">"
                "SSID:<br><input type=\"text\" name=\"ssid\"><br>"
                "Passwort:<br><input type=\"password\" name=\"password\"><br>"
                "<input type=\"submit\" value=\"Speichern\">"
                "</form></body></html>";
  server.send(200, "text/html", page);
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
    if (client.connect("ESP32Client")) {
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

  // SSL-Zertifikatsüberprüfung deaktivieren (Achtung: Sicherheitsrisiko)
  espClient.setInsecure();

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
