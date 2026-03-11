#ifndef SENSORS_H
#define SENSORS_H

// Sensor pin definitions
#define DHT11_PIN 4      // DHT11 sensor pin
#define PIR_PIN 13       // PIR motion sensor pin

// GY30 I2C address
#define GY30_ADDR 0x5C   // GY30 address with AD0 connected to VCC

// Function declarations
void sensorsSetup();
void sensorsLoop();
float getTemperature();
float getHumidity();
int getLightLevel();
bool isMotionDetected();

#endif // SENSORS_H