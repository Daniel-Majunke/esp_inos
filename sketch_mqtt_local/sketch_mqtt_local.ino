#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>

// WLAN-Konfiguration
String ssid = "";       // Dein WLAN-Name
String password = "";  // Dein WLAN-Passwort
String topics = "";  // Dein Topic wird wahrscheinlich noch umbenannt
// MQTT-Server-Konfiguration
const char* mqtt_server = "192.168.2.34";  // IP-Adresse des Servers (ersetze durch die IP deines Rechners)
const int mqtt_port = 1883;                 // Port des MQTT-Brokers (Standardport für MQTT ohne SSL/TLS)

//Relais
int relayPin = 5;      // Pin, an dem das Relais angeschlossen ist
bool relayState = LOW; // Aktueller Zustand des Relais

WiFiClient espClient;
Preferences preferences;
PubSubClient client(espClient);

// WLAN-Verbindung herstellen
void setup_wifi() {
  Serial.println();
  Serial.print("Verbinden mit ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WLAN verbunden");
  Serial.println("IP-Adresse: ");
  Serial.println(WiFi.localIP());
}
 
//Nachrichten empfangen
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nachricht empfangen [");
  Serial.print(topic);
  Serial.print("] ");
  String payloadStr = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadStr += (char)payload[i]; 
  }
  setRelayState(payloadStr);
  Serial.println();
  String newTopic = "response/" + String(topic);
  const char* payloadPublish = "success";
  boolean retained = false;
  client.publish(newTopic.c_str(),payloadPublish, retained);
  Serial.println(newTopic);
  Serial.println(String(payloadPublish));
}

// Verbindung mit dem MQTT-Broker herstellen und Topic abonnieren
void reconnect() {
  while (!client.connected()) {
    Serial.print("Versuche Verbindung zum MQTT-Server aufzubauen...");
    if (client.connect("ESP32Client")) {
      Serial.println("Verbunden");
      Serial.println("topicAbo bei recconnect:" + topics);

      client.subscribe(topics.c_str()); // Beispiel-Topic abonnieren
    } else {
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println("; neuer Versuch in 5 Sekunden");
      delay(5000);
    }
  }
}

void setRelayState(String state) {
  if (state == "on") {
    digitalWrite(relayPin, HIGH); // Relais einschalten
  } else if (state == "off") {
    digitalWrite(relayPin, LOW);  // Relais ausschalten
  }
}

void setup() {
  delay(3000);
  Serial.begin(115200);
  Serial.print("start");

  pinMode(relayPin, OUTPUT);            // Setze den Relais-Pin als Ausgang
  digitalWrite(relayPin, relayState);

  preferences.begin("wifi-config", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();

  preferences.begin("mqtt-config", true);
  topics = preferences.getString("topics", "");
  preferences.end();
  Serial.print("topics" + topics);

  setup_wifi();                             // WLAN-Verbindung herstellen
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