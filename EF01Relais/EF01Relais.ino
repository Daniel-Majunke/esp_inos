#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <DHT.h>

const char* ssid = "Pfotenweg4G";
const char* password = "12DAN879888";

const char* host = "www.autarkes-leben.eu";
const int httpsPort = 443;

const int DHTPIN = 2;
const int DHTTYPE = DHT11;

DHT dht(DHTPIN, DHTTYPE);

unsigned long lastTime = 0;
unsigned long timerDelay = 3600*1000;

void setup() {
  Serial.begin(115200);
  Serial.println();

  dht.begin();
  
  measureAndSendData();
}

void loop() {
  if (millis() - lastTime >= timerDelay) {
    lastTime = millis();
    measureAndSendData();
  }
}

void measureAndSendData() {
  Serial.print("Verbindung zum WLAN wird hergestellt...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect(host, httpsPort)) {
    Serial.println("Verbindung fehlgeschlagen!");
    return;
  }
  Serial.println("Verbunden!");

  // GET-Anfrage senden
  int temperature = static_cast<int>(dht.readTemperature()) - 10;
  int humidity = static_cast<int>(dht.readHumidity());
  String url = "/ApiTemperatur/SaveTemperatur?apiSchluessel=C1hoy9KZc5yg4RqO4Ljh&temperatur=" + String(temperature) + "&feuchtigkeit=" + String(humidity) + "&name=Wohnstube&geraeteId=10";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");
  
  client.stop();
  WiFi.disconnect( true );
  delay( 1 );
  Serial.println("WLAN ausgeschaltet");
}

