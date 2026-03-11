#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>

// Weather API configuration - 使用心知天气
#define WEATHER_API_KEY "SssB7DwJzbSjrAcnH" // Replace with your actual Seniverse API key
#define WEATHER_UPDATE_INTERVAL 3600000 // 1 hour update interval

// Function declarations
void weatherSetup();
void weatherLoop();
void updateWeather();
void loadCityAddress();
void saveCityAddress();
void changeCityAddress(int direction);
String getWeatherDescription();
float getWeatherTemperature();
String getCurrentCity();

#endif // WEATHER_MANAGER_H
