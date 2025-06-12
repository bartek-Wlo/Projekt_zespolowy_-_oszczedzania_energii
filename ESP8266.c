/* ESP8266 - Działa jako Serwer HTTP, Access Point dla ESP-01 oraz Klient WiFi dla Straßenbahn */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h> // Potrzebne do wysyłania żądań do ESP-01

/********************************************************************
 |        Żeby kod działał: należy zewrzeć piny: TX z RST.          |
 |      W readMe jest dokłady opis wgrywania kodu na ESP 01S        |
 ********************************************************************/

#define uS_TO_S_FACTOR 1000000ULL  // Współczynnik konwersji z mikrosekund na sekundy
int TIME_TO_SLEEP = 20          // Czas, na jaki ESP8266 pójdzie spać (w sekundach) - 0.33 minuty = 20 sekund

// Dane logowania do sieci, którą ESP8266 będzie tworzyć (Access Point)
const char* ap_ssid = "ESP_projekt_2025";
const char* ap_password = "espProjekt";

const char* sta_ssid = "Straßenbahn_33"; /*  Nazwa sieci WiFi udostępnianej  */
const char* sta_password = "gyiu8623"; /*    Hasło sieci WiFi udostępnianej  */

const char* serverUrl = "https://panamint.kcir.pwr.edu.pl/~mlenczuk/Projekt/status.txt"; //         <<<<< ZMIEŃ NA AKTUALNY !!!!!
const char* SHA1 =      "A5 D1 7E 41 9B AD 37 F1 C0 BB 5E 0F 0D 4C 90 CB 41 F3 5F CB";  //          <<<<< ZMIEŃ NA AKTUALNY !!!!!

bool GoDeepSleepMode = true; /* Gdy nie nawiąże połączenia będzie się usypiać*/
const int ledPin = LED_BUILTIN; // Zwykle GPIO2 na NodeMCU/Wemos D1 Mini
String esp01_ip_address = "";   // Zmienna do przechowywania IP ESP-01


ESP8266WebServer server(80); /* Ustawienie portu 80 do nasłuchiwania         */
void initializeSerialAndLED();
bool connectToWiFiSTA();
bool checkInternetConnection();
void getCommandFromRemoteServer();

void enterDeepSleep();
void powerDownSequence();
bool powerUpSequence();
void restartSequence();

void setupAccessPoint();
void setupHTTPcommands();
void getSleepTimeFromServer();

bool sendRelayCommandToESP01(String host_ip, const char* command_path);
bool sendShutdownCommandToOdroid(); /* Zmienne Globalne do Funkcji:          */
bool pingOdroidServer(int restartNumber);
  const String odroid_ip = "192.168.55.83"; // Adres IP ODROID-M1 w sieci lokalnej                 <<<<< ZMIEŃ NA AKTUALNY !!!!!
  const String odroid_shutdown_path = "/shutdown"; // endpoint ODROID-M1

/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\ setup()  loop() /‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void setup() {
  initializeSerialAndLED();

  if(connectToWiFiSTA() == false) enterDeepSleep();
  if(checkInternetConnection() == false) enterDeepSleep();;
  
  getCommandFromRemoteServer();  if(GoDeepSleepMode) enterDeepSleep();

  setupAccessPoint();
  setupHTTPcommands();
  getSleepTimeFromServer();

  server.begin();
  Serial.println(" | Serwer HTTP uruchomiony. Nasłuchuje na obu interfejsach.");
}

/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void loop() {
  server.handleClient();
  /*Zmienne static są INICJALIZOWANE TYLKO przy pierwszym uruchomieniu pętli */
  static unsigned long lastReconnectAttempt = 0;
  static unsigned long lastTurnOnAttempt = 0;
  static bool wasConnected = WiFi.status() == WL_CONNECTED;
  static int ReconnectAttempt = 0;
  static bool turnON = false;
  static bool odroidON = false; static int pingAttempt = 0;
  
  if (WiFi.status() != WL_CONNECTED) {
    if (wasConnected) { 
      ReconnectAttempt = 0;
      Serial.println(" | Utracono połączenie z WiFi!");
      wasConnected = false;/* Poprzedni status połączenia dla następnej pętli*/
    }
    /* Czy upłyneło już 30 [s] od ostatniej próby połączenia
    [CZAS OD URUCHOMIENIA] - [CZAS OSTATNIEJ PRÓBY POŁĄCZENIA] > 30 [s]      */
    if (millis() - lastReconnectAttempt > 30000) {
      lastReconnectAttempt = millis();
      if( connectToWiFiSTA() /* Trwa 15 [s]*/) wasConnected = true;
      else ++ReconnectAttempt; 
      if(ReconnectAttempt >= 4) powerDownSequence(); /* Następuje po 2 [min] */
    } 
  } else if (millis() - lastReconnectAttempt > 10000) { /* Co 10 [s]:        */
    lastReconnectAttempt = millis();
    getCommandFromRemoteServer();    
    if(GoDeepSleepMode) powerDownSequence();
  } else if ((turnON==false)&&(millis() - lastTurnOnAttempt > 5000)) {/*5 [s]*/
    if(powerUpSequence()) turnON = true;
    lastTurnOnAttempt = millis();
  } else if ((turnON)&&(odroidON==false)&&(millis()-lastTurnOnAttempt > 20000)) {
    if(sendRelayCommandToESP01(esp01_ip_address, "/")==false) {turnON = false; pingAttempt = 0;}
    else if(pingOdroidServer(pingAttempt)) odroidON = true; /* CO 20 [s] sprawdza */
    else {
      lastTurnOnAttempt = millis();
      if( ++pingAttempt >= 6) {
        restartSequence();
        pingAttempt = 0;
      }
    }
  }
}







/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\     FUNKCJE     /‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void initializeSerialAndLED() {
  Serial.begin(74880); delay(500);
  // Serial.begin(115200); delay(500); //By port szeregowy zdążył się zainicjować
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);  // LED off
}

bool connectToWiFiSTA() {//‾‾‾‾‾‾‾‾‾‾‾‾‾WiFi Station‾‾(zewnętrzne)‾‾‾‾‾‾‾‾‾‾‾‾‾
  // Serial.println("\n | Konfiguracja ESP8266...");
  WiFi.mode(WIFI_AP_STA); /* ustawia tryb pracy modułu Wi-Fi: tworzy własną sieć Wi-Fi, łączy się z istniejącą siecią Wi-Fi. */
  // Serial.println(" | Tryb WIFI_AP_STA ustawiony.");

  WiFi.begin(sta_ssid, sta_password); /* Łączy się z istniejącą siecią Wi-Fi. - Station (STA)*/
  Serial.println(" | Łączenie z WiFi (STA) '" + String(sta_ssid) + "'");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) { /* Przez 15 [s]   */
    delay(500); 
    Serial.print(".");
    ++attempts;
  }
  if(WiFi.status() == WL_CONNECTED) {
    // Serial.println("\n | Połączono z WiFi (STA)!"); // <-- Pierwszy komunikat który widać na Seial Monitor.
    Serial.println(" | Adres IP serwera (STA): " + WiFi.localIP().toString() );
    // delay(2000) // 2 [s] na połączenie
    return true;
  }
  Serial.println("\n | Time Out, NIE połączono z WiFi (STA)!");
  return false;
}

bool checkInternetConnection() { //‾‾‾‾‾‾‾Check czy połączy się z google‾‾‾‾‾‾‾
  Serial.println("\n | Próba nawiązania połączenia z google.com");
  IPAddress testIP;
  if (WiFi.hostByName("google.com", testIP)) {
    Serial.println(" | OK. IP: " + testIP.toString() );
    return true;
  } else {
    Serial.println(" | Błąd DNS! ESP nie może rozwiązać nazw hostów.");
    return false;
  }
}

void getCommandFromRemoteServer() { //‾‾‾‾‾‾‾Łączenie z zdalnym Serverem‾‾‾‾‾‾‾
  Serial.println("\n HTTP: Rozpoczynam połączenie z serwerem.");
  WiFiClientSecure client;
  // client.setInsecure();
  client.setFingerprint(SHA1);
  HTTPClient http;
  // delay(500);
  http.begin(client, serverUrl);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println(" HTTP: Odpowiedź z serwera: " + payload);
    
    if (payload == "ON") {Serial.println(" HTTP:    Stan: ON"); GoDeepSleepMode = false;} 
    else if (payload == "OFF") {Serial.println(" HTTP:   Stan: OFF"); GoDeepSleepMode = true;}
    else Serial.println(" HTTP:   Stan: nie rozpoznany!");
  } else {
    Serial.println(" HTTP: Błąd połączenia, kod HTTP: " + String(httpCode));
  }
  http.end();
}

/*______________ENTER DEEP, SLEEP, POWER OFF, POWER ON, RESTART______________*/
// if (GoDeepSleepMode) {
void enterDeepSleep() { //‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾Uruchamianie Deep Sleep‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
  Serial.println(" DS: Przygotowuję się do trybu Deep Sleep.");
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin(); delay(1); // Krótka pauza dla stabilizacji
  Serial.printf(" DS: Deep sleep for %d [s] (%llu [ms]).\n", TIME_TO_SLEEP, (unsigned long long)TIME_TO_SLEEP * uS_TO_S_FACTOR);
  ESP.deepSleep(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println(" DS: ERROR, NIE WŁĄCZONO TRYBU DEEP SLEEP!!");
}
// }

void powerDownSequence() {//‾‾‾‾‾‾‾‾‾‾‾‾‾Wyłączanie CAŁEGO systemu‾‾‾‾‾‾‾‾‾‾‾‾‾
  Serial.println("\n OFF:   Procedura: Relay OFF");
    if(sendShutdownCommandToOdroid()) { // Opcjonalnie: endpoint do wyłączania przekaźnika na ESP-01
      Serial.print(" OFF:   Czekam na zamknięcie Odroid-M1\n");
      delay(15000); // 15 [s], odroid wyłącza się ~11 [s].
    }
    Serial.print(" OFF:   Odcinam zasilanie do Odroid-M1\n");
    sendRelayCommandToESP01(esp01_ip_address, "/relayOFF");
    server.send(200, "text/plain", "Polecenie /relayOFF wysłane do ESP-01.");
    Serial.println("\n OFF:   Procedura: przekaźnik jest OFF, płytka idze spać!");
    enterDeepSleep();
}

bool powerUpSequence() {// ‾‾‾‾‾‾‾‾‾‾‾‾‾‾Włączanie CAŁEGO systemu‾‾‾‾‾‾‾‾‾‾‾‾‾‾
  Serial.println("\n ON:    Procedura: Relay ON");
  if(sendRelayCommandToESP01(esp01_ip_address, "/relayON") == false) return false;
  server.send(200, "text/plain", "Polecenie /relayON wysłane do ESP-01.");
  return true;
}

void restartSequence() { //‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾Restart CAŁEGO systemu‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
  Serial.println("\n REBOOT: Restart zasilania Odroid-M1");
  sendRelayCommandToESP01(esp01_ip_address, "/relayOFF"); // Odcięcie zasilania
  server.send(200, "text/plain", "Polecenie /relayOFF wysłane do ESP-01.");
  delay(3000); // Krótkie odczekanie przed ponownym włączeniem
  sendRelayCommandToESP01(esp01_ip_address, "/relayON");  // Włączenie zasilania ponownie
  server.send(200, "text/plain", "Restart Odroid-M1 zakończony.");
  Serial.println(" REBOOT: Procedura: Restart zakończona.");
}/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾ENTER DEEP, SLEEP, POWER OFF, POWER ON, RESTART‾‾‾‾‾‾‾‾‾‾‾‾‾*/


void setupAccessPoint() { //‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾WiFi Access Point‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
  Serial.print(" | Tworzenie Access Point: "); /* Tworzy własną sieć Wi-Fi. - Access Point (AP)*/
  Serial.println(ap_ssid);
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print(" | AP IP address: ");
  Serial.println(apIP);
}










/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/  COMMAND:  HTTP - ESP 8266  \_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
void setupHTTPcommands() {
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
      /* Usunałem to, bo i tak pierwszą rzeczą do zrobienia będize ustawienie relay ON */
      delay(1000);
      sendRelayCommandToESP01(esp01_ip_address, "/relayOFF"); // Wyłączamy na wszeli wypadek przekaźnik */
    } else {
      server.send(400, "text/plain", "Brak parametru 'ip'");
    }
  });

  server.on("/turnRelayOffOnESP01", HTTP_GET, [](){
    powerDownSequence();
  });

  server.on("/turnRelayOnOnESP01", HTTP_GET, [](){
    powerUpSequence();
  });
}


void getSleepTimeFromServer() {
  Serial.println("ESP: Pobieranie sleep_time...");
  WiFiClientSecure client;
  client.setFingerprint(SHA1);
  HTTPClient http;
  http.begin(client, "https://panamint.kcir.pwr.edu.pl/~mlenczuk/Projekt/sleep_time.txt");

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    TIME_TO_SLEEP = payload.toInt();
    Serial.println("ESP: Odczytano TIME_TO_SLEEP = " + String(TIME_TO_SLEEP));
  } else {
    Serial.println("ESP: Nie udało się pobrać sleep_time.txt");
  }

  http.end();
}



/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/  COMMAND:  ESP 01S  \_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
bool sendRelayCommandToESP01(String host_ip, const char* command_path) {
  if (host_ip.length() == 0) {
    Serial.println(" 01 command:    Nie znam adresu IP ESP-01, nie mogę wysłać komendy.");
    return false;
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
      return false;
    }
    http_to_esp01.end();
    return true;
  } else {
    Serial.println(" 01 command:    Nie można zainicjować połączenia HTTP do ESP-01.");
    return false;
  }
}


/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/ COMMAND:  ODROID M1 \_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/   ODROID_ip:5000/shutdown   \_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
bool sendShutdownCommandToOdroid() {
  if (odroid_ip.length() == 0 || odroid_shutdown_path.length() == 0) {
    Serial.println(" M1:    Brakuje IP Odroida lub ścieżki shutdown – przerwano.");
    return false;
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
      }/* Jeżeli HTTP code == Connection Lost == ODROID się wyłączył. */ 
    } else if(http_odroid.errorToString(httpCode).equals("connection lost") || httpCode == -11) {
      Serial.printf(" M1:    [HTTP Odroid] GET... code: %s\n", http_odroid.errorToString(httpCode).c_str());
    } else {
      Serial.printf(" M1:    [HTTP Odroid] GET... nie powiodło się, błąd: %s\n", http_odroid.errorToString(httpCode).c_str());
      http_odroid.end(); return false;
    }
    http_odroid.end();
    return true;
  } else {
    Serial.println(" M1:    Nie można zainicjować połączenia HTTP do Odroida.");
    return false;
  }
}

/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
/*\_/‾\_/‾\_/‾\_/‾\_/‾\_/     ODROID_ip:5000/ping     \_/‾\_/‾\_/‾\_/‾\_/‾\_/*/
/*/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\*/
bool pingOdroidServer(int restartNumber) {
  if (odroid_ip.length() == 0) {
    Serial.println(" Brak IP serwera Odroid.");
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  String url = "http://" + odroid_ip + ":5000/ping";  // Zakładamy port 5000

  Serial.print(" M1:    Sprawdzanie "+String(restartNumber)+". serwera Odroid pod adresem: ");
  Serial.println(url);

  if (http.begin(client, url)) {
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.printf(" M1:    Odpowiedź HTTP: %d\n", httpCode);
      String payload = http.getString();
      Serial.println(" M1:    Odpowiedź serwera: " + payload);
      return payload == "pong";
    } else {
      Serial.printf(" M1:    Błąd HTTP: %s\n", http.errorToString(httpCode).c_str());
      return false;
    }
    http.end();
  } else {
    Serial.println(" M1:    Nie udało się nawiązać połączenia.");
    return false;
  }
}