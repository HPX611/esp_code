#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

// OLED display settings
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1

// Function declarations
void oledSetup();
void oledLoop();
void updateDisplay();
void handleButtonInput();
void drawWiFiIcon(int x, int y, int signalStrength);
void drawFanIcon(int x, int y, bool isRunning);
void drawLightIcon(int x, int y, bool isOn);
void drawWeatherIcon(int x, int y, String weather);

#endif // OLED_DISPLAY_H