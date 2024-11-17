#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin("FRITZ!BOX 5530 PL", "Tassen12%");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Verbindung wird hergestellt...");
  }
  Serial.println("Verbunden mit dem WLAN");

  client.setServer("mqtt.server.com", 1883);
  client.setCallback(callback);
  client.connect("ESP32Client");
  client.subscribe("your/topic");
}

void loop() {
  client.loop();
}
