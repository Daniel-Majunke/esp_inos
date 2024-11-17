#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>

WebServer server(80);
Preferences preferences;

const char* ap_ssid = "ESP32_Setup";
const char* ap_password = "esp123456";
const char* register_url = "http://192.168.178.28:5000/api/EspRegistration/RecivedRegisterCode";

bool isWifiConnected = false;  // Zustand zur Steuerung der Anzeige

// HTML-Seite zur Eingabe der WLAN-Daten und optionaler Fehlermeldung
String htmlPageWifiConfig(String message = "") {
  return R"rawliteral(
    <!DOCTYPE HTML><html>
    <head>
      <title>WLAN-Konfiguration</title>
    </head>
    <body>
      <h1>WLAN-Konfiguration</h1>
      )rawliteral" + message + R"rawliteral(
      <form action="/" method="post">
        SSID: <input type="text" name="ssid"><br>
        Passwort: <input type="password" name="password"><br>
        <input type="submit" value="Verbinden">
      </form>
    </body>
    </html>
  )rawliteral";
}

// HTML-Seite für die Registrierung nach erfolgreicher WLAN-Verbindung
String htmlPageRegister = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Registrierung</title>
</head>
<body>
  <h1>Gerät registrieren</h1>
  <p>Die WLAN-Verbindung war erfolgreich!</p>
  <form action="/register" method="post">
    Registrierungscode: <input type="text" name="registerCode">
    <input type="submit" value="Absenden">
  </form>
</body>
</html>)rawliteral";

void setup() {
  delay(2000);
  Serial.begin(115200);
  preferences.begin("wifi", false);

  // Access Point starten und WLAN-Verbindung parallel versuchen
  startAccessPoint();
  connectToWifi();  // WLAN-Verbindung versuchen

  // GET-Anfrage für die Konfigurations- oder Registrierungsseite
  server.on("/", HTTP_GET, []() {
    if (isWifiConnected) {
      // Zeigt die Registrierungsseite an, wenn verbunden
      server.send(200, "text/html", htmlPageRegister);
    } else {
      // Zeigt die WLAN-Konfigurationsseite an, wenn keine Verbindung besteht
      server.send(200, "text/html", htmlPageWifiConfig());
    }
  });

  // POST-Anfrage für die WLAN-Daten
  server.on("/", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    // WLAN-Daten speichern und Verbindung versuchen
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);

    if (connectToWifi()) {
      isWifiConnected = true;  // Verbindung erfolgreich, Registrierungsseite wird bei GET-Anfrage angezeigt
      server.send(200, "text/html", htmlPageRegister);
    } else {
      // Verbindung fehlgeschlagen, Fehlermeldung anzeigen
      server.send(200, "text/html", htmlPageWifiConfig("<p>WLAN-Verbindung fehlgeschlagen. Bitte erneut versuchen.</p>"));
    }
  });

  // Route für Registrierungscode
  server.on("/register", HTTP_POST, []() {
    String registerCode = server.arg("registerCode");
    HTTPClient http;
    http.begin(register_url);
    int httpResponseCode = http.POST("{\"registerCode\":\"" + registerCode + "\"}");
    http.end();

    if (httpResponseCode == HTTP_CODE_OK) {
      server.send(200, "text/html", "<h1>Registrierung erfolgreich</h1>");
      // Access Point deaktivieren und nur im WLAN-Modus bleiben
      WiFi.softAPdisconnect(true);
    } else {
      server.send(200, "text/html", "<h1>Registrierung fehlgeschlagen</h1>");
    }
  });

  server.begin();  // Webserver starten
}

bool connectToWifi() {
  String ssid = preferences.getString("ssid");
  String password = preferences.getString("password");

  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Versuche, mit dem WLAN zu verbinden...");

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWLAN-Verbindung erfolgreich!");
    Serial.println("IP-Adresse: ");
    Serial.println(WiFi.localIP());
    isWifiConnected = true;  // Status auf „verbunden“ setzen
    return true;
  } else {
    Serial.println("\nVerbindung zum WLAN fehlgeschlagen.");
    isWifiConnected = false;  // Status bleibt „nicht verbunden“
    return false;
  }
}

void startAccessPoint() {
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("Access Point gestartet. IP-Adresse:");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}
