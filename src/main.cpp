#include <Arduino.h>

// ==========================================
// CONFIGURACIÓN DE PINES (Arduino Mega 2560)
// ==========================================

// Motor 1: Delantero Izquierdo (DI)
const uint8_t RPWM_DI = 2;
const uint8_t LPWM_DI = 3;
const uint8_t EN_DI   = 22;

// Motor 2: Delantero Derecho (DD)
const uint8_t RPWM_DD = 4;
const uint8_t LPWM_DD = 5;
const uint8_t EN_DD   = 23;

// Motor 3: Trasero Izquierdo (TI)
const uint8_t RPWM_TI = 6;
const uint8_t LPWM_TI = 7;
const uint8_t EN_TI   = 24;

// Motor 4: Trasero Derecho (TD)
const uint8_t RPWM_TD = 8;
const uint8_t LPWM_TD = 9;
const uint8_t EN_TD   = 25;

// Sensores de Línea (TCRT5000)
const uint8_t SENSOR_LINEA_IZQ = 30;
const uint8_t SENSOR_LINEA_DER = 31;

// Sensores Ultrasónicos (HC-SR04)
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
  DESPLAZAMIENTO_LATERAL,
  ESCAPANDO
};

EstadoRobot estadoActual = INICIO_RUSH;

unsigned long tiempoInicio = 0;
const unsigned long TIEMPO_ESPERA_REGLAMENTO = 5000; // 5 segundos
const int DISTANCIA_UMBRAL_CM = 45;

// ==========================================
// DECLARACIÓN DE FUNCIONES
// ==========================================
void controlarDriver(uint8_t rPwm, uint8_t lPwm, int velocidad);
void moverMecanum(int Vy, int Vx, int W);
long medirDistanciaCM(uint8_t trigPin, uint8_t echoPin);
bool detectarLinea();

void setup() {
  // Configurar pines de motores como salida
  pinMode(RPWM_DI, OUTPUT); pinMode(LPWM_DI, OUTPUT); pinMode(EN_DI, OUTPUT);
  pinMode(RPWM_DD, OUTPUT); pinMode(LPWM_DD, OUTPUT); pinMode(EN_DD, OUTPUT);
  pinMode(RPWM_TI, OUTPUT); pinMode(LPWM_TI, OUTPUT); pinMode(EN_TI, OUTPUT);
  pinMode(RPWM_TD, OUTPUT); pinMode(LPWM_TD, OUTPUT); pinMode(EN_TD, OUTPUT);

  // Habilitar drivers IBT-2
  digitalWrite(EN_DI, HIGH);
  digitalWrite(EN_DD, HIGH);
  digitalWrite(EN_TI, HIGH);
  digitalWrite(EN_TD, HIGH);

  // Configurar sensores
  pinMode(SENSOR_LINEA_IZQ, INPUT);
  pinMode(SENSOR_LINEA_DER, INPUT);

  pinMode(TRIG_IZQ, OUTPUT); pinMode(ECHO_IZQ, INPUT);
  pinMode(TRIG_CEN, OUTPUT); pinMode(ECHO_CEN, INPUT);
  pinMode(TRIG_DER, OUTPUT); pinMode(ECHO_DER, INPUT);

  tiempoInicio = millis();
}

void loop() {
  // 1. Prioridad Máxima: Detección de Línea Blanca
  if (detectarLinea() && estadoActual != ESCAPANDO) {
    estadoActual = ESCAPANDO;
  }

  // 2. Lógica de la Máquina de Estados
  switch (estadoActual) {

    case INICIO_RUSH:
      // Espera reglamentaria de 5 segundos antes de arrancar
      if (millis() - tiempoInicio >= TIEMPO_ESPERA_REGLAMENTO) {
        moverMecanum(255, 0, 0); // Rush frontal inicial
        delay(350);
        estadoActual = BUSCANDO;
      } else {
        moverMecanum(0, 0, 0);
      }
      break;

    case BUSCANDO: {
      long distCen = medirDistanciaCM(TRIG_CEN, ECHO_CEN);
      long distIzq = medirDistanciaCM(TRIG_IZQ, ECHO_IZQ);
      long distDer = medirDistanciaCM(TRIG_DER, ECHO_DER);

      if (distCen < DISTANCIA_UMBRAL_CM || distIzq < DISTANCIA_UMBRAL_CM || distDer < DISTANCIA_UMBRAL_CM) {
        estadoActual = ATACANDO;
      } else {
        // Giro suave de búsqueda
        moverMecanum(0, 0, 120);
      }
      break;
    }

    case ATACANDO: {
      long distCen = medirDistanciaCM(TRIG_CEN, ECHO_CEN);
      long distIzq = medirDistanciaCM(TRIG_IZQ, ECHO_IZQ);
      long distDer = medirDistanciaCM(TRIG_DER, ECHO_DER);

      if (distCen < DISTANCIA_UMBRAL_CM) {
        // Envestida directa a máxima velocidad
        moverMecanum(255, 0, 0);
      } else if (distIzq < DISTANCIA_UMBRAL_CM) {
        // Desplazamiento omnidireccional a la izquierda para alinear
        moverMecanum(180, -200, 0);
      } else if (distDer < DISTANCIA_UMBRAL_CM) {
        // Desplazamiento omnidireccional a la derecha para alinear
        moverMecanum(180, 200, 0);
      } else {
        // Pérdida de objetivo
        estadoActual = BUSCANDO;
      }
      break;
    }

    case ESCAPANDO:
      // Retroceso y desplazamiento lateral de emergencia
      moverMecanum(-255, 0, 0);
      delay(400);
      moverMecanum(0, 255, 150); // Esquiva omnidireccional
      delay(300);
      estadoActual = BUSCANDO;
      break;
  }
}

// ==========================================
// CINEMÁTICA INVERSA MECANUM
// ==========================================
// Vy: Avance/Retroceso (-255 a 255)
// Vx: Traslación Lateral Izq/Der (-255 a 255)
// W:  Rotación Horario/Antihorario (-255 a 255)
void moverMecanum(int Vy, int Vx, int W) {
  int vDI = Vy + Vx + W;
  int vDD = Vy - Vx - W;
  int vTI = Vy - Vx + W;
  int vTD = Vy + Vx - W;

  // Escalar valores para no rebasar el límite PWM de 255
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

// Control individual por cada IBT-2
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

// Lectura de Ultrasónico
long medirDistanciaCM(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duracion = pulseIn(echoPin, HIGH, 18000); // Timeout 18ms (~3m)
  if (duracion == 0) return 999;
  return duracion * 0.034 / 2;
}

// Lectura de Sensores de Línea
bool detectarLinea() {
  return (digitalRead(SENSOR_LINEA_IZQ) == HIGH || digitalRead(SENSOR_LINEA_DER) == HIGH);
}