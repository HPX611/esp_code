#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Preferences.h>

// 时间更新间隔
const unsigned long NTP_UPDATE_INTERVAL = 5000;    // 5秒
const unsigned long NETWORK_CHECK_INTERVAL = 1000; // 1秒

// 时间和日期变量
extern int currentHour;
extern int currentMinute;
extern int currentSecond;
extern int currentYear;
extern int currentMonth;
extern int currentDay;

// 网络状态变量
extern bool networkConnected;

// 函数声明
void timeSetup();
void updateTime();
String getFormattedTime();

#endif // TIME_MANAGER_H
