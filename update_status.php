<?php
if (isset($_GET['status'])) {
    $status = $_GET['status'] === 'ON' ? 'ON' : 'OFF'; // tylko ON lub OFF
    file_put_contents('status.txt', $status);
}
?>
