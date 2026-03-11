#include "weather_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

String weatherDescription = "N/A";
float weatherTemperature = 0;
unsigned long lastWeatherUpdate = 0;

String cityNames[] = {"beijing", "shanghai", "guangzhou", "shenzhen", "chengdu", "hangzhou", "wuhan", "xian"};
String cityDisplayNames[] = {"北京", "上海", "广州", "深圳", "成都", "杭州", "武汉", "西安"};
int cityCount = sizeof(cityNames) / sizeof(cityNames[0]);
int currentCityIndex = 0;
String cityId = "xian";

Preferences cityPrefs;
const char* CITY_PREF_NAMESPACE = "city-config";
const char* CITY_PREF_KEY = "city";

void weatherSetup() {
    loadCityAddress();
}

void weatherLoop() {
    updateWeather();
}

void loadCityAddress() {
    Preferences wifiPrefs;
    wifiPrefs.begin("wifi-creds", false);
    String storedCity = wifiPrefs.getString("city", "");
    wifiPrefs.end();
    
    if (storedCity.length() == 0) {
        cityPrefs.begin(CITY_PREF_NAMESPACE, false);
        storedCity = cityPrefs.getString(CITY_PREF_KEY, "");
        cityPrefs.end();
    }
    
    if (storedCity.length() > 0) {
        storedCity.toLowerCase();
        
        bool cityFound = false;
        for (int i = 0; i < cityCount; i++) {
            if (cityDisplayNames[i] == storedCity) {
                cityId = cityNames[i];
                currentCityIndex = i;
                cityFound = true;
                break;
            }
        }
        
        if (!cityFound) {
            cityId = storedCity;
        }
    }
}

void saveCityAddress() {
    cityPrefs.begin(CITY_PREF_NAMESPACE, false);
    cityPrefs.putString(CITY_PREF_KEY, cityId);
    cityPrefs.end();
}

void changeCityAddress(int direction) {
    currentCityIndex += direction;
    if (currentCityIndex < 0) currentCityIndex = cityCount - 1;
    if (currentCityIndex >= cityCount) currentCityIndex = 0;
    
    cityId = cityNames[currentCityIndex];
    
    saveCityAddress();
    
    lastWeatherUpdate = 0;
    updateWeather();
}

void updateWeather() {
    if (WiFi.status() == WL_CONNECTED && millis() - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
        lastWeatherUpdate = millis();
        
        HTTPClient http;
        String url = "https://api.seniverse.com/v3/weather/now.json?key=" + String(WEATHER_API_KEY) + "&location=" + cityId + "&language=zh-Hans&unit=c";
        
        http.begin(url);
        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error && doc.containsKey("results")) {
                JsonArray results = doc["results"];
                if (results.size() > 0) {
                    JsonObject result = results[0];
                    if (result.containsKey("now")) {
                        JsonObject now = result["now"];
                        if (now.containsKey("temperature")) {
                            weatherTemperature = now["temperature"].as<float>();
                        }
                        if (now.containsKey("text")) {
                            weatherDescription = now["text"].as<String>();
                        }
                    }
                }
            }
        }
        
        http.end();
    }
}

String getWeatherDescription() {
    return weatherDescription;
}

float getWeatherTemperature() {
    return weatherTemperature;
}

String getCurrentCity() {
    return cityId;
}
