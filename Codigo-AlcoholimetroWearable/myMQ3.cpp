#include "myMQ3.h"

extern int pin_MQ3;
float R0_clean_air = 0.0;      // Variable para almacenar la resistencia del sensor en aire limpio (R0)

//________________________________________________________________________________ floatMap()
float map_float(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//________________________________________________________________________________ initMQ3()
void initMQ3(){
    analogReadResolution(12); // Configura la resolución del ADC a 12 bits (0-4095)

    Serial.println("Iniciando calibración de R0...");
    Serial.println("Asegura que el sensor MQ-3 esté en aire limpio.");

    // --- CALIBRACIÓN DE R0 (Resistencia del sensor en aire limpio) ---
    float sum_adc_clean = 0;
    int num_readings = 200; // Tomamos más lecturas para un promedio más estable

    for (int x = 0; x < num_readings; x++) {
        sum_adc_clean += analogRead(pin_MQ);
        Serial.print("ADC en aire limpio: ");
        Serial.println(analogRead(pin_MQ));
        delay(50); // Pequeño retardo entre lecturas
    }
    float avg_adc_clean = sum_adc_clean / num_readings;
    float voltage_clean = avg_adc_clean * (VCC_VOLTAGE / 4095.0); // Convertimos el promedio a voltaje

    // Calcular R0 (resistencia del sensor en aire limpio)
    if (voltage_clean < 0.001) 
        voltage_clean = 0.001; // Umbral de seguridad
    R0_clean_air = RL * ((VCC_VOLTAGE / voltage_clean) - 1);

    Serial.print("Promedio ADC en aire limpio: ");
    Serial.print(avg_adc_clean);
    Serial.print(" | Voltaje en aire limpio: ");
    Serial.print(voltage_clean, 3);
    Serial.print("V | R0 calculado: ");
    Serial.print(R0_clean_air, 2);
    Serial.println(" kOhms");
    Serial.println("--- Calibración de R0 completada. ---");
}

//________________________________________________________________________________ leerAlcohol()
float leerAlcohol(){
  float current_adc, current_voltage, RS_current, RSR0_ratio, alcohol_mg_per_L;
  float sum_alcohol = 0;
  const int muestras = 150;

  for(int i=0; i<muestras; i++){
    current_adc = analogRead(pin_MQ);
    current_voltage = current_adc * (VCC_VOLTAGE / 4095.0);

    if (current_voltage < 0.001){
      current_voltage = 0.001;
      }
    RS_current = RL * ((VCC_VOLTAGE / current_voltage) - 1);
    RSR0_ratio = RS_current / R0_clean_air;

    alcohol_mg_per_L = map_float(RSR0_ratio, RSR0_at_0_mg_L, RSR0_at_10_mg_L, 0.0, 10.0);
    // Add boundary conditions to prevent readings outside your defined range
    if (alcohol_mg_per_L < 0.0) {
      alcohol_mg_per_L = 0.0; // Cannot have negative alcohol concentration
    }
    if (alcohol_mg_per_L > 10.0) {
      alcohol_mg_per_L = 10.0; // Sensor saturates at 10 mg/L
    }
    sum_alcohol += alcohol_mg_per_L;

    Serial.print("RS/R0: ");
    Serial.print(RSR0_ratio, 3);
    Serial.print(" | Alcohol estimado: ");
    Serial.print(alcohol_mg_per_L, 2);
    Serial.print(" mg/L");
    Serial.print("| ADC: ");
    Serial.println(current_adc);
    delay(5);
  }

  float avg_alcohol = sum_alcohol / muestras;

  Serial.print("RS/R0: ");
  Serial.print(RSR0_ratio, 3);
  Serial.print(" | Alcohol estimado: ");
  Serial.print(avg_alcohol, 2);
  Serial.println(" mg/L");
  
  return avg_alcohol;
}