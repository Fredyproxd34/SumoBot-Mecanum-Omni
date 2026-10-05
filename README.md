# SumoBot Mecanum Omnidireccional (4x IBT-2 Independientes)

![C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Arduino%20Mega%202560-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

Firmware de control autónomo para Robot de Sumo con tracción holonómica/omnidireccional mediante 4 ruedas Mecanum y 4 drivers IBT-2 independientes. Permite desplazamientos laterales, diagonales y rotación sobre su propio eje.

> **¿Buscas la versión diferencial?** Para la versión estándar de 2 canales IBT-2 en Arduino Uno/Nano, consulta el repositorio [Codigo-sumo](https://github.com/Fredyproxd34/Codigo-sumo).

## Especificaciones Hardware
- **Controlador:** Arduino Mega 2560 (requiere 8 pines PWM independientes)
- **Drivers de Motor:** 4x IBT-2 (H-Bridge 43A)
- **Ruedas:** 4x Mecanum (omnidireccionales)
- **Sensores de Distancia:** 3x HC-SR04 (Izquierda, Centro, Derecha)
- **Sensores de Línea:** 2x TCRT5000 (Izquierda, Derecha)

## Diagrama de Asignación de Motores

```text
       [Delantero Izq]   (^)   [Delantero Der]
            (DI)                  (DD)
                 \              /
                  +------------+
                  |  Robot     |
                  +------------+
                 /              \
            (TI)                  (TD)
        [Trasero Izq]          [Trasero Der]

## Asignación de Pines (Arduino Mega)

| Componente | Función | Pin Arduino | Tipo |
| :--- | :--- | :--- | :--- |
| **IBT-2 Izquierdo** | RPWM / LPWM | Pin 5 / Pin 6 | PWM |
| **IBT-2 Izquierdo** | R_EN / L_EN | Pin 7 / Pin 8 | Digital |
| **IBT-2 Derecho** | RPWM / LPWM | Pin 9 / Pin 10 | PWM |
| **IBT-2 Derecho** | R_EN / L_EN | Pin 3 / Pin 4 | Digital |
| **Ultrasonido Centro** | TRIG / ECHO | Pin 11 / Pin 12 | Digital |
| **Ultrasonido Izq** | TRIG / ECHO | Pin A0 / Pin A1 | Analógico / Digital |
| **Ultrasonido Der** | TRIG / ECHO | Pin A4 / Pin A5 | Analógico / Digital |
| **Sensores Línea** | IZQ / DER | Pin A3 / Pin A2 | Digital In |