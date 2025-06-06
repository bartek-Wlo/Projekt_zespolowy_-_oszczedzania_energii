#include <ESP8266WiFi.h> // Wymagane, nawet jeśli nie używasz Wi-Fi, bo definiuje funkcje oszczędzania energii

/********************************************************************
 *        Żeby kod działał: należy zewrzeć piny: D0 z RST.          *
 *        W czasie wgrywania kody pinu muszą być ROZWARTE!          *
 ********************************************************************/

#define uS_TO_S_FACTOR 1000000ULL  // Współczynnik konwersji z mikrosekund na sekundy
#define TIME_TO_SLEEP  10         // Czas, na jaki ESP8266 pójdzie spać (w sekundach) - 1.5 minuty = 90 sekund

void setup() {
  // Inicjalizacja portu szeregowego do debugowania
  // Serial.begin(115200);
  Serial.begin(74880); delay(500); // Lepiej 74880 -> to nie będzie kaszków
  delay(100); // Krótka pauza, aby port szeregowy zdążył się zainicjować

  Serial.println("\n--- ESP8266 Test Deep Sleep ---");
  Serial.println("Płytka obudziła się!");

  // --- Tutaj umieść swoją główną logikę działania ---
  // To jest kod, który zostanie wykonany za każdym razem, gdy płytka się obudzi.
  // Np. odczyt czujnika, wysłanie danych do MQTT, zaktualizowanie wyświetlacza itp.
  Serial.println("Wykonuję zadania (np. odczyt czujnika, wysyłanie danych)...");
  delay(2000); // Symulacja "pracy" przez 2 sekundy

  // --- Przygotowanie do trybu Deep Sleep ---
  Serial.println("Zadania zakończone. Przygotowuję się do trybu Deep Sleep.");

  // Wyłącz Wi-Fi, aby zaoszczędzić energię (jeśli było używane)
  // Te linie są ważne dla maksymalnej oszczędności energii
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1); // Krótka pauza dla stabilizacji

  // Ustaw czas spania i aktywuj Deep Sleep
  Serial.printf("Idę spać na %d sekund (%llu mikrosekund). Dobranoc!\n", TIME_TO_SLEEP, (unsigned long long)TIME_TO_SLEEP * uS_TO_S_FACTOR);
  ESP.deepSleep(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Kod po ESP.deepSleep() nie zostanie wykonany, ponieważ ESP zresetuje się.
  // Ta linia jest tylko na wypadek, gdyby deepSleep z jakiegoś powodu nie zadziałał.
  Serial.println("Ta wiadomość nie powinna się pojawić, jeśli Deep Sleep działa poprawnie.");
}

void loop() {
  // Pętla loop() nie zostanie wykonana po wywołaniu ESP.deepSleep() w setup(),
  // ponieważ ESP zresetuje się i rozpocznie ponownie od setup().
  // Nie ma tu potrzeby umieszczania żadnego kodu.
}