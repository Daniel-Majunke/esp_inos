#include <WiFi.h>

void setup() {
  Serial.begin(115200);

  // WiFi initialisieren, auch wenn keine Verbindung hergestellt wird
  WiFi.mode(WIFI_STA);
  WiFi.begin();

  // Warte, bis das WiFi-Modul bereit ist
  delay(2000);

  // MAC-Adresse abrufen
  String macAddress = WiFi.macAddress();
  Serial.println("MAC Address: " + macAddress);
}

void loop() {
  // Kein Code im loop notwendig
}
