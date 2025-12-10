<?php
// Inclui a conexão externa
require_once "conexao.php";

// Recebe os dados do formulário
$nome      = $_POST['name'];
$sobrenome = $_POST['surname'];
$email     = $_POST['email'];
$senha     = $_POST['password'];
$confirma  = $_POST['confirmPassword'];

// Verifica se as senhas coincidem
if ($senha !== $confirma) {
    die("Erro: as senhas não coincidem!");
}

// Criptografa a senha antes de salvar
$senhaHash = password_hash($senha, PASSWORD_DEFAULT);

// Prepara a query (segura contra SQL Injection)
$stmt = $conn->prepare("INSERT INTO usuarios (nome, sobrenome, email, senha) VALUES (?, ?, ?, ?)");
$stmt->bind_param("ssss", $nome, $sobrenome, $email, $senhaHash);

// Executa
if ($stmt->execute()) {
    echo "Cadastro realizado com sucesso!";
} else {
    echo "Erro ao cadastrar: " . $stmt->error;
}

$stmt->close();
$conn->close();
?>
