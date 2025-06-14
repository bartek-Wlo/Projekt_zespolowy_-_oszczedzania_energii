# Opis plików umieszczonych w repozytorium:
- `ESP01.c` → Program wgrany do ESP 01S
- `ESP8266.c` → Program wgrany do ESP 8266 + NodeMCU
- `ESP_deepSleep_test.c` → Program testowy do sprawdzania czy płytka obsłguje poprawnie wchodzenie w deep sleep i wybudzanie się z niego.
- `ESP_wemos.c` → Program do wyłączenia płytki wemos, gdy pracuje jako zasilacz.
- `index.html` → kod strony na panamint
- `update_status.php` → skrypt do zmienia stanu w pliku status.txt
- `odroid_shutdown_server.py` → skrypt serwera HTTP postawionego na ODROID-M1
- `odroid_shutdown.service` → plik konfiguracyjny używany przez system Linux do zarządzania usługami na ODROID-M1


# Wstęp
Projekt miał na celu stworzenie systemu oszczędzania energii dla urządzeń takich jak: routery, Raspberry Pi, ODROID, (W projekcie użyto ODROID-M1), przy wykorzystaniu mikrokontrolera ESP32/ESP8266, sterującego przekaźnikiem.

## Spis użytch użądzeń 
| Urządzenie nazwa | Opis | Nazwa | Adres MAC |
|-|-|-|-|
| ESP 01S + Przekaźnik  | Malutki czarny ESP | "ESP-4AC41F" | 48:3f:da:4a:c4:1f |
| ESP 8266 + Wemos D1 R1 WiFi | Duży niebieski ESP | "ESP-2CDCDF" | c4:d8:d5:2c:dc:df |
| ESP 8266 + NodeMCU V3 z CH340 | Duży czarny ESP | "ESP-C8591D" | ec:fa:bc:c8:59:1d |
| ODROID-M1 | Mini komputer na zielonej płytce | "odroidm1" | 16:6b:12:d1:a7:91 |
| TL-WN725N | USB Wi-Fi adapter podłączony do ODROIDa | | 30:68:93:7b:30:7d |
| ESP-01S UART CH340 | Programator adapter USB do ESP-01S | | |


## Opis Cyklu działania:
1. Obudzenie się ESP 8266.
2. Połączenie się z WiFi,
3. Zweryfikowanie czy przyszła wiadomość ze strony nakazująca prace:   
  (1) ponowne uśpienie na ustalony okres ALBO (2) Kontynułowanie pracy:
4. ESP8266 rozkazuje ESP sterującym przekaźnikem na zwracie obowdu zasilania
5. Główne urządzenie u nas ODROID-M1 uruchamia się
6. Nawiązanie komunikacji ODROIDa z ESP8266
7. ODROID wykonuje zadanie
8. ODROID po wykonaniu zadania informuje o rozpoczęciu sekwencji wyłączania systemu
9. ESP8266 rozpoczyna sekwęcje power down, poprzez wysłanie polecenia power off do ODROID
  ESP wysyła polecenie do ODROIDa ponieważ jest tutaj zostawione miejsce na awaryjne włączanie systemu, np. w sytułacji stwierdzenia krytycznie niskiego poziomu baterii.
10. Po określonym czasie zasialanie do ODROIDa zostaje odcięte
11. ESP8266 usypia się na określony czas.


## Sterowanie systemu przez HTTP (HTTP control interface) za pomocą panaminta
Główny interfejs sterowania systemem, punkt z którego ESP8266 odbiera wiadomość o wykonaniu pracy albo ponownym uśpieniu
- https://panamint.kcir.pwr.edu.pl/~bwlodarc/PROJEKT/update_status.php?status=OFF
- https://panamint.kcir.pwr.edu.pl/~bwlodarc/PROJEKT/update_status.php?status=ON
Dotatkowy interface nadający logi o mówiące o statusie projektu:
- https://panamint.kcir.pwr.edu.pl/~bwlodarc/PROJEKT/log_message.php?numer=0&nazwa="TEST"
Gdzie `numer` to  numer operacji tłumaczony na konretny stan, `nazwa` to zazwa nadawcy logu, oprócz IP.
  
## Sterowanie systemu przez HTTP (HTTP control interface) w sieci lokalnej
Wyłącza ODROIDa, który ma postawiony serwer:
- http://192.168.55.83:5000/shutdown

Wysyłanie poleceń do ESP8266, który przekazuje je w pod sieci lokalnej (zawierającej tylko ESP-8266 i ESP-01S):
- http://192.168.55.224/turnRelayOffOnESP01 (Uruchamia całą sekwencje POWER DOWN, z czekaniem na wyłączenie ODROIDa)
- http://192.168.55.224/turnRelayOnOnESP01

# Podsumowanie, Uwagi
ESP 8266 + Wemos D1 R1 WiFi → nie obsługuje poprawnie trybu deep sleep, zespołowi nie udało się doprwadzić do systułacji w której sam się budzi.

## Wgrywanie kodu na ESP 01S
- Za pomocą ESP-01S UART CH340 jest dość problematyczne i wymaga posiadania kabli Arduino męsko-męskich.
- Należy zewrzeć piny `RST` + `TX` i przez cały okres wgrywania kodu do pokazania 100% należy je zwierać.
- Procedura:
1. Podłączyć programator do wejścia USB od PC z Arduino IDE.
2. Zwerzeć pny  `RST` + `TX`
3. Odłączyć programator od wejścia USB z PC (Warto mieć jakiś przedłużacz do tego kroku, bez niego może być niemożliwe cągłe trzmanie zwartych pinów)
4. Ponownie podłączyć programator.
5. Wgrać program
6. Zaczekać do 100%
7. Rozwerzeć piny
8. Ponownie rozłączyć programator
9. Doprowadzić zasialnie do ESP (ESP uruchamia się z nowym programem.)
- Kod był wgrywany na Board: Generic ESP8266 Module.

|3V3|RX|
|-|-|
|RST|GPIO0|
|EN|GPIO2|
|TX|GND|
