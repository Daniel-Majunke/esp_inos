#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <esp_system.h>

const char* ap_ssid = "ESP_Setup";
const char* ap_password = "esp123456";
const int device_id = 31;
const char* user_id = "358c82ee-5c56-40a9-9e40-742f4e7b898a";

String ssid = "";
String password = ""; 
String topicString = "topic/b7a46f2bf14045a28cad1fd678bc084b"; 

WebServer server(80);
String htmlPage;

Preferences preferences;

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

void checkForUpdate() {
    Serial.println("Checking for OTA update...");
    HTTPClient http;

    http.begin("http://192.168.178.28:5000/api/Update/GetFirmware");
    http.addHeader("Content-Type", "application/json");
    String jsonPayload = "{\"userId\": \"" + String(user_id) + "\", \"deviceid\": \"" + device_id + "\"}";
    int httpCode = http.POST(jsonPayload);
    if (httpCode == 200) {  // HTTP OK
        int contentLength = http.getSize();
        Serial.println("Update available. Content length: " + String(contentLength));
        bool canBegin = Update.begin(contentLength);
        if (canBegin) {
            Serial.println("Begin OTA update...");
            WiFiClient& client = http.getStream();
            size_t written = Update.writeStream(client);
            if (written == contentLength) {
                Serial.println("OTA update success!");
            } else {
                Serial.println("OTA update failed. Written only " + String(written) + " / " + String(contentLength) + " bytes.");
                delay(5000);
                checkForUpdate();
            }
            if (Update.end()) {
                Serial.println("Update successfully completed.");
                if (Update.isFinished()) {
                    Serial.println("Rebooting...");
                    ESP.restart();
                } else {
                    Serial.println("Update not finished.");
                    delay(5000);
                    checkForUpdate();
                }
            } else {
                Serial.println("Error Occurred: " + String(Update.getError()));
                delay(5000);
                checkForUpdate();
            }
        } else {
            Serial.println("Not enough space to begin OTA.");
        }
    } else {
        Serial.println("Error on HTTP request. Code: " + String(httpCode));
        delay(5000);
        checkForUpdate();
    }
    http.end();
}

void handleRoot() {
  Serial.println("Serving HTML page on root request...");
  generateHtmlPage();
  server.send(200, "text/html", htmlPage);
}

void startAccessPoint() {
  ssid = "";
  password = ""; 
  Serial.println("Starting Access Point...");

  WiFi.mode(WIFI_AP); // <--- Access Point Modus aktivieren
  WiFi.softAP(ap_ssid, ap_password); // <--- AP starten

  delay(5000);
  
  server.on("/", handleRoot);
  server.on("/connect", handleConnect);
  server.begin();

  Serial.println("Access Point started with SSID: " + String(ap_ssid));
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void connectToWiFi(const String& ssid, const String& password) {
  Serial.println("Connecting to WiFi SSID: " + ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 10) {
    delay(1000);
    Serial.print(".");
    tries++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
  } else {
    Serial.println("Failed to connect to WiFi");
  }
}

void saveCredentials(const String& ssid, const String& password) {
  Serial.println("Saving WiFi credentials...");
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();

  preferences.begin("mqtt-config", false);
  preferences.putString("topics", topicString);
  preferences.end();
  esp_restart(); 
}

void handleConnect() {
  Serial.println("Handling WiFi connection request...");
  ssid = server.arg("ssid");
  password = server.arg("password");
  connectToWiFi(ssid, password);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected, saving credentials");
    saveCredentials(ssid, password);
    WiFi.softAPdisconnect(true);
  } else {
    Serial.println("Failed to connect to WiFi, restarting Access Point...");
    startAccessPoint();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
 
  delay(100);

  preferences.begin("wifi-config", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();

  if (ssid != "") {
    Serial.println("Found saved WiFi credentials, attempting to connect...");
    connectToWiFi(ssid, password);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi, fetching config and checking for updates...");
      checkForUpdate();
      return;
    }
  }

  Serial.println("No WiFi credentials found, starting Access Point...");
  startAccessPoint();
}

void loop() {
  server.handleClient();
}
