#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define DHTPIN 12   // Ändern Sie die Pin-Nummer entsprechend Ihrem ESP8266-Board
#define DHTTYPE DHT11

const char* ssid = "Pfotenweg4g";
const char* password = "12DAN879888";

DHT dht(DHTPIN, DHTTYPE);

const char* serverAddress = "autarkes-leben.eu";
const int serverPort = 80;
const char* apiEndpoint = "/apitemperatur/savetemperatur";
const char* apiSchluessel = "wurkub5rzszkXdaqZCenHw==";

void setup() {
  Serial.begin(9600);
  delay(10);

  // Verbindung zum WLAN herstellen
  Serial.println();
  Serial.print("Verbinde mit WLAN: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi-Verbindung hergestellt");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  dht.begin();
}

void loop() {
  // Temperatur lesen
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  Serial.print("Temperatur: ");
  Serial.print(temperature);
  Serial.println("°C");

  Serial.print("Feuchtigkeit: ");
  Serial.print(humidity);
  Serial.println("%");

  // HTTP-POST an den Server senden
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;

    // Verbindung zum Server herstellen
    if (client.connect(serverAddress, serverPort)) {
      // HTTP-Header erstellen
      String postData = "apiSchluessel=";
      postData += apiSchluessel;
      postData += "&temperatur=";
      postData += temperature;
      postData += "&feuchtigkeit=";
      postData += humidity;

      // POST-Anfrage senden
      client.println("POST " + String(apiEndpoint) + " HTTP/1.1");
      client.println("Host: " + String(serverAddress));
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Content-Length: " + String(postData.length()));
      client.println();
      client.println(postData);
      client.println();

      // Antwort empfangen
      while (client.connected()) {
        if (client.available()) {
          String line = client.readStringUntil('\n');
          // Hier können Sie die Antwort des Servers verarbeiten
          Serial.println(line);
        }
      }

      // Verbindung schließen
      client.stop();
    }
  }

  // Tiefschlafmodus aktivieren
  ESP.deepSleep(30000e6);  // 5 Minuten warten (300.000 Millisekunden)

  delay(100); // Eine kleine Verzögerung, um sicherzustellen, dass der Serial-Monitor Zeit hat, die Ausgabe anzuzeigen
}
