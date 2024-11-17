#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <DHT.h>

const char* ssid = "Pfotenweg4G";
const char* password = "12DAN879888";

const char* host = "www.autarkes-leben.eu";
const int httpsPort = 443;

const int DHTPIN = 12;
const int DHTTYPE = DHT11;

DHT dht(DHTPIN, DHTTYPE);

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

  dht.begin();

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
  int temperature = static_cast<int>(dht.readTemperature());
  int humidity = static_cast<int>(dht.readHumidity());
  String url = "/ApiTemperatur/SaveTemperatur?apiSchluessel=wurkub5rzszkXdaqZCenHw==&temperatur=" + String(temperature) + "&feuchtigkeit=" + String(humidity);
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

  // Deep Sleep aktivieren
  ESP.deepSleep(3600e6); // Eine Stunde warten (3600 Sekunden * 1e6 Mikrosekunden)
}

void loop() {
  // Nichts im Loop ausführen
}
