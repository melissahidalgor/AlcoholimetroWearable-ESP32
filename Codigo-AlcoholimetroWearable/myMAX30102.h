#ifndef MYMAX30102_H
#define MYMAX30102_H

// INCLUDES
#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

// ------------ DEFINES -------------
#define MAX_BRIGHTNESS 255
const byte RATE_SIZE = 4;

// ------------- FUNCTIONS ----------
void initMAX30102();
int leerPulso();
int32_t leerSpO2();

#endif /*myMAX30102.h*/
