#include "sensors.h"
#include <DHT.h>
#include <Wire.h>

DHT dht(DHT11_PIN, DHT11);

float temperature = 0.0;
float humidity = 0.0;
int lightLevel = 0;
bool motionDetected = false;
bool lastMotionState = false;

unsigned long lastDHTReadTime = 0;
unsigned long lastLightReadTime = 0;
unsigned long lastMotionDetectedTime = 0;

const unsigned long MOTION_TIMEOUT = 60000; // 30秒的超时时间，可根据实际需要调整

const unsigned long DHT_READ_INTERVAL = 2000;
const unsigned long LIGHT_READ_INTERVAL = 1000;

void initGY30() {
    Wire.begin(21, 22);
    Wire.beginTransmission(GY30_ADDR);
    Wire.write(0x10);
    Wire.endTransmission();
    delay(100);
}

int readGY30() {
    Wire.beginTransmission(GY30_ADDR);
    Wire.write(0x10);
    byte error = Wire.endTransmission();
    
    if (error != 0) {
        return 0;
    }
    
    delay(120);
    
    Wire.requestFrom(GY30_ADDR, 2);
    
    if (Wire.available() == 2) {
        uint16_t rawValue = Wire.read() << 8;
        rawValue |= Wire.read();
        int lux = rawValue / 1.2;
        return lux;
    } else {
        return 0;
    }
}

void sensorsSetup() {
    dht.begin();
    initGY30();
    pinMode(PIR_PIN, INPUT);
}

void sensorsLoop() {
    if (millis() - lastDHTReadTime >= DHT_READ_INTERVAL) {
        lastDHTReadTime = millis();
        
        float t = dht.readTemperature();
        float h = dht.readHumidity();
        
        if (!isnan(t) && !isnan(h)) {
            temperature = t;
            humidity = h;
        }
    }
    
    if (millis() - lastLightReadTime >= LIGHT_READ_INTERVAL) {
        lastLightReadTime = millis();
        lightLevel = readGY30();
    }
    
    bool currentMotionState = digitalRead(PIR_PIN);
    
    if (currentMotionState != lastMotionState) {
        lastMotionState = currentMotionState;
        if (currentMotionState == HIGH) {
            lastMotionDetectedTime = millis();
            motionDetected = true;
        }
    }
    
    // 检查是否在超时时间内，确保静止时也能检测到人体存在
    if (millis() - lastMotionDetectedTime <= MOTION_TIMEOUT) {
        motionDetected = true;
    } else {
        motionDetected = false;
    }
}

float getTemperature() {
    return temperature;
}

float getHumidity() {
    return humidity;
}

int getLightLevel() {
    return lightLevel;
}

bool isMotionDetected() {
    return motionDetected;
}
