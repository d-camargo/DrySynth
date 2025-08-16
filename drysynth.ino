#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "dht.h"
#include <math.h>

// ===== Hardware =====
LiquidCrystal_I2C lcd(0x27, 16, 2);
dht DHT;

// Pinos
#define BTN_TOGGLE 7
#define RELE_PIN   9
#define FAN_PIN    5
#define DHTPIN     A2

// Limites (histerese) por modo
const float T_SECA_LIGA     = 35.0;
const float T_SECA_DESLIGA  = 50.0;
const float T_DORME_LIGA    = 28.0;
const float T_DORME_DESLIGA = 40.0;
const float T_VENT_LIGA     = 30.0;
const float T_VENT_DESLIGA  = 50.0;

// Fan quando lâmpada está DESLIGADA:
// use 0 para OFF total, ou um valor baixo (ex.: 35) para circulação mínima
const uint8_t FAN_IDLE_PWM  = 30;

// Filtro de salto máximo
const float  MAX_SALTO       = 3.0;   // °C
static float tempValida      = 0.0;
static bool  primeiraLeitura = true;

// Modos
enum Mode { SECAGEM, ECONOMIA, VENTILA };
Mode modoAtual = SECAGEM;

// Estado da lâmpada (HIGH = relé fechado = lâmpada ON)
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

// PWM linear do fan: Tmin=20°C, Tmax depende do modo
uint8_t calcFanSpeed(float temp, float tMax) {
  if (temp <= 20.0) return 0;
  if (temp >= tMax)  return 255;
  return (uint8_t)((temp - 20.0) * 255.0 / (tMax - 20.0));
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

  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, HIGH);   // HIGH = lâmpada ON (começa acesa)

  pinMode(FAN_PIN, OUTPUT);
  analogWrite(FAN_PIN, 0);

  pinMode(BTN_TOGGLE, INPUT_PULLUP);
}

void loop() {
  // Alterna de modo no botão
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

    // recomeça com lâmpada acesa e aceita a 1ª leitura crua
    lampadaLigada   = true;
    digitalWrite(RELE_PIN, HIGH);
    primeiraLeitura = true;
  }

  // Leitura + filtro de salto
  DHT.read22(DHTPIN);
  float tempRaw = DHT.temperature;
  float umidRel = DHT.humidity;

  if (!isnan(tempRaw)) {
    if (primeiraLeitura) {
      tempValida = tempRaw;
      primeiraLeitura = false;
    } else if (fabs(tempRaw - tempValida) <= MAX_SALTO) {
      tempValida = tempRaw;
    }
  }
  float temp = tempValida;

  if (isnan(temp) || (modoAtual == SECAGEM && isnan(umidRel))) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Erro sensor");
    delay(2000);
    return;
  }

  // Umidade absoluta (g/m3)
  float es      = 6.112 * exp((17.67 * temp)/(temp + 243.5));
  float e       = (umidRel/100.0) * es;
  float umidAbs = (216.7 * e)/(temp + 273.15);

  // Controle da lâmpada (HIGH = ON, LOW = OFF) por modo
  float tMax = T_SECA_DESLIGA;  // default
  float tMin = T_SECA_LIGA;
  switch (modoAtual) {
    case SECAGEM:
      tMin = T_SECA_LIGA;     tMax = T_SECA_DESLIGA;  break;
    case ECONOMIA:
      tMin = T_DORME_LIGA;    tMax = T_DORME_DESLIGA; break;
    case VENTILA:
      tMin = T_VENT_LIGA;     tMax = T_VENT_DESLIGA;  break;
  }

  if (lampadaLigada && temp >= tMax) {
    digitalWrite(RELE_PIN, LOW);
    lampadaLigada = false;
  } else if (!lampadaLigada && temp < tMin) {
    digitalWrite(RELE_PIN, HIGH);
    lampadaLigada = true;
  }

  // FAN: acelera só quando a lâmpada está ON; senão usa FAN_IDLE_PWM
  uint8_t fanPWM = lampadaLigada ? calcFanSpeed(temp, tMax) : FAN_IDLE_PWM;
  analogWrite(FAN_PIN, fanPWM);

  // Display
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
  lcd.write((uint8_t)223);  // °
  lcd.print("C ");
  switch (modoAtual) {
    case SECAGEM:  lcd.print("SECA"); break;
    case ECONOMIA: lcd.print("ECO");  break;
    case VENTILA:  lcd.print("VENT"); break;
  }

  delay(2000);
}
