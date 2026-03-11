#ifndef __WIFI_CONFIG_H
#define __WIFI_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// 配网状态
enum WiFiStatus {
    WIFI_NOT_CONFIGURED,
    WIFI_CONFIG_MODE,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_FAILED
};

// WiFi配置结构
struct WiFiConfig {
    char ssid[32];
    char password[64];
};

// 函数声明
void wifiSetup();
void wifiLoop();
bool isWiFiConnected();
String getWiFiSSID();
String getLocalIP();
void startConfigMode();
void stopConfigMode();
bool isInConfigMode();
void resetWiFiConfig();
void setLEDCallback(void (*callback)(int state));

#endif // !__WIFI_CONFIG_H