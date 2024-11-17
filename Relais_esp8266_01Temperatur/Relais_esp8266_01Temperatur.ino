#include <ESP8266WiFi.h>
#include <DHT.h>
#include <WiFiClientSecure.h>

const char* ssid = "Pfotenweg4G";
const char* password = "12DAN879888";

#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* serverAddress = "www.autarkes-leben.eu";
const int serverPort = 443;  // Port 443 für HTTPS
const char* apiEndpoint = "/apitemperatur/savetemperatur";
const char* apiSchluessel = "O/WJkmg+U/bfhkSBuznFkg==";

const char* accessToken = "YOUR_ACCESS_TOKEN"; // Fügen Sie hier Ihren Access Token ein

void setup() {
  Serial.begin(115200);
  delay(10);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Verbindung zum WLAN wird hergestellt...");
  }

  Serial.println("Erfolgreich mit dem WLAN verbunden!");

  dht.begin();
}

void loop() {
  // Temperatur und Feuchtigkeit lesen
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  Serial.print("Temperatur: ");
  Serial.print(temperature);
  Serial.println("°C");

  Serial.print("Luftfeuchtigkeit: ");
  Serial.print(humidity);
  Serial.println("%");

  // HTTPS-POST-Anfrage an den Server senden
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    Serial.println("bIN IM iF");
    // Verbindung zum Server herstellen
    if (client.connect(serverAddress, serverPort)) {
     Serial.println("clientconnect");
      String postData = "apiSchluessel=";
      postData += apiSchluessel;
      postData += "&temperatur=";
      postData += String(temperature);
      postData += "&feuchtigkeit=";
      postData += String(humidity);

      client.print(String("POST ") + apiEndpoint + " HTTP/1.1\r\n" +
                   "Host: " + serverAddress + "\r\n" +
                   "Content-Type: application/x-www-form-urlencoded\r\n" +
                   "Content-Length: " + String(postData.length()) + "\r\n" +
                   "Connection: close\r\n\r\n" +
                   postData);

      // Warten auf Antwort des Servers
      delay(5000); // Kurze Verzögerung, um sicherzustellen, dass die Antwort vollständig ist

      // Antwort empfangen
      String serverResponse;
      while (client.connected() || client.available()) {
        if (client.available()) {
          char c = client.read();
          serverResponse += c;
        }
      }

      // Antwort im Serial Monitor anzeigen
      Serial.println("Serverantwort:");
      Serial.println(serverResponse);

      // Verbindung schließen
      client.stop();
    }
  }

  delay(6000); // Eine Sekunde warten
  Serial.println("Im WLAN eingeloggt");
}
