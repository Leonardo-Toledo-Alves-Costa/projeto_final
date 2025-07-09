#include <Arduino.h>
#include <ESP32Servo.h>  // Biblioteca para o servo motor

// --- Pinos ---
#define LED_PIN    5
#define LED_ALR    4
#define TRIG_PIN   33
#define ECHO_PIN   32
#define BTN_PIN    19
#define POT_PIN    34
#define PIN_SERVO  13  // Pino do servo

// Ponte H
#define PIN_ENA    14
#define PIN_IN1    25
#define PIN_IN2    26

// --- Variáveis ---
const String SENHA_CORRETA = "1234";
String entradaUsuario = "";
int SENHA_ST = 0;
bool senhaProcessada = false;
bool BTN_ST = false;
bool btnProcessado = false;
bool acessoLiberado = false;
bool saidaLiberada = false;
bool PRES = false;

float distancia = 400;
unsigned long ultimaMedicao = 0;
const unsigned long INTERVALO_MEDICAO = 100;
bool BTN_msg = false;

bool arCondLigado = false;

// Servo
Servo servoPorta;

// --- Subrotinas ---
void SENHA();
void MED();
void CHK_PRES();
void Alarme();
void CHK_BTN();
void controleArCondicionado();
void abrirPortaComServo();

void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_ALR, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BTN_PIN, INPUT);
  pinMode(POT_PIN, INPUT);

  // Ponte H
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_ALR, LOW);
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, 0); // motor desligado

  // Servo motor
  servoPorta.attach(PIN_SERVO);
  servoPorta.write(0); // Porta fechada inicialmente

  Serial.println("Sistema iniciado.");
  Serial.println("Digite a senha e pressione Enter:");
}

void loop() {
  SENHA();
  MED();
  CHK_PRES();
  CHK_BTN();
  controleArCondicionado();

  if (SENHA_ST == 1 && !senhaProcessada) {
    Serial.println("✅ Senha correta! Aguardando entrada...");
    senhaProcessada = true;
  }

  if (SENHA_ST == -1 && !senhaProcessada) {
    Serial.println("❌ Senha incorreta.");
    digitalWrite(LED_PIN, LOW);
    senhaProcessada = true;
  }

  if (PRES && SENHA_ST != 1 && !saidaLiberada) {
    Alarme();
    Serial.println("🚨 Invasor detectado! Alarme acionado.");
  }

  if (BTN_ST && acessoLiberado && !btnProcessado) {
    Serial.println("🔓 Saída autorizada, abrindo porta...");
    digitalWrite(LED_PIN, HIGH);
    btnProcessado = true;

    abrirPortaComServo(); // Porta abre ao sair

    saidaLiberada = true;
    BTN_ST = false;
    acessoLiberado = false;
    SENHA_ST = 0;
    senhaProcessada = false;
  }
}

void SENHA() {
  while (Serial.available()) {
    char caractere = Serial.read();
    if (caractere == '\n' || caractere == '\r') {
      entradaUsuario.trim();
      SENHA_ST = (entradaUsuario == SENHA_CORRETA) ? 1 : -1;
      senhaProcessada = false;
      entradaUsuario = "";
      Serial.println("Digite a senha:");
    } else {
      entradaUsuario += caractere;
    }
  }
}

void MED() {
  static float leituras[3] = {400, 400, 400};
  static int indice = 0;

  if (millis() - ultimaMedicao >= INTERVALO_MEDICAO) {
    ultimaMedicao = millis();

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duracao = pulseIn(ECHO_PIN, HIGH, 20000);
    float novaLeitura = (duracao > 0) ? duracao * 0.0343 / 2 : 400;

    leituras[indice] = novaLeitura;
    indice = (indice + 1) % 3;

    distancia = (leituras[0] + leituras[1] + leituras[2]) / 3.0;
  }
}

void CHK_PRES() {
  static bool jaEntrouComSenha = false;

  if (distancia <= 5 && !PRES) {
    PRES = true;
    Serial.println("👤 Presença detectada.");

    if (SENHA_ST == 1) {
      Serial.println("✅ Entrada autorizada. Luz acesa.");
      digitalWrite(LED_PIN, HIGH);
      acessoLiberado = true;
      jaEntrouComSenha = true;

      abrirPortaComServo(); // Porta abre ao entrar
    }

  } else if (distancia > 15 && PRES) {
    PRES = false;
    Serial.println("🔍 Nenhuma presença detectada.");
    digitalWrite(LED_PIN, LOW);
    digitalWrite(LED_ALR, LOW);
    delay(2000);

    if (jaEntrouComSenha || saidaLiberada) {
      Serial.println("🔄 Pessoa saiu. Sistema reiniciado.");
      BTN_ST = false;
      BTN_msg = false;
      btnProcessado = false;
      jaEntrouComSenha = false;
      saidaLiberada = false;

      Serial.println("Sistema iniciado.");
      Serial.println("Digite a senha e pressione Enter:");
    }
  }
}

void Alarme() {
  digitalWrite(LED_ALR, HIGH);
  delay(100);
  digitalWrite(LED_ALR, LOW);
  delay(100);
}

void CHK_BTN() {
  bool estadoAtual = digitalRead(BTN_PIN);
  if (estadoAtual == HIGH) {
    BTN_ST = true;
    if (BTN_ST == true && estadoAtual == LOW && BTN_msg == false) {
      Serial.println("🟢 Botão pressionado!");
      BTN_msg = true;
    }
  }
}

void controleArCondicionado() {
  int valorPot = analogRead(POT_PIN);              // 0 a 4095
  int temperatura = map(valorPot, 0, 4095, 0, 55);  // Simula temperatura de 0 a 55 °C

  if (temperatura >= 25 && !arCondLigado) {
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
    analogWrite(PIN_ENA, 255); // Liga motor
    Serial.println("❄ Ar-condicionado ligado.");
    arCondLigado = true;
  }

  if (temperatura < 25 && arCondLigado) {
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
    analogWrite(PIN_ENA, 0); // Desliga motor
    Serial.println("🛑 Ar-condicionado desligado.");
    arCondLigado = false;
  }
}

void abrirPortaComServo() {
  servoPorta.write(90);   // Abre porta
  delay(3000);            // Espera 3 segundos
  servoPorta.write(0);    // Fecha porta
}
