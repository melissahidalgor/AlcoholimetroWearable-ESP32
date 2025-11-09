#ifndef MYMQ3_H
#define MYMQ3_H

// INCLUDES
#include <Arduino.h>

// ------------ VARS AND DEFINES -------------
// Definiciones necesarias para el cálculo de R0
extern const int pin_MQ;       // GPIO al que está conectado el sensor MQ-3
const float VCC_VOLTAGE = 3.3; // Voltaje de alimentación de tu ESP32
const float RL = 10.0;         // Resistencia de carga en kOhms 

// Define your calibration points:
const float RSR0_at_0_mg_L = 1.0;  // Your clean air RSR0 ratio
const float RSR0_at_10_mg_L = 0.8; // Your RSR0 ratio at 10 mg/L saturation


// ------------- FUNCTIONS ----------
float map_float(float x, float in_min, float in_max, float out_min, float out_max);
void initMQ3();
float leerAlcohol();

#endif /*myMQ3.h*/
