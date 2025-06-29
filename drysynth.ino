#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "dht.h"
#include <math.h>

// ===== Configurações de hardware =====
LiquidCrystal_I2C lcd(0x27, 16, 2);
dht DHT;

// Pinos
#define BTN_TOGGLE 7
#define RELE_PIN   9
#define FAN_PIN    5
#define DHTPIN     A2

// Limites de histerese por modo
const float T_SECA_LIGA     = 37.0;
const float T_SECA_DESLIGA  = 45.0;
const float T_DORME_LIGA    = 28.0;
const float T_DORME_DESLIGA = 40.0;
const float T_VENT_LIGA     = 30.0;
const float T_VENT_DESLIGA  = 50.0;

// Modos de operação
enum Mode { SECAGEM, ECONOMIA, VENTILA };
Mode modoAtual = SECAGEM;

// Estado da lâmpada (começa acesa)
bool lampadaLigada = true;

// Debounce simples
bool botaoApertado(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(50);
    if (digitalRead(pin) == LOW) {
      while (digitalRead(pin) == LOW);
      return true;
    }
  }
  return false;
}

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();

  // Boas-vindas
  lcd.clear();
  lcd.setCursor(3,0); lcd.print("Bem Vindo!");
  lcd.setCursor(4,1); lcd.print("DrySynth");
  delay(3000);
  lcd.clear();

  // Configura pinos
  pinMode(RELE_PIN, OUTPUT);
  // HIGH = relé fechado = lâmpada ON
  digitalWrite(RELE_PIN, HIGH);

  pinMode(FAN_PIN, OUTPUT);
  analogWrite(FAN_PIN, 0);

  pinMode(BTN_TOGGLE, INPUT_PULLUP);
}

void loop() {
  // 1) alterna modo no botão
  if (botaoApertado(BTN_TOGGLE)) {
    modoAtual = Mode((modoAtual + 1) % 3);
    lcd.clear();
    switch (modoAtual) {
      case SECAGEM:  lcd.print("MODO: SECAGEM");  break;
      case ECONOMIA: lcd.print("MODO: ECONOMIA"); break;
      case VENTILA:  lcd.print("MODO: VENTILA");  break;
    }
    delay(3000);
    lcd.clear();
    // garante que a lâmpada volte a ser considerada acesa
    lampadaLigada = true;
    digitalWrite(RELE_PIN, HIGH);
  }

  // 2) leitura do DHT22
  DHT.read22(DHTPIN);
  float temp    = DHT.temperature;
  float umidRel = DHT.humidity;
  if (isnan(temp) || (modoAtual == SECAGEM && isnan(umidRel))) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Erro sensor");
    delay(2000);
    return;
  }

  // 3) cálculo da umidade absoluta
  float es      = 6.112 * exp((17.67 * temp) / (temp + 243.5));
  float e       = (umidRel / 100.0) * es;
  float umidAbs = (216.7 * e) / (temp + 273.15);

  // 4) controle da lâmpada (HIGH = ON) e do fan
  switch (modoAtual) {
    case SECAGEM:
      if (lampadaLigada && temp >= T_SECA_DESLIGA) {
        digitalWrite(RELE_PIN, LOW);
        lampadaLigada = false;
      }
      else if (!lampadaLigada && temp < T_SECA_LIGA) {
        digitalWrite(RELE_PIN, HIGH);
        lampadaLigada = true;
      }
      analogWrite(FAN_PIN, 180);
      break;

    case ECONOMIA:
      if (lampadaLigada && temp >= T_DORME_DESLIGA) {
        digitalWrite(RELE_PIN, LOW);
        lampadaLigada = false;
      }
      else if (!lampadaLigada && temp < T_DORME_LIGA) {
        digitalWrite(RELE_PIN, HIGH);
        lampadaLigada = true;
      }
      analogWrite(FAN_PIN, 80);
      break;

    case VENTILA:
      if (lampadaLigada && temp >= T_VENT_DESLIGA) {
        digitalWrite(RELE_PIN, LOW);
        lampadaLigada = false;
      }
      else if (!lampadaLigada && temp < T_VENT_LIGA) {
        digitalWrite(RELE_PIN, HIGH);
        lampadaLigada = true;
      }
      analogWrite(FAN_PIN, 255);
      break;
  }

  // 5) exibição no LCD
  static bool mostrarAbs = false;
  lcd.clear();
  lcd.setCursor(0,0);
  if (mostrarAbs) {
    lcd.print("Um.abs:");
    lcd.print(umidAbs, 1);
    lcd.print("g/m3");
  } else {
    lcd.print("Um.rel:");
    lcd.print(umidRel, 1);
    lcd.print("%");
  }
  mostrarAbs = !mostrarAbs;

  lcd.setCursor(0,1);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.write((uint8_t)223);
  lcd.print("C ");
  switch (modoAtual) {
    case SECAGEM:  lcd.print("SECA"); break;
    case ECONOMIA: lcd.print("ECO");  break;
    case VENTILA:  lcd.print("VENT"); break;
  }

  delay(2000);
}
