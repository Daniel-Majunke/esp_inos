#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>



void setup() {

    const char* ssid = "Pfotenweg4G";
    const char* password = "12DAN879888";

    Serial.begin(115000);
    delay(10);

    IPAddress ip(192, 168, 2, 150);
    IPAddress gateway(192, 168, 178, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns(192, 168, 178, 1);
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
    String url = "http://api.callmebot.com/whatsapp.php?phone=+4915227939776&text=This+is+a+test&apikey=375541"; 
    //sendHttp(url);
    leseFeuchtigkeit();
    
    delay(100000);
}

void leseFeuchtigkeit() 
{
    #define SensorPin A0
    float sensorValue = analogRead(SensorPin);
    Serial.println(sensorValue);
}

void sendHttp(String url ) {
    HTTPClient sender;
    WiFiClient wifiClient;

    sender.begin(wifiClient, url);

    // HTTP-Code der Response speichern
    int httpCode = sender.GET();
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String payload = sender.getString();
            Serial.println(payload);
        }
        sender.end();
    }
    else {
        Serial.printf("HTTP-Verbindung konnte nicht hergestellt werden!");
    }
}