#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <esp_system.h>

const char* ap_ssid = "ESP_Setup";
const char* ap_password = "esp123456";
const char* register_code = "uT8j@zph3#";
bool configRetrieved = false;
String ssid = "";
String password = "";
String topicString = "";
String macAddress = "";

WebServer server(80);

String htmlPage;

Preferences preferences;

void generateHtmlPage() {
  int n = WiFi.scanNetworks();
  Serial.println("Generiere HTML-Seite mit verfügbaren Netzwerken...");
  htmlPage = "<!DOCTYPE HTML><html><head><title>WLAN-Konfiguration</title></head><body><h1>WLAN-Konfiguration</h1>";
  htmlPage += "<form action=\"/connect\" method=\"post\">";
  htmlPage += "SSID: <select name=\"ssid\">";
  for (int i = 0; i < n; ++i) {
    htmlPage += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
    Serial.println("Gefundenes Netzwerk: " + WiFi.SSID(i));
  }
  htmlPage += "</select><br>";
  htmlPage += "Passwort: <input type=\"password\" name=\"password\"><br>";
  htmlPage += "<input type=\"submit\" value=\"Verbinden\">";
  htmlPage += "</form></body></html>";
}

void checkForUpdate() {
    Serial.println("Prüfe auf OTA-Updates...");
    HTTPClient http;

    http.setInsecure();  // Akzeptiere alle SSL-Zertifikate
    http.begin("https://autarkes-leben.eu/api/Update/GetFirmware");
    http.addHeader("Content-Type", "application/json");
    String jsonPayload = "{\"registerCode\": \"" + String(register_code) + "\", \"macAddress\": \"" + macAddress + "\"}";
    int httpCode = http.POST(jsonPayload);
    if (httpCode == 200) {  // HTTP OK
        int contentLength = http.getSize();
        Serial.println("Update verfügbar. Inhaltlänge: " + String(contentLength));
        bool canBegin = Update.begin(contentLength);
        if (canBegin) {
            Serial.println("Beginne OTA-Update...");
            WiFiClient& client = http.getStream();
            size_t written = Update.writeStream(client);
            if (written == contentLength) {
                Serial.println("OTA-Update erfolgreich!");
            } else {
                Serial.println("OTA-Update fehlgeschlagen. Nur " + String(written) + " von " + String(contentLength) + " Bytes geschrieben.");
                delay(5000);
                checkForUpdate();
            }
            if (Update.end()) {
                Serial.println("Update erfolgreich abgeschlossen.");
                if (Update.isFinished()) {
                    Serial.println("Neustart...");
                    ESP.restart();
                } else {
                    Serial.println("Update nicht abgeschlossen.");
                    delay(5000);
                    checkForUpdate();
                }
            } else {
                Serial.println("Fehler aufgetreten: " + String(Update.getError()));
                delay(5000);
                checkForUpdate();
            }
        } else {
            Serial.println("Nicht genug Speicherplatz für OTA.");
        }
    } else {
        Serial.println("Fehler bei der HTTP-Anfrage. Code: " + String(httpCode));
        delay(5000);
        checkForUpdate();
    }
    http.end();
}

void handleRoot() {
  Serial.println("Sende HTML-Seite auf Root-Anfrage...");
  generateHtmlPage();
  server.send(200, "text/html", htmlPage);
}

void startAccessPoint() {
  Serial.println("Starte Access Point...");
  WiFi.softAP(ap_ssid, ap_password);
  delay(1000);
  server.on("/", handleRoot);
  server.on("/connect", handleConnect);
  server.begin();
  Serial.println("Access Point gestartet mit SSID: " + String(ap_ssid));
}

void connectToWiFi(const String& ssid, const String& password) {
  Serial.println("Verbinde mit WiFi SSID: " + ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 10) {
    delay(1000);
    Serial.print(".");
    tries++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Mit WiFi verbunden");
  } else {
    Serial.println("Verbindung mit WiFi fehlgeschlagen");
  }
}

void saveWiFiCredentials(const String& ssid, const String& password) {
  Serial.println("Speichere WiFi-Zugangsdaten...");
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}

bool saveFirstTopic(String response) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("JSON-Deserialisierung fehlgeschlagen: ");
    Serial.println(error.c_str());
    return false;
  }
  const char* firstTopic = doc["topics"][0];
  if (firstTopic != nullptr) {
    topicString = String(firstTopic);
    bool configRetrieved = true;
    preferences.begin("mqtt-config", false);
    preferences.putString("topics", topicString);
    preferences.putBool("getConfig", configRetrieved);
    preferences.end();
    Serial.println("Erstes Topic gespeichert: " + topicString);
  } else {
    Serial.println("Kein Topic in der Konfiguration gefunden.");
    return false;
  }
  return true;
}

void fetchConfig() {
  Serial.println("Hole Konfiguration vom Server...");
  while (!configRetrieved) {
    HTTPClient http;
    http.setInsecure();  // Akzeptiere alle SSL-Zertifikate
    http.begin("https://autarkes-leben.eu/api/Update/GetConfig");
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"registerCode\": \"" + String(register_code) + "\", \"macAddress\": \"" + macAddress + "\"}";
    Serial.println("Sende Payload: " + jsonPayload);
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode == 200) {
      String response = http.getString();
      if (response.length() > 0) {
        if (saveFirstTopic(response))
          esp_restart();
      }
    } else {
      Serial.println("Fehler beim Abrufen der Konfiguration. HTTP-Code: " + String(httpResponseCode));
    }
    http.end();
    delay(2000);
  }
}

void handleConnect() {
  Serial.println("Bearbeite WiFi-Verbindungsanfrage...");
  ssid = server.arg("ssid");
  password = server.arg("password");
  connectToWiFi(ssid, password);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi verbunden, speichere Zugangsdaten und hole Konfiguration...");
    saveWiFiCredentials(ssid, password);
    WiFi.softAPdisconnect(true);
    fetchConfig();
  } else {
    Serial.println("Verbindung mit WiFi fehlgeschlagen, starte Access Point neu...");
    startAccessPoint();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starte Setup...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100);

  preferences.begin("wifi-config", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();

  preferences.begin("mqtt-config", true);
  configRetrieved = preferences.getBool("getConfig", false);
  preferences.end();
  macAddress = WiFi.macAddress();
  Serial.println("MAC-Adresse: " + macAddress);
  if (ssid != "" && password != "") {
    Serial.println("Gespeicherte WiFi-Zugangsdaten gefunden, versuche zu verbinden...");
    connectToWiFi(ssid, password);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Mit WiFi verbunden, hole Konfiguration und prüfe auf Updates...");
      fetchConfig();
      checkForUpdate();
      return;
    }
  }

  Serial.println("Keine WiFi-Zugangsdaten gefunden, starte Access Point...");
  startAccessPoint();
  configRetrieved = false;
  preferences.begin("mqtt-config", false);
  preferences.putBool("getConfig", configRetrieved);
  preferences.end();
}

void loop() {
  server.handleClient();
}
