#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>

const char* ssid = "Pfotenweg4G";
const char* password = "12DAN879888";

const char* host = "www.autarkes-leben.eu";
const int httpsPort = 443;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Verbindung zum WLAN wird hergestellt...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Erfolgreich mit dem WLAN verbunden!");
  Serial.println("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  // Verbindung zum Server herstellen
  BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Zertifikatsüberprüfung deaktivieren
  Serial.print("Verbindung zum Server wird hergestellt...");
  if (!client.connect(host, httpsPort)) {
    Serial.println("Verbindung fehlgeschlagen!");
    return;
  }
  Serial.println("Verbunden!");

  // GET-Anfrage senden
  String url = "/ApiTemperatur/SaveTemperatur?apiSchluessel=O/WJkmg+U/bfhkSBuznFkg==&temperatur=22&feuchtigkeit=33";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  // Antwort empfangen
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  
  // Antwort im seriellen Monitor anzeigen
  String line;
  while(client.available()){
    line = client.readStringUntil('\n');
    Serial.println(line);
  }
}

void loop() {
}
