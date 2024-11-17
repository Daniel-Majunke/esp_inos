#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

// Access Point Einstellungen
const char* ap_ssid = "ESP32_Setup";
const char* ap_password = "esp1";

// Server URL für Firmware-Update (lokal)
const char* update_url = "http://localhost:5000/api/Code/GetCode";

// GUID zur Identifikation des Geräts
const char* device_guid = "deine-einzigartige-guid-hier";

// Erstelle einen Webserver auf Port 80
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
  // Hole SSID und Passwort aus dem Formular
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  // Versuche, die WLAN-Verbindung herzustellen
  WiFi.begin(ssid.c_str(), password.c_str());

  // Warte auf die Verbindung
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  // Wenn die Verbindung erfolgreich ist, zeige eine Erfolgsmeldung und starte neu
  if (WiFi.status() == WL_CONNECTED) {
    server.send(200, "text/html", "<h1>Erfolgreich verbunden! Gerät wird neugestartet...</h1>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(200, "text/html", "<h1>Verbindung fehlgeschlagen. Bitte versuchen Sie es erneut.</h1>");
  }
}

void setup() {
  Serial.begin(115200);

  // Prüfe, ob WLAN-Daten bereits vorhanden sind und eine Verbindung hergestellt werden kann
  WiFi.begin();
  if (WiFi.status() != WL_CONNECTED) {
    // Starte den ESP als Access Point, wenn keine Verbindung möglich ist
    Serial.println("Starte Access Point...");
    
    // Trenne eventuell bestehende AP-Verbindungen und entferne alte Konfiguration
    WiFi.softAPdisconnect(true);
    delay(100);

    // Starte den AP mit neuen Einstellungen
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("Access Point gestartet. IP-Adresse: ");
    Serial.println(WiFi.softAPIP());

    // Starte den Webserver
    server.on("/", handleRoot);
    server.on("/connect", handleConnect);
    server.begin();
    Serial.println("Webserver gestartet.");
  } else {
    // Wenn die Verbindung erfolgreich ist, starte den Update-Prozess
    Serial.println("WLAN verbunden. Starte Update-Prozess...");
    checkForUpdate();
  }
}

void checkForUpdate() {
  // Erstelle einen HTTP-Client für das Firmware-Update
  HTTPClient http;

  // Beginne die Verbindung zur Update-URL
  http.begin(update_url);

  // Füge die GUID als zusätzliche Header-Information hinzu
  http.addHeader("Device-GUID", device_guid);

  // Beginne das Firmware-Update direkt von der angegebenen URL
  t_httpUpdate_return ret = httpUpdate.update(http, update_url);

  // Update-Ergebnis überprüfen
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Update fehlgeschlagen. Fehler (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("Keine Updates verfügbar.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("Update erfolgreich! Gerät startet neu...");
      // ESP32 wird automatisch nach erfolgreichem Update neu gestartet
      break;
  }

  // Beende HTTP-Client
  http.end();
}

void loop() {
  // Bearbeite Webserver-Anfragen im Access Point-Modus
  if (WiFi.getMode() == WIFI_AP) {
    server.handleClient();
    Serial.println("Webserver bearbeitet Anfragen...");
  }
}
