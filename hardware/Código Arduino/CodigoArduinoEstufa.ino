#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <DHT.h>

using namespace websockets;

// ==== Configuração WiFi e WebSocket ====
const char* ssid = "Estufa";
const char* password = "Estufaifprivp";
const char* websocketServer = "ws://192.168.39.29:81";

WebsocketsClient socket;
bool connected = false;

// ==== Configuração do Sensor DHT22 ====
#define DHTPIN 10
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float humidity = 0.0;

// Pino (A4)
#define LDR A4

// ==== LED e Botão ====
const int ledPin = 12;
bool ledState = false;
const int buttonPin = 0;

// ==== Relés ====
const int relePins[4] = {8, 7, 5, 6};
bool releStates[4] = {false};

// ==== Sensor de Umidade do Solo ====
const int soilPin = A0; // Conectado em A0

// ==== Modo de Operação ====
bool manualMode = false;

// ==== Setup ====
void setup() {
  Serial.begin(115200);

  // Inicializa sensores e pinos
  dht.begin();
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  for (int i = 0; i < 4; i++) {
    pinMode(relePins[i], OUTPUT);
    digitalWrite(relePins[i], HIGH); // Relés começam desligados
  }

  // Conexões
  connectWiFi();
  connectToWebSocket();

  socket.onMessage(handleMessage);
  socket.onEvent(handleEvent);
}

// ==== Loop ====
void loop() {
  if (!connected) {
    Serial.println("Tentando reconectar ao servidor WebSocket...");
    connectToWebSocket();
  }

  socket.poll();

  // Leitura do DHT22
  String dhtData = loadDHT22Data();
  socket.send(dhtData + ":dht:localhost:esp");

  // Leitura do LDR
  String ldrData = loadLDR();
  socket.send(ldrData + ":ldr:localhost:esp");

  // Leitura do sensor de solo
  String soilData = loadSoilData();
  socket.send(soilData + ":soil:localhost:esp");

  // Aplicar lógica de automação se não estiver no modo manual
  if (!manualMode) {
    applyAutomationLogic();
  }

  delay(1000);

  // Controle do LED via botão local
  if (digitalRead(buttonPin) == LOW) {
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    socket.send(String(ledState) + ":led:localhost:esp");
    delay(200);
  }
}

// ==== Aplicar lógica de automação ====
void applyAutomationLogic() {
  // Controlar relés baseado nos sensores
  // Exemplo: Se umidade do solo baixa, ligar relé 1 (irrigação)
  int soilValue = analogRead(soilPin);
  int soilPercent = map(soilValue, 4095, 0, 0, 100);
  
  if (soilPercent < 30) {
    digitalWrite(relePins[0], LOW);   // Liga
    releStates[0] = true;
    socket.send("1:rele3:localhost:esp");
  } else {
    digitalWrite(relePins[0], HIGH);  // Desliga
    releStates[0] = false;
    socket.send("0:rele3:localhost:esp");
  }
  
  // Exemplo: Se temperatura alta, ligar relé 2 (ventilação)
  if (temperature > 29) {
    digitalWrite(relePins[1], LOW);   // Liga
    releStates[1] = true;
    socket.send("1:rele1:localhost:esp");
  } else {
    digitalWrite(relePins[1], HIGH);  // Desliga
    releStates[1] = false;
    socket.send("0:rele1:localhost:esp");
  }
  
  // Exemplo: Se luz baixa, ligar relé 3 (iluminação)
  int lightValue = analogRead(LDR);
  int lightPercent = lightValue / 40.95;
  
  if (lightPercent < 40) {
    digitalWrite(relePins[2], LOW);   // Liga
    releStates[2] = true;
    socket.send("1:rele2:localhost:esp");
  } else {
    digitalWrite(relePins[2], HIGH);  // Desliga
    releStates[2] = false;
    socket.send("0:rele2:localhost:esp");
  }
}

// ==== Manipula mensagens do WebSocket ====
void handleMessage(WebsocketsMessage message) {
  Serial.print("Mensagem recebida: ");
  Serial.println(message.data());

  String data = message.data();
  
  // Verificar se é comando de modo
  if (data == "mode:manual") {
    manualMode = true;
    Serial.println("Modo manual ativado");
    return;
  } 
  else if (data == "mode:auto") {
    manualMode = false;
    Serial.println("Modo automático ativado");
    return;
  }
  
  // Processar outros comandos apenas se estiver no modo manual
  if (manualMode) {
    String status = parseData(data, 1);
    String sensor = parseData(data, 2);

    if (sensor == "led") {
      ledState = status.toInt();
      digitalWrite(ledPin, ledState);
    } 
    else if (sensor.startsWith("rele")) {
      int releIndex = sensor.substring(4).toInt(); // rele1 → 1
      if (releIndex >= 1 && releIndex <= 4) {
        int idx = releIndex - 1;
        releStates[idx] = status.toInt();
        digitalWrite(relePins[idx], releStates[idx] ? LOW : HIGH); // Ajuste lógica
      }
    }
  }
}

// ==== Eventos do WebSocket ====
void handleEvent(WebsocketsEvent event, WSInterfaceString data) {
  switch (event) {
    case WebsocketsEvent::ConnectionOpened:
      Serial.println("WebSocket conectado.");
      socket.send("Hello, server!");
      connected = true;
      break;
    case WebsocketsEvent::ConnectionClosed:
      Serial.println("WebSocket desconectado.");
      connected = false;
      break;
    default:
      break;
  }
}

// ==== Conecta ao WebSocket ====
void connectToWebSocket() {
  connected = socket.connect(websocketServer);
  if (connected) Serial.println("Conectado ao WebSocket!");
  else Serial.println("Falha na conexão com o WebSocket.");
}

// ==== Conecta ao WiFi ====
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao WiFi. IP: " + WiFi.localIP().toString());
}

// ==== Lê dados do DHT22 ====
String loadDHT22Data() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Falha na leitura do DHT22!");
    return "0,0";
  }

  return String(temperature) + "," + String(humidity);
}

// ==== Lê dados do Sensor de Umidade do Solo ====
String loadSoilData() {
  int leitura = analogRead(soilPin); // 0 a 4095
  int umidadePercentual = map(leitura, 4095, 0, 0, 100);

  Serial.print("Solo - Bruto: ");
  Serial.print(leitura);
  Serial.print(" | Umidade: ");
  Serial.print(umidadePercentual);
  Serial.println("%");

  return String(umidadePercentual);
}

String loadLDR() {
  float leitura = analogRead(LDR) / 40.95; // Coloque GND -> 10K resistor -> OUT -> LDR -> 3.3V

  Serial.print("LDR: ");
  Serial.print(leitura);
  Serial.println("%");

  return String(leitura);
}

// ==== Função auxiliar para dividir mensagens ====
String parseData(String data, int index) {
  String result = "";
  int currentIndex = 1;
  int start = 0;
  int end = data.indexOf(":");

  while (end != -1 && currentIndex <= index) {
    if (currentIndex == index) {
      result = data.substring(start, end);
      break;
    }
    start = end + 1;
    end = data.indexOf(":", start);
    currentIndex++;
  }

  return result;
}