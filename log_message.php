<?php
// Ustawienie strefy czasowej, aby data i czas były poprawne
date_default_timezone_set('Europe/Warsaw'); // Możesz zmienić na swoją strefę czasową

// 1. Wykrycie adresu IP klienta
$ip = $_SERVER['REMOTE_ADDR'];

// 2. Pobranie aktualnego czasu
$timestamp = date('Y-m-d H:i:s');

// 3. Odczytanie numeru operacji i nazwy nadawcy
$numer_operacji = isset($_GET['numer']) ? (int)$_GET['numer'] : 0; // Upewnij się, że to liczba
$nazwa = isset($_GET['nazwa']) ? trim($_GET['nazwa']) : 'Nieznany Nadawca';

// Zabezpieczenie przed pustą nazwą
if (empty($nazwa)) {
    $nazwa = 'Nieznany Nadawca';
}

// 4. Dekodowanie numeru operacji na wiadomość
$wiadomosc_dekodowana = '';
switch ($numer_operacji) {
    case 1:
        $wiadomosc_dekodowana = 'ESP8266 Uruchomione. Połączono z WiFi.';
        break; // ESP8266 uruchomione. Połączono z wifi, (Wiadomość jest opóźniana do połączenia z serwerem)
    case 2:
        $wiadomosc_dekodowana = 'Połączono z serwerem';
        break; // Połączono z serwerem
    case 3:
        $wiadomosc_dekodowana = 'Uruchamianie deep sleep';
        break; // Uruchamianie Deep sleep
    case 4:
        $wiadomosc_dekodowana = 'Uruchamianie power down sequence, Przekaźnik OFF';
        break;
    case 5:
        $wiadomosc_dekodowana = 'Uruchamianie ODROID M1, Przekaźnik ON';
        break;
    case 6:
        $wiadomosc_dekodowana = 'Restart ODORID M1, Przekaźnik --> OFF --> ON';
        break;
    case 7:
        $wiadomosc_dekodowana = 'Tworzenie access Point dla ESP 01S';
        break;
    case 8:
    	$wiadomosc_dekodowana = 'Nawiązano kontakt z ODROID M1 !';
        break;
    // Możesz dodać więcej przypadków
    default:
        $wiadomosc_dekodowana = 'Nieznana operacja (kod: ' . $numer_operacji . ')';
        break; //
}

// 5. Sformatowanie wpisu do logu
// Format: IP | NAZWA | CZAS OTRZYMANIA WIADOMOŚĆI | WIADOMOŚĆ
$log_entry = "$ip | $nazwa | $timestamp | $wiadomosc_dekodowana\n";

// 6. Zapisanie do pliku log.txt
// Używamy FILE_APPEND, aby dopisać na końcu pliku, i LOCK_EX, aby zapobiec problemom przy jednoczesnym zapisie
if (file_put_contents('log.txt', $log_entry, FILE_APPEND | LOCK_EX) !== false) {
    // Odpowiedź do klienta (opcjonalne, ale pomocne do debugowania)
    echo "Wiadomość zapisana pomyślnie.";
} else {
    header("HTTP/1.1 500 Internal Server Error");
    echo "Błąd zapisu do pliku log.txt. Sprawdź uprawnienia.";
}
?>
