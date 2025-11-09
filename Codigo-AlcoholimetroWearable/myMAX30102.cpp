#include "myMAX30102.h"

extern const int pin_SDA = 25;
extern const int pin_SCL = 26;

MAX30105 particleSensor;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

uint32_t irBuffer[100]; // infrared LED sensor data
uint32_t redBuffer[100]; // red LED sensor data
int32_t bufferLength; //data length

//________________________________________________________________________________ initMAX30102()
void initMAX30102()
{
  Wire.begin(pin_SDA, pin_SCL);
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 sensor not found. Check wiring.o");
    while (1);
  }

  Serial.println("MAX30105 sensor initialized.");
  particleSensor.setup(); //Configure sensor with these settings
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);

  // Read the first 100 samples, and determine the signal range
  bufferLength = 100; // Buffer length of 100 stores 4 seconds of samples running at 25sps
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample
  }
}

//________________________________________________________________________________ leerPulso()
// ------------ Función para leer BPM -------------
int leerPulso() {
  int beatAvg;
  for(int i=0; i<350; i++)
  {
    long irValue = particleSensor.getIR();
    float beatsPerMinute;
    bool isFingerDetected = false; // To track if a finger is on the sensor

    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          beatAvg += rates[x];
        }
        beatAvg /= RATE_SIZE;
        isFingerDetected = true;
      }
    } 

    /*Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.print(beatAvg);*/
    if (irValue < 5000) { // Adjust this threshold if needed
        isFingerDetected = false;
       // Serial.print(" No finger?");
    }
    if(beatAvg > 220){
      beatAvg = 0;
      Serial.print("Fuera de rango");
    }  
  }
   
  Serial.print("Avg BPM=");
  Serial.println(beatAvg);
  return beatAvg;
}

//________________________________________________________________________________ leerSpO2())
int32_t leerSpO2() {
  int32_t spo2; //SPO2 value
  for(int i=0; i<4; i++){
    int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
    int32_t heartRate; //heart rate value
    int8_t validHeartRate; //indicator to show if the heart rate calculation is valid
    //Dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //Take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); // We're finished with this sample so move to next sample
    }
  /* Serial.print(F(", HR="));
      Serial.print(heartRate, DEC);
      Serial.print(F(", HRvalid="));
      Serial.print(validHeartRate, DEC);
      Serial.print(F(", SPO2="));
      Serial.print(spo2, DEC);
      Serial.print(F(", SPO2Valid="));
      Serial.print(validSPO2, DEC);
    */

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);  
  }

  Serial.print("SPO2=");
  Serial.println(spo2, DEC);

  return spo2;
}