<?php
if (isset($_GET['time'])) {
    $time = intval($_GET['time']);
    if ($time > 0 && $time <= 604800) { // np. od 1 sekndy do 1 tygodnia
        file_put_contents('sleep_time.txt', $time);
        echo "OK";
    } else {
        echo "Nieprawidłowa wartość!";
    }
} else {
    echo "Brak parametru time.";
}
?>
