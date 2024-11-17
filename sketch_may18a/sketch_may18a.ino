#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

const char* ssid = "Pfotenweg4G";
const char* password = "12DAN879888";

WebServer server(80);

void handleReceive() {
  if (server.hasArg("plain") == false) { // Check if body received
    server.send(200, "text/plain", "Body not received");
    return;
  }

  String message = server.arg("plain");
  Serial.println("Received message: " + message);
  server.send(200, "text/plain", "Message received");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  server.on("/receive", HTTP_POST, handleReceive);
  server.begin();
}

void loop() {
  server.handleClient();

  if (millis() % 10000 == 0) { // Send data every 10 seconds
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin("http://37.27.36.73:5000/webhook");
      http.addHeader("Content-Type", "application/json");
      String jsonData = "{\"sensor\": \"temperature\", \"value\": 24.5}";
      int httpResponseCode = http.POST(jsonData);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
      } else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    }
  }
}

