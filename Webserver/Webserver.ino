


/*
    HTTP-GET Request
    Basic-Code für GET-Request an einen HTTP-Server
    Created by cooper, 2020
    makesmart.net
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

HTTPClient sender;
WiFiClient wifiClient;

// WLAN-Daten
const char* ssid     = "Pfotenweg4G";
const char* password = "12DAN879888";


void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }

  Serial.println("Verbunden!");


  if (sender.begin(wifiClient, "http://jsonplaceholder.typicode.com/users/1")) {

    // HTTP-Code der Response speichern
    int httpCode = sender.GET();
   

    if (httpCode > 0) {
      
      // Anfrage wurde gesendet und Server hat geantwortet
      // Info: Der HTTP-Code für 'OK' ist 200
      if (httpCode == HTTP_CODE_OK) {

        // Hier wurden die Daten vom Server empfangen

        // String vom Webseiteninhalt speichern
        String payload = sender.getString();

        // Hier kann mit dem Wert weitergearbeitet werden
       // ist aber nicht unbedingt notwendig
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


void loop() {

}
