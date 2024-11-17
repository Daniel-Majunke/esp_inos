#include <Preferences.h>

Preferences preferences;

void setup() {
  // Starte die serielle Kommunikation
  delay(3000);
  Serial.begin(115200);
  Serial.print("start");

  // Setze die WLAN-Konfiguration auf leere Strings
  preferences.begin("wifi-config", false);  // `false` bedeutet, dass der Namespace zum Schreiben geöffnet wird
  preferences.putString("ssid", "");  // Setze SSID auf ""
  preferences.putString("password", "");  // Setze Passwort auf ""
  preferences.end();
  
  // Setze die MQTT-Konfiguration auf einen leeren String
  preferences.begin("mqtt-config", false);
  preferences.putString("topics", "");
  preferences.putBool("getConfig", false); 
  preferences.end();

  Serial.println("SSID und Passwort auf leer gesetzt.");
  Serial.println("Topics auf leer gesetzt.");
}

void loop() {
  // Hauptcode läuft hier
}
