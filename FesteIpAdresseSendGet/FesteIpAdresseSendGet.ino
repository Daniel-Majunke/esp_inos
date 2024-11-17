
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

HTTPClient sender;
WiFiClient wifiClient;

const char* ssid     = "Pfotenweg4G";
const char* password = "12DAN879888";
void setup() {
  #define SensorPin A0 
  
  Serial.begin(115000);
  delay(10);
 
  IPAddress ip(192, 168,2, 97);
  IPAddress gateway(192, 168, 2, 10);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 2, 10);
  WiFi.config(ip, dns, subnet);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
} 
 
void loop() {
  delay(5000);
  //feuchtigkeitsmesser
  float sensorValue = analogRead(SensorPin);
  String zustand = "";   
  Serial.println(sensorValue);
  if(sensorValue > 480){
    zustand = "ON";
  }else{
    zustand = "OFF"; 
  }
  
  
  if (sender.begin(wifiClient, "http://192.168.2.98/RELAY=" + zustand)) {

    // HTTP-Code der Response speichern
    int httpCode = sender.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = sender.getString();
        Serial.println(payload); 
      }
    }else{
      // Falls HTTP-Error
      Serial.printf("HTTP-Error: ", sender.errorToString(httpCode).c_str());
    }
    // Wenn alles abgeschlossen ist, wird die Verbindung wieder beendet
    sender.end();
    
  }else {
    Serial.printf("HTTP-Verbindung konnte nicht hergestellt werden!");
  }
}
