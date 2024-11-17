#include <dummy.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Netzwerk-Konfiguration
const char* ssid = "FRITZ!Box 5530 PL";
const char* password = "Tassen12%";

// MQTT-Server-Konfiguration
const char* mqtt_server = "autarkes-leben.eu"; // Deine Domain
const int mqtt_port = 8883; // Sicherer MQTT-Port über TLS

WiFiClientSecure espClient;  // WiFiClientSecure für SSL/TLS-Verbindungen
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Verbinden mit ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WLAN verbunden");
  Serial.println("IP-Adresse: ");
  Serial.println(WiFi.localIP());
}

// MQTT-Nachricht empfangen
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nachricht empfangen [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  String newTopic = "response/" + String(topic);
  const char* payloadPublish = "success";
  boolean retained = false;
  client.publish(newTopic.c_str(),payloadPublish, retained);
}

// Verbindung mit dem MQTT-Broker herstellen und Topic abonnieren
void reconnect() {
  while (!client.connected()) {
    Serial.print("Versuche Verbindung zum MQTT-Server aufzubauen...");
    if (client.connect("ESP32Client")) {
      Serial.println("Verbunden");
      client.subscribe("userId/d0b0bfc27c7d42a7aef77005a6379b94"); // Beispiel-Topic abonnieren
    } else {
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println("; neuer Versuch in 5 Sekunden");
      delay(5000);
    }
  }
}

void setup() {
  setup_wifi(); 
  espClient.setInsecure();                            // WLAN-Verbindung herstellen
  client.setServer(mqtt_server, mqtt_port); // MQTT-Server und Port einstellen
  client.setCallback(callback);             // Callback für empfangene Nachrichten
}

void loop() {
  if (!client.connected()) {
    reconnect(); // Verbindung wiederherstellen, falls verloren
  }
  client.loop(); // MQTT-Client-Loop
  delay(2000); // 2 Sekunden warten
}