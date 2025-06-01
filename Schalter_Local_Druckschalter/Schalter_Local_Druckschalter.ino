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

const int buttonPin = 4; // Wähle hier einen freien GPIO-Pin für deinen Schalter (z.B. GPIO 13)

// Zustand des Schalters
bool lastButtonState = HIGH; 
unsigned long buttonPressTime = 0; 
const long longPressDuration = 3000; 


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
    Serial.println("Verbindung fehlgeschlagen");
    // hier wird der Accesspoint nicht mehr gestartet
    
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
  pinMode(buttonPin, INPUT_PULLUP);

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

void stopAccessPointAndRestart() {
  Serial.println("Beende Access Point und starte neu...");
  server.stop();
  WiFi.softAPdisconnect(true); // Trenne den SoftAP komplett
  apMode = false; // Setze das AP-Modus-Flag zurück
  delay(1000);
  ESP.restart(); // Neustart des ESP32, um in den normalen Modus zu gehen
}

void loop() {
  // Aktuellen Zustand des Tasters lesen
  int currentButtonState = digitalRead(buttonPin);

  // Tasterentprellung und Erkennung langes Drücken
  if (currentButtonState != lastButtonState) {
    if (currentButtonState == LOW) { // Taster wurde gerade gedrückt
      buttonPressTime = millis(); // Zeitpunkt des Drückens merken
      Serial.println("Taster gedrückt.");
    } else { // Taster wurde gerade losgelassen
      long duration = millis() - buttonPressTime;
      Serial.print("Taster losgelassen. Dauer: ");
      Serial.print(duration);
      Serial.println(" ms");

      if (duration >= longPressDuration) { // Wenn der Taster lange genug gedrückt wurde
        Serial.println("Langes Drücken erkannt!");
        if (apMode) {
          // Wenn bereits im AP-Modus, beende ihn und starte neu (um in den WLAN-Modus zu wechseln)
          stopAccessPointAndRestart();
        } else {
          // Wenn nicht im AP-Modus, starte ihn
          startAccessPoint();
        }
      }
    }
    lastButtonState = currentButtonState; // Letzten Zustand aktualisieren
  }

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


/*
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>


// WLAN-Konfiguration
String ssid = "";
String password = "";
String topics = "";
const char* ap_ssid = "ESP_Setup";
const char* ap_password = "esp123456";

// MQTT-Server-Konfiguration
const char* mqtt_server = "192.168.178.28";  // Domain des MQTT-Servers
const int mqtt_port = 1883;                     // Korrekter Port des MQTT-Brokers (z.B. für MQTT über SSL)

// Relais
int relayPin = 5;       // Pin, an dem das Relais angeschlossen ist
bool relayState = LOW; // Aktueller Zustand des Relais

// Schalter Pin
const int buttonPin = 4; // Wähle hier einen freien GPIO-Pin für deinen Schalter (z.B. GPIO 13)

// Zustand des Schalters
bool lastButtonState = HIGH; 
unsigned long buttonPressTime = 0; 
const long longPressDuration = 3000; 

WiFiClientSecure espClient;   
Preferences preferences;
PubSubClient client(espClient);
WebServer server(80);
String htmlPage;

bool apMode = false;  // Flag für Access Point Modus

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
    Serial.println("WLAN-Verbindung fehlgeschlagen. Bleibe im Betriebsmodus oder starte AP manuell.");
    // HINWEIS: Hier wird der AP NICHT mehr automatisch gestartet
  } else {
    Serial.println("");
    Serial.println("WLAN verbunden");
    Serial.println("IP-Adresse: ");
    Serial.println(WiFi.localIP());
  }
}

void startAccessPoint() {
  if(apMode) return; // Schon im AP Modus, nichts tun

  Serial.println("Starte Access Point...");
  WiFi.softAP(ap_ssid, ap_password);
  delay(1000);
  server.on("/", handleRoot);
  server.on("/connect", handleSave);
  server.begin();
  apMode = true; // Setze das AP-Modus-Flag
  Serial.println("Access Point gestartet mit SSID: " + String(ap_ssid));
}

void stopAccessPointAndRestart() {
  Serial.println("Beende Access Point und starte neu...");
  server.stop();
  WiFi.softAPdisconnect(true); // Trenne den SoftAP komplett
  apMode = false; // Setze das AP-Modus-Flag zurück
  delay(1000);
  ESP.restart(); // Neustart des ESP32, um in den normalen Modus zu gehen
}


void generateHtmlPage() {
  int n = WiFi.scanNetworks();
  Serial.println("Generiere HTML-Seite mit verfügbaren Netzwerken...");
  htmlPage = "<!DOCTYPE HTML><html><head><title>WLAN-Konfiguration</title></head><body><h1>WLAN-Konfiguration</h1>";
  htmlPage += "<form action=\"/connect\" method=\"post\">";
  htmlPage += "SSID: <select name=\"ssid\">";
  for (int i = 0; i < n; ++i) {
    htmlPage += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
    Serial.println("Netzwerk gefunden: " + WiFi.SSID(i));
  }
  htmlPage += "</select><br>";
  htmlPage += "Passwort: <input type=\"password\" name=\"password\"><br>";
  htmlPage += "MQTT Topic (optional): <input type=\"text\" name=\"topics\" value=\"" + topics + "\"><br>"; // MQTT Topic hinzufügen
  htmlPage += "<input type=\"submit\" value=\"Verbinden\">";
  htmlPage += "</form></body></html>";
}
void handleRoot() {
  Serial.println("Sende HTML-Seite bei Root-Anfrage...");
  generateHtmlPage();
  server.send(200, "text/html", htmlPage);
}


// Verarbeitung des Formulars und Speichern der Daten
void handleSave() {
  if (server.method() == HTTP_POST) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    topics = server.arg("topics"); // MQTT Topic aus dem Formular lesen

    // WLAN-Daten speichern
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    // MQTT-Topic speichern
    preferences.begin("mqtt-config", false);
    preferences.putString("topics", topics);
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
  Serial.println(state + "- state");
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

  pinMode(relayPin, OUTPUT);   // Setze den Relais-Pin als Ausgang    
  digitalWrite(relayPin, relayState);

  // Konfiguriere den Schalter-Pin als Eingang mit Pull-up Widerstand
  pinMode(buttonPin, INPUT_PULLUP);

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

  // Initialer Versuch, sich mit dem WLAN zu verbinden. AP startet NICHT automatisch bei Fehler.
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port); // MQTT-Server und Port einstellen
  client.setCallback(callback);             // Callback für empfangene Nachrichten
}

void loop() {
  // Aktuellen Zustand des Tasters lesen
  int currentButtonState = digitalRead(buttonPin);

  // Tasterentprellung und Erkennung langes Drücken
  if (currentButtonState != lastButtonState) {
    if (currentButtonState == LOW) { // Taster wurde gerade gedrückt
      buttonPressTime = millis(); // Zeitpunkt des Drückens merken
      Serial.println("Taster gedrückt.");
    } else { // Taster wurde gerade losgelassen
      long duration = millis() - buttonPressTime;
      Serial.print("Taster losgelassen. Dauer: ");
      Serial.print(duration);
      Serial.println(" ms");

      if (duration >= longPressDuration) { // Wenn der Taster lange genug gedrückt wurde
        Serial.println("Langes Drücken erkannt!");
        if (apMode) {
          // Wenn bereits im AP-Modus, beende ihn und starte neu (um in den WLAN-Modus zu wechseln)
          stopAccessPointAndRestart();
        } else {
          // Wenn nicht im AP-Modus, starte ihn
          startAccessPoint();
        }
      }
    }
    lastButtonState = currentButtonState; // Letzten Zustand aktualisieren
  }


  if (apMode) {
    server.handleClient(); // Webserver für Access Point
  } else {
    // Nur MQTT- und andere Aufgaben, wenn NICHT im AP-Modus
    if (!client.connected()) {
      reconnect(); // Verbindung wiederherstellen, falls verloren
    }
    client.loop(); // MQTT-Client-Loop
    // Hier könntest du weitere Funktionen hinzufügen
  }
} */
