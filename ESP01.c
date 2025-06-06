/* ESP-01 - Klient, łączy się z AP ESP8266, wysyła IP i uruchamia serwer do sterowania przekaźnikiem */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h> // Potrzebne dla własnego serwera ESP-01

// Dane logowania do sieci AP stworzonej przez ESP8266
const char* ssid = "ESP_projekt_2025";
const char* password = "espProjekt";
// Adres IP serwera ESP8266 (jego adres jako Access Point)
const char* host_esp8266 = "192.168.4.1";

const int relayPin = 0; // GPIO0 (ESP-01S relay)

ESP8266WebServer esp01_server(80); // Serwer HTTP na ESP-01

void handleRelayON() {
  digitalWrite(relayPin, HIGH); // Włącz przekaźnik (stan wysoki)
  Serial.println("Przekaźnik WŁĄCZONY (HIGH) na żądanie.");
  esp01_server.send(200, "text/plain", "Relay is ON");
}

void handleRelayOFF() {
  digitalWrite(relayPin, LOW); // Wyłącz przekaźnik (stan niski)
  Serial.println("Przekaźnik WYŁĄCZONY (LOW) na żądanie.");
  esp01_server.send(200, "text/plain", "Relay is OFF");
}

void handleRoot() {
 String status = (digitalRead(relayPin) == HIGH) ? "ON" : "OFF";
 esp01_server.send(200, "text/plain", "ESP-01 Relay Control. Current state: " + status +
 "\nUse /relayON to turn ON\nUse /relayOFF to turn OFF");
}


void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Stan początkowy przekaźnika: WYŁĄCZONY (LOW)
  Serial.println("Przekaźnik inicjalnie WYŁĄCZONY (LOW).");

  Serial.println("\nKonfiguracja ESP-01...");
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi AP serwera ('");
  Serial.print(ssid);
  Serial.print("')");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nPołączono z WiFi serwera!");
  Serial.print("Mój IP (nadany przez serwer AP): ");
  Serial.println(WiFi.localIP());

  delay(1000);

  // Wysyłka IP do serwera ESP8266
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String url = String("http://") + host_esp8266 + "/sendip";
    Serial.print("Wysyłanie IP do: ");
    Serial.println(url);

    if (http.begin(client, url)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String postData = "ip=" + WiFi.localIP().toString();
      int httpCode = http.POST(postData);

      if (httpCode > 0) {
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println("Odpowiedź serwera ESP8266:");
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] POST... nie powiodło się, błąd: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.println("http.begin() nie powiodło się. Nie można połączyć z serwerem ESP8266.");
    }
  }

  // Uruchomienie serwera na ESP-01
  esp01_server.on("/", HTTP_GET, handleRoot);
  esp01_server.on("/relayON", HTTP_GET, handleRelayON);
  esp01_server.on("/relayOFF", HTTP_GET, handleRelayOFF); // Dodajemy opcję wyłączenia
  esp01_server.begin();
  Serial.println("Serwer HTTP na ESP-01 uruchomiony. Oczekuje na komendy...");
}

void loop() {
  esp01_server.handleClient(); // Obsługuj przychodzące żądania HTTP na ESP-01
  // Nie ma już automatycznego przełączania przekaźnika
}