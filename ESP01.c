/* ESP-01 - Klient, łączy się z AP ESP8266, wysyła IP i uruchamia serwer do sterowania przekaźnikiem */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h> // Potrzebne dla własnego serwera ESP-01

/********************************************************************
 |        Żeby kod działał: należy zewrzeć piny: TX z RST.          |
 |      W readMe jest dokłady opis wgrywania kodu na ESP 01S        |
 ********************************************************************/

// Dane logowania do sieci AP stworzonej przez ESP8266
const char* ssid = "ESP_projekt_2025";
const char* password = "espProjekt";

const char* host_esp8266 = "192.168.4.1"; // Adres IP serwera ESP8266 (jego adres jako Access Point)
const int relayPin = 0; // GPIO0 (ESP-01S relay)

ESP8266WebServer esp01_server(80); // Serwer HTTP na ESP-01

void connectToWiFi();
void sendToServer(const String& path);
void startWebServer();
void handleRelayON();
void handleRelayOFF();
void handleRoot();


/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\ setup()  loop() /‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Stan początkowy przekaźnika: WYŁĄCZONY (LOW)
  Serial.println("Przekaźnik inicjalnie WYŁĄCZONY (LOW).");

  connectToWiFi();
  // sendToServer("/sendip"); Nie ma potrzeby tutaj tego dodawać, jest w loop()
  startWebServer();
}

/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void loop() {
  esp01_server.handleClient();
  /*Zmienne static są INICJALIZOWANE TYLKO przy pierwszym uruchomieniu pętli */
  static unsigned long lastReconnectAttempt = 0;
  static bool wasConnected = false; /* false, by uruchomić sendToServer()    */

  if (WiFi.status() != WL_CONNECTED) { /*   jeżeli NIE ma połączenia z WiFi: */
    if (wasConnected) {
      Serial.println("Utracono połączenie z WiFi!");
      wasConnected = false;/* Poprzedni status połączenia dla następnej pętli*/
    }
    /* Czy upłyneło już 20 [s] od ostatniej próby połączenia
    [CZAS OD URUCHOMIENIA] - [CZAS OSTATNIEJ PRÓBY POŁĄCZENIA] > 20 [s]      */
    if (millis() - lastReconnectAttempt > 20000) {
      lastReconnectAttempt = millis(); /*         millis() zwraca [ms] od    */
      connectToWiFi(); /*                         uruchomienia urządzenia.   */
    }
  } else { /*                              jeżeli jesteśmy połączeni z WiFi: */
    if (!wasConnected) { /*               a w poprzedniej pętli nie byliśmy. */
      Serial.println("Połączenie z WiFi przywrócone.");
      wasConnected = true;/* Poprzedni status połączenia dla następnej pętli */
      sendToServer("/sendip"); /*        Wysyłamy wiadomość do serwera z ip. */
    }
  }
}





/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\ Funkcje /‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void connectToWiFi() { /*                     FUN DO ŁĄCZENIA SIĘ Z SERWEREM */
  Serial.print("Łączenie z WiFi AP '");
  Serial.print(ssid);
  Serial.print("'...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { /* Przez 10 [s]   */
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nPołączono z WiFi!");
    Serial.print("Adres IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nNie udało się połączyć z WiFi.");
  }
}

/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void sendToServer(const String& path) {/* DO WYSYŁANIA WIADOMOŚCI DO SERWERA */
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Brak połączenia z WiFi. Nie można wysłać danych.");
    return;
  }

  WiFiClient client;
  HTTPClient http;
  String url = String("http://") + host_esp8266 + path;
  Serial.print("Wysyłanie danych do: ");
  Serial.println(url);

  if (http.begin(client, url)) {
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postData = "ip=" + WiFi.localIP().toString();
    int httpCode = http.POST(postData);

    if (httpCode > 0) {
      Serial.printf("[HTTP] POST... kod: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Odpowiedź serwera ESP8266:");
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] Błąd POST: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Błąd inicjalizacji http.begin()");
  }
}

/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void startWebServer() {
  esp01_server.on("/", HTTP_GET, handleRoot);
  esp01_server.on("/relayON", HTTP_GET, handleRelayON);
  esp01_server.on("/relayOFF", HTTP_GET, handleRelayOFF);
  esp01_server.begin();
  Serial.println("Serwer HTTP uruchomiony.");
}



/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\  mini  Funkcje  /‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void handleRelayON() {
  digitalWrite(relayPin, HIGH); // Włącz przekaźnik (stan wysoki)
  Serial.println("Przekaźnik WŁĄCZONY (HIGH) na żądanie.");
  esp01_server.send(200, "text/plain", "Relay is ON");
}

/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void handleRelayOFF() {
  digitalWrite(relayPin, LOW); // Wyłącz przekaźnik (stan niski)
  Serial.println("Przekaźnik WYŁĄCZONY (LOW) na żądanie.");
  esp01_server.send(200, "text/plain", "Relay is OFF");
}

/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void handleRoot() {
 String status = (digitalRead(relayPin) == HIGH) ? "ON" : "OFF";
 esp01_server.send(200, "text/plain", "ESP-01 Relay Control. Current state: " + status +
 "\nUse /relayON to turn ON\nUse /relayOFF to turn OFF");
}
