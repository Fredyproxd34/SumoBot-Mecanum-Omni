#include <Arduino.h>

// ==========================================
// CONFIGURACIÓN DE PINES (Arduino Mega 2560)
// ==========================================
const uint8_t RPWM_DI = 2, LPWM_DI = 3, EN_DI = 22;
const uint8_t RPWM_DD = 4, LPWM_DD = 5, EN_DD = 23;
const uint8_t RPWM_TI = 6, LPWM_TI = 7, EN_TI = 24;
const uint8_t RPWM_TD = 8, LPWM_TD = 9, EN_TD = 25;

const uint8_t SENSOR_LINEA_IZQ = 30;
const uint8_t SENSOR_LINEA_DER = 31;

const uint8_t TRIG_IZQ = 32, ECHO_IZQ = 33;
const uint8_t TRIG_CEN = 34, ECHO_CEN = 35;
const uint8_t TRIG_DER = 36, ECHO_DER = 37;

// ==========================================
// PARÁMETROS Y MÁQUINA DE ESTADOS
// ==========================================
enum EstadoRobot {
  INICIO_RUSH,
  BUSCANDO,
  ATACANDO,
  ESCAPANDO
};

EstadoRobot estadoActual = INICIO_RUSH;

unsigned long tiempoInicio = 0;
unsigned long tiempoEstado = 0; // Para temporizadores no bloqueantes
const unsigned long TIEMPO_ESPERA_REGLAMENTO = 5000;
const int DISTANCIA_UMBRAL_CM = 45;

// Variables globales para sensores
long distCen = 999, distIzq = 999, distDer = 999;

// Declaración de funciones
void controlarDriver(uint8_t rPwm, uint8_t lPwm, int velocidad);
void moverMecanum(int Vy, int Vx, int W);
long medirDistanciaCM(uint8_t trigPin, uint8_t echoPin);
bool detectarLinea();
void actualizarSensores();

void setup() {
  pinMode(RPWM_DI, OUTPUT); pinMode(LPWM_DI, OUTPUT); pinMode(EN_DI, OUTPUT);
  pinMode(RPWM_DD, OUTPUT); pinMode(LPWM_DD, OUTPUT); pinMode(EN_DD, OUTPUT);
  pinMode(RPWM_TI, OUTPUT); pinMode(LPWM_TI, OUTPUT); pinMode(EN_TI, OUTPUT);
  pinMode(RPWM_TD, OUTPUT); pinMode(LPWM_TD, OUTPUT); pinMode(EN_TD, OUTPUT);

  digitalWrite(EN_DI, HIGH);
  digitalWrite(EN_DD, HIGH);
  digitalWrite(EN_TI, HIGH);
  digitalWrite(EN_TD, HIGH);

  pinMode(SENSOR_LINEA_IZQ, INPUT);
  pinMode(SENSOR_LINEA_DER, INPUT);

  pinMode(TRIG_IZQ, OUTPUT); pinMode(ECHO_IZQ, INPUT);
  pinMode(TRIG_CEN, OUTPUT); pinMode(ECHO_CEN, INPUT);
  pinMode(TRIG_DER, OUTPUT); pinMode(ECHO_DER, INPUT);

  tiempoInicio = millis();
}

void loop() {
  // 1. Prioridad Máxima: Detección de Línea
  if (detectarLinea() && estadoActual != ESCAPANDO) {
    estadoActual = ESCAPANDO;
    tiempoEstado = millis(); // Registrar inicio del escape
  }

  // 2. Máquina de Estados
  switch (estadoActual) {

    case INICIO_RUSH:
      if (millis() - tiempoInicio >= TIEMPO_ESPERA_REGLAMENTO) {
        moverMecanum(255, 0, 0);
        delay(350); // Rush inicial
        estadoActual = BUSCANDO;
      } else {
        moverMecanum(0, 0, 0);
      }
      break;

    case BUSCANDO:
      actualizarSensores();
      if (distCen < DISTANCIA_UMBRAL_CM || distIzq < DISTANCIA_UMBRAL_CM || distDer < DISTANCIA_UMBRAL_CM) {
        estadoActual = ATACANDO;
      } else {
        moverMecanum(0, 0, 120); // Giro de búsqueda
      }
      break;

    case ATACANDO:
      actualizarSensores();
      if (distCen < DISTANCIA_UMBRAL_CM) {
        moverMecanum(255, 0, 0); // Embestida frontal
      } else if (distIzq < DISTANCIA_UMBRAL_CM) {
        moverMecanum(180, -200, 0); // Flanqueo izquierda
      } else if (distDer < DISTANCIA_UMBRAL_CM) {
        moverMecanum(180, 200, 0); // Flanqueo derecha
      } else {
        estadoActual = BUSCANDO; // Pérdida de objetivo
      }
      break;

    case ESCAPANDO: {
      unsigned long transcurrido = millis() - tiempoEstado;
      if (transcurrido < 350) {
        moverMecanum(-255, 0, 0); // Retroceso directo no bloqueante
      } else if (transcurrido < 650) {
        moverMecanum(0, 255, 150); // Esquiva y giro
      } else {
        estadoActual = BUSCANDO; // Fin del escape
      }
      break;
    }
  }
}

void actualizarSensores() {
  distCen = medirDistanciaCM(TRIG_CEN, ECHO_CEN);
  distIzq = medirDistanciaCM(TRIG_IZQ, ECHO_IZQ);
  distDer = medirDistanciaCM(TRIG_DER, ECHO_DER);
}

void moverMecanum(int Vy, int Vx, int W) {
  int vDI = Vy + Vx + W;
  int vDD = Vy - Vx - W;
  int vTI = Vy - Vx + W;
  int vTD = Vy + Vx - W;

  int maxVal = max(abs(vDI), max(abs(vDD), max(abs(vTI), abs(vTD))));
  if (maxVal > 255) {
    vDI = (vDI * 255) / maxVal;
    vDD = (vDD * 255) / maxVal;
    vTI = (vTI * 255) / maxVal;
    vTD = (vTD * 255) / maxVal;
  }

  controlarDriver(RPWM_DI, LPWM_DI, vDI);
  controlarDriver(RPWM_DD, LPWM_DD, vDD);
  controlarDriver(RPWM_TI, LPWM_TI, vTI);
  controlarDriver(RPWM_TD, LPWM_TD, vTD);
}

void controlarDriver(uint8_t rPwm, uint8_t lPwm, int velocidad) {
  velocidad = constrain(velocidad, -255, 255);
  if (velocidad >= 0) {
    analogWrite(rPwm, velocidad);
    analogWrite(lPwm, 0);
  } else {
    analogWrite(rPwm, 0);
    analogWrite(lPwm, abs(velocidad));
  }
}

long medirDistanciaCM(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duracion = pulseIn(echoPin, HIGH, 18000);
  if (duracion == 0) return 999;
  return duracion * 0.034 / 2;
}

bool detectarLinea() {
  return (digitalRead(SENSOR_LINEA_IZQ) == HIGH || digitalRead(SENSOR_LINEA_DER) == HIGH);
}
