#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


class String;

void setup() {

    const char* ssid = "Pfotenweg4G";
    const char* password = "12DAN879888";

    Serial.begin(115000);
    delay(10);

    IPAddress ip(192, 168, 2, 150);
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

void sendHttp(String url, int counter) {
    HTTPClient sender;
    WiFiClient wifiClient;

    Serial.println(counter); 
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
        delay(100);
        counter++;
        if(counter >= 10)
        {
            String text = "Ich+erreiche+die+Tomaten+nicht";
            String urlInner = "http://api.callmebot.com/whatsapp.php?phone=+4915227939776&text=" + text + "&apikey=375541";
            Serial.println(urlInner); 
            sendHttp(urlInner, 0);
            delay(10000);
        }
        sendHttp(url, counter);
    }
}

int leseFeuchtigkeit()
{
#define SensorPin A0
    float sensorValue = analogRead(SensorPin);

    return sensorValue;
}

void loop() {
    int feuchtigkeit = leseFeuchtigkeit();
    Serial.println(feuchtigkeit);
    String url = ""; 
    if(feuchtigkeit < 480)
    {
        url = "http://192.168.2.98/RELAY=ON";
    }else
    {
        url = "http://192.168.2.98/RELAY=OFF";
    }


   
    Serial.println(url);
    sendHttp(url, 0);


    delay(100000);
}



