/* ESP8266 - Działa jako Serwer HTTP, Access Point dla ESP-01 oraz Klient WiFi dla Straßenbahn */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h> // Potrzebne do wysyłania żądań do ESP-01

/********************************************************************
 *        Żeby kod działał: należy zewrzeć piny: D0 z RST.          *
 *        W czasie wgrywania kody pinu muszą być ROZWARTE!          *
 ********************************************************************/

#define uS_TO_S_FACTOR 1000000ULL  // Współczynnik konwersji z mikrosekund na sekundy
#define TIME_TO_SLEEP  10          // Czas, na jaki ESP8266 pójdzie spać (w sekundach) - 1.5 minuty = 90 sekund

// Dane logowania do sieci, którą ESP8266 będzie tworzyć (Access Point)
const char* ap_ssid = "ESP_projekt_2025";
const char* ap_password = "espProjekt";

// Dane logowania do istniejącej sieci WiFi (np. router domowy)
const char* sta_ssid = "Straßenbahn_33";
const char* sta_password = "gyiu8623";
const char* serverUrl = "https://d4ad-156-17-147-14.ngrok-free.app/status"; // <<<<< ZMIEŃ NA AKTUALNY !!!!!
bool GoDeepSleepMode = false;

ESP8266WebServer server(80);

const int ledPin = LED_BUILTIN; // Zwykle GPIO2 na NodeMCU/Wemos D1 Mini
String esp01_ip_address = "";   // Zmienna do przechowywania IP ESP-01

/** Użyte funkcje: **/
void sendRelayCommandToESP01(String host_ip, const char* command_path);
void sendShutdownCommandToOdroid();

/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_ setup() loop() /‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void setup() {
  Serial.begin(74880); delay(500);
  // Serial.begin(115200); delay(500); //By port szeregowy zdążył się zainicjować
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);  // LED off na start

  Serial.println("\n | Konfiguracja ESP8266...");
  WiFi.mode(WIFI_AP_STA); /* ustawia tryb pracy modułu Wi-Fi: tworzy własną sieć Wi-Fi, łączy się z istniejącą siecią Wi-Fi. */
  Serial.println(" | Tryb WIFI_AP_STA ustawiony.");

  /***************************** WiFi Station (zewnętrzne) **********************************/
  WiFi.begin(sta_ssid, sta_password);
  Serial.print(" | Łączenie z WiFi (STA) '"); /* Łączy się z istniejącą siecią Wi-Fi. - Station (STA)*/
  Serial.print(sta_ssid);
  Serial.print("'");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n | Połączono z WiFi (STA)!"); // <-- Pierwszy komunikat który widać na Seial Monitor.
  Serial.print(" | Adres IP serwera (STA): ");
  Serial.println(WiFi.localIP());
  

  delay(2000); // 2 [s] na połączenie
  /*********************************** Check czy połączony z internetem *******************************************/
  Serial.println("\n | Czy jest połączenie z google.com");
  IPAddress testIP;
  if (WiFi.hostByName("google.com", testIP)) {
    Serial.print(" | OK. IP: ");
    Serial.println(testIP);
  } else {Serial.println(" | Błąd DNS! ESP nie może rozwiązać nazw hostów.");}
  /******************************* Zczytywanie komuniaktu z zdalnego serwera.  *****************************/
  Serial.println("\n HTTP: Rozpoczynam połączenie z z serwerem.");
  // WiFiClientSecure client;
  // client.setInsecure(); 
  // client.setFingerprint("17 01 44 C5 57 6E E9 C2 DA 5E 9B 18 C1 CA BE B6 6D 3A C7 4D");
  HTTPClient http;
  delay(500);
  http.begin(client, serverUrl);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println(" HTTP: Odpowiedź z serwera: " + payload);

    if (payload == "ON") {Serial.println(" HTTP:    Stan: ON"); GoDeepSleepMode = false;} 
    else if (payload == "OFF") {Serial.println(" HTTP:   Stan: OFF"); GoDeepSleepMode = true;}
    else {Serial.println(" HTTP:   Stan: nie rozpoznany!");}
  } else {
    Serial.print(" HTTP: Błąd połączenia, kod HTTP: ");
    Serial.println(httpCode);
  }
  http.end();

  /************** Zamienić 1 na warunek sprawdzający czy na serwerze jest komenda do pracy *****************/
  if(GoDeepSleepMode) { /* Sekwencja Power Down - Deep Sleep */
    Serial.println(" DS: Przygotowuję się do trybu Deep Sleep.");
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin(); delay(1); // Krótka pauza dla stabilizacji
    Serial.printf(" DS: Deep sleep for %d [s] (%llu [ms]).\n", TIME_TO_SLEEP, (unsigned long long)TIME_TO_SLEEP * uS_TO_S_FACTOR);
    ESP.deepSleep(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println(" DS: ERROR, NIE WŁĄCZONO TRYBU DEEP SLEEP!!");
  }


  /************************** WiFi Access Point *****************************/
  Serial.print(" | Tworzenie Access Point: "); /* Tworzy własną sieć Wi-Fi. - Access Point (AP)*/
  Serial.println(ap_ssid);
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print(" | AP IP address: ");
  Serial.println(apIP);

  /********************** Polecenia serwera HTTP *******************************/
  server.on("/sendip", HTTP_POST, []() { /* Definicja endpointu HTTP - Funkcja lambda: */
      esp01_ip_address = server.arg("ip"); // Zapisz IP ESP-01S
    if (server.hasArg("ip")) { /* http://[IP_ESP8266]/sendip <-- Takie zapytanie wysyła ESP-01S */
      Serial.println("\n 01:    Odebrano połączenie od klienta (ESP-01)!");
      Serial.print(" 01:    IP klienta (ESP-01): ");
      Serial.println(esp01_ip_address);
      digitalWrite(ledPin, LOW);
      server.send(200, "text/plain", "Dzięki, IP odebrane przez serwer ESP8266");
      delay(100); // Krótka pauza
      digitalWrite(ledPin, HIGH);

      // Po odebraniu IP, wyślij komendę do ESP-01, aby włączył przekaźnik
      delay(1000);
      sendRelayCommandToESP01(esp01_ip_address, "/relayOFF"); // Wyłączamy na wszeli wypadek przekaźnik

    } else {
      server.send(400, "text/plain", "Brak parametru 'ip'");
    }
  });

  /** Obsługa poleceń: **/
  // Opcjonalnie: endpoint do wyłączania przekaźnika na ESP-01
  server.on("/turnRelayOffOnESP01", HTTP_GET, [](){
    Serial.println("\n OFF:   Procedura: Relay OFF");
    sendShutdownCommandToOdroid();
    Serial.print(" OFF:   Czekam na zamknięcie Odroid-M1\n");
    delay(15000); // 15 [s]
    Serial.print(" OFF:   Odcinam zasilanie do Odroid-M1\n");
    sendRelayCommandToESP01(esp01_ip_address, "/relayOFF");
    server.send(200, "text/plain", "Polecenie /relayOFF wysłane do ESP-01.");
  });

  server.on("/turnRelayOnOnESP01", HTTP_GET, [](){
    Serial.println("\n ON:    Procedura: Relay ON");
    sendRelayCommandToESP01(esp01_ip_address, "/relayON");
    server.send(200, "text/plain", "Polecenie /relayON wysłane do ESP-01.");
  });


  server.begin();
  Serial.println(" | Serwer HTTP uruchomiony. Nasłuchuje na obu interfejsach.");
}

void loop() {
  server.handleClient();
}

/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_  def. funkcji  /‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/

void sendRelayCommandToESP01(String host_ip, const char* command_path) {
  if (host_ip.length() == 0) {
    Serial.println(" 01 command:    Nie znam adresu IP ESP-01, nie mogę wysłać komendy.");
    return;
  }

  WiFiClient client_for_esp01;
  HTTPClient http_to_esp01;
  String url = String("http://") + host_ip + command_path;

  Serial.print(" 01 command:    Wysyłanie komendy do ESP-01: ");
  Serial.println(url);

  if (http_to_esp01.begin(client_for_esp01, url)) {
    int httpCode = http_to_esp01.GET();
    if (httpCode > 0) {
      Serial.printf(" 01 command:    [HTTP ESP-01] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        String payload = http_to_esp01.getString();
        Serial.print(" 01 command:    Odpowiedź od ESP-01:");
        Serial.println(payload);
      }
    } else {
      Serial.printf(" 01 command:    [HTTP ESP-01] GET... nie powiodło się, błąd: %s\n", http_to_esp01.errorToString(httpCode).c_str());
    }
    http_to_esp01.end();
  } else {
    Serial.println(" 01 command:    Nie można zainicjować połączenia HTTP do ESP-01.");
  }
}

// Adres IP Odroida M1 (w tej samej sieci WiFi)
const String odroid_ip = "192.168.232.83";  // ← Zmień na prawidłowy adres Odroida M1
const String odroid_shutdown_path = "/shutdown"; // ← Zmień na faktyczny endpoint

void sendShutdownCommandToOdroid() {
  if (odroid_ip.length() == 0 || odroid_shutdown_path.length() == 0) {
    Serial.println(" M1:    Brakuje IP Odroida lub ścieżki shutdown – przerwano.");
    return;
  }

  WiFiClient client_odroid;
  HTTPClient http_odroid;
  String url = String("http://") + odroid_ip + ":5000" + odroid_shutdown_path;

  Serial.print(" M1:    Wysyłanie komendy shutdown do Odroida M1: ");
  Serial.println(url);

  if (http_odroid.begin(client_odroid, url)) {
    int httpCode = http_odroid.GET();
    if (httpCode > 0) {
      Serial.printf(" M1:    [HTTP Odroid] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        String payload = http_odroid.getString();
        Serial.println(" M1:    Odpowiedź od Odroida:");
        Serial.println(payload);
      } else {
        Serial.printf(" M1:    Odroid zwrócił kod odpowiedzi: %d\n", httpCode);
      }
    } else {
      Serial.printf(" M1:    [HTTP Odroid] GET... nie powiodło się, błąd: %s\n", http_odroid.errorToString(httpCode).c_str());
    }
    http_odroid.end();
  } else {
    Serial.println(" M1:    Nie można zainicjować połączenia HTTP do Odroida.");
  }
}

