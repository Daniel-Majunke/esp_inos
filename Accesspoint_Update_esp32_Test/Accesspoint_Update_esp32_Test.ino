#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_system.h>

Preferences preferences;

// MQTT-Server-Konfiguration
#define DEFAULT_MQTT_SERVER "192.168.178.28"
const int mqtt_port = 1883; // Port des MQTT-Brokers

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);

// HTML-Seite zur Eingabe der WLAN-Daten
const char* htmlPage = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>WLAN-Konfiguration</title>
</head>
<body>
  <h1>WLAN-Konfiguration</h1>
  <form action="/connect" method="post">
    SSID: <input type="text" name="ssid"><br>
    Passwort: <input type="password" name="password"><br>
    <input type="submit" value="Verbinden">
  </form>
</body>
</html>)rawliteral";

// Funktion, um die Root-Seite anzuzeigen
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Funktion zur Verarbeitung der WLAN-Verbindung
void handleConnect() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  // WLAN-Daten speichern
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();

  // Versuche, die WLAN-Verbindung herzustellen
  WiFi.begin(ssid.c_str(), password.c_str());

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    server.send(200, "text/html", "<h1>Erfolgreich verbunden! Gerät wird neugestartet...</h1>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(200, "text/html", "<h1>Verbindung fehlgeschlagen. Bitte versuchen Sie es erneut.</h1>");
  }
}

// WLAN-Verbindung herstellen
void setup_wifi(const char* ssid, const char* password) {
  delay(10);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Verbinden mit ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WLAN verbunden");
  Serial.println("IP-Adresse: ");
  Serial.println(WiFi.localIP());
}

// Konfiguration laden
void load_configuration() {
  HTTPClient http;
  http.begin("http://your-server-address/api/Config/GetConfig");

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    // JSON-Daten parsen
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("Fehler beim Parsen der Konfiguration: ");
      Serial.println(error.c_str());
      return;
    }

    const char* mqtt_server = doc["mqtt_server"];
    int mqtt_port = doc["mqtt_port"];

    // MQTT-Server einstellen
    client.setServer(mqtt_server, mqtt_port);
  } else {
    Serial.println("Fehler beim Abrufen der Konfiguration.");
  }

  http.end();
}

// MQTT-Nachricht empfangen
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nachricht empfangen [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  String newTopic = "response/" + String(topic);
  const char* payloadPublish = "success";
  boolean retained = false;
  client.publish(newTopic.c_str(), payloadPublish, retained);
  Serial.println(newTopic);
  Serial.println(String(payloadPublish));
}

// Verbindung mit dem MQTT-Broker herstellen und Topic abonnieren
void reconnect() {
  while (!client.connected()) {
    Serial.print("Versuche Verbindung zum MQTT-Server aufzubauen...");
    if (client.connect("ESP32Client")) {
      Serial.println("Verbunden");
      client.subscribe("userId/4bdafced3d0940248460a61a4fb28268"); // Beispiel-Topic abonnieren
    } else {
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println("; neuer Versuch in 5 Sekunden");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // WLAN-Daten löschen, wenn es sich um einen Flash-Reset handelt
  if (esp_reset_reason() == ESP_RST_DEEPSLEEP || esp_reset_reason() == ESP_RST_SW) {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
  }

  preferences.begin("wifi", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  if (ssid == "" || password == "") {
    // Starte Access Point zur Konfiguration
    Serial.println("Starte Access Point...");
    WiFi.softAP("ESP32_Setup", "esp123456");
    Serial.print("Access Point gestartet. IP-Adresse: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/connect", handleConnect);
    server.begin();
    Serial.println("Webserver gestartet.");
  } else {
    // Verwende gespeicherte WLAN-Daten
    setup_wifi(ssid.c_str(), password.c_str());
    load_configuration();                    // Konfiguration vom Server laden
    client.setCallback(callback);             // Callback für empfangene Nachrichten
    reconnect();                              // Initiale Verbindung zum MQTT-Server herstellen
  }
}

void loop() {
  if (WiFi.getMode() == WIFI_MODE_AP) {
    server.handleClient();
  } else {
    if (!client.connected()) {
      reconnect(); // Verbindung wiederherstellen, falls verloren
    }
    client.loop(); // MQTT-Client-Loop
  }
  delay(10); // Vermeidung hoher CPU-Auslastung
}
