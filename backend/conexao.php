<?php
$host = "localhost";     // endereço do servidor MySQL
$user = "root";          // usuário do MySQL
$pass = "";              // senha do MySQL
$db   = "estufa_db";      // nome do banco de dados

$conn = new mysqli($host, $user, $pass, $db);

// Verifica conexão
if ($conn->connect_error) {
    die("Erro na conexão: " . $conn->connect_error);
}
?>
