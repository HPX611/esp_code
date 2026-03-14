#include "oled_display.h"
#include <sensors.h>
#include <slave_comm.h>
#include <WiFi.h>
#include "../time/time_manager.h"
#include "../icon/icon_manager.h"

// OLED display object
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// Last display update times
unsigned long lastTimeUpdateTime = 0;      // Last time update
unsigned long lastStatusUpdateTime = 0;     // Last status update

// Display update intervals
const unsigned long TIME_UPDATE_INTERVAL = 1000; // 1 second for time
const unsigned long STATUS_UPDATE_INTERVAL = 1000; // 1 second for status



void oledSetup() {
    // Initialize I2C
    // Wire.begin(21, 22); // SDA: 21, SCL: 22
    
    // Initialize OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Don't proceed, loop forever
    }
    
    // Clear display
    display.clearDisplay();
    display.display();
    
    // Set text settings
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // 显示初始化信息
    display.setCursor(0, 0);
    display.println("OLED初始化成功");
    display.println("系统启动中...");
    display.display();
    delay(2000);
    
    Serial.println("OLED display initialized");
}

// 更新时间显示
void updateTimeDisplay() {
    // 仅清除时间区域
    display.fillRect(0, 0, OLED_WIDTH, 32, SSD1306_BLACK);
    
    // ------------------------------
    // First line: Time
    // ------------------------------
    display.setTextSize(2);
    String timeStr = "";
    if (currentHour < 10) timeStr += "0";
    timeStr += String(currentHour) + ":";
    if (currentMinute < 10) timeStr += "0";
    timeStr += String(currentMinute);
    if (currentSecond < 10) timeStr += ":0";
    else timeStr += ":";
    timeStr += String(currentSecond);
    
    // Calculate center position for time
    int timeWidth = timeStr.length() * 12; // Approximate width for size 2 font
    int timeX = (OLED_WIDTH - timeWidth) / 2;
    int timeY = 0;
    
    display.setCursor(timeX, timeY);
    display.print(timeStr);
    
    // ------------------------------
    // Second line: Date
    // ------------------------------
    display.setTextSize(1);
    String dateStr = "";
    if (currentYear > 0) {
        dateStr += String(currentYear) + "/";
        if (currentMonth < 10) dateStr += "0";
        dateStr += String(currentMonth) + "/";
        if (currentDay < 10) dateStr += "0";
        dateStr += String(currentDay);
    } else {
        dateStr = "2024/01/01";
    }
    
    int dateWidth = dateStr.length() * 6;
    int dateX = (OLED_WIDTH - dateWidth) / 2;
    int dateY = timeY + 20;
    
    display.setCursor(dateX, dateY);
    display.print(dateStr);
    
    // 更新显示
    display.display();
}

void oledLoop() {
    // Update display based on different intervals
    unsigned long currentMillis = millis();
    
    // Update time every second
    if (currentMillis - lastTimeUpdateTime >= TIME_UPDATE_INTERVAL) {
        lastTimeUpdateTime = currentMillis;
        updateTimeDisplay();
    }
    
    // Update full status every 5 seconds
    if (currentMillis - lastStatusUpdateTime >= STATUS_UPDATE_INTERVAL) {
        lastStatusUpdateTime = currentMillis;
        try {
            updateDisplay();
        } catch (...) {
        }
    }
}













void updateDisplay() {
    // Clear display
    display.clearDisplay();
    
    // ------------------------------
    // First line: Time
    // ------------------------------
    display.setTextSize(2);
    String timeStr = "";
    if (currentHour < 10) timeStr += "0";
    timeStr += String(currentHour) + ":";
    if (currentMinute < 10) timeStr += "0";
    timeStr += String(currentMinute);
    if (currentSecond < 10) timeStr += ":0";
    else timeStr += ":";
    timeStr += String(currentSecond);
    
    // Calculate center position for time
    int timeWidth = timeStr.length() * 12; // Approximate width for size 2 font
    int timeX = (OLED_WIDTH - timeWidth) / 2;
    int timeY = 0;
    
    display.setCursor(timeX, timeY);
    display.print(timeStr);
    
    // ------------------------------
    // Second line: Date
    // ------------------------------
    display.setTextSize(1);
    String dateStr = "";
    if (currentYear > 0) {
        dateStr += String(currentYear) + "/";
        if (currentMonth < 10) dateStr += "0";
        dateStr += String(currentMonth) + "/";
        if (currentDay < 10) dateStr += "0";
        dateStr += String(currentDay);
    } else {
        dateStr = "2024/01/01";
    }
    
    int dateWidth = dateStr.length() * 6;
    int dateX = (OLED_WIDTH - dateWidth) / 2;
    int dateY = timeY + 20;
    
    display.setCursor(dateX, dateY);
    display.print(dateStr);
    
    // ------------------------------
    // Third line: Temperature, Light, WiFi icon
    // ------------------------------
    int thirdLineY = dateY + 12;
    
    // Display temperature
    display.setTextSize(1);
    float temp = getTemperature();
    String tempStr = String(temp);
    int tempX = 0;
    display.setCursor(tempX, thirdLineY);
    display.print(tempStr);
    // Display degree symbol by drawing it
    int degreeX = tempX + tempStr.length() * 6;
    int degreeY = thirdLineY + 1;
    // Draw a small circle for degree symbol
    display.drawCircle(degreeX, degreeY, 1, SSD1306_WHITE);
    // Display 'C' after the degree symbol
    display.setCursor(degreeX + 3, thirdLineY);
    display.print("C");
    
    // Display light level
    display.setTextSize(1);
    int light = getLightLevel();
    String lightStr = String(light) + "lux";
    int lightX = 60; // 居中显示
    display.setCursor(lightX, thirdLineY);
    display.print(lightStr);
    
    // Display small WiFi icon using custom bitmap
    int wifiX = 105;
    int wifiY = thirdLineY-5;
    
    // WiFi signal strength icons
    static const uint8_t wifiIconFull[] PROGMEM = {0x00,0x00,0x00,0x00,0x07,0xE0,0x0C,0x30,0x18,0x18,0x30,0x0C,0x67,0xE6,0x0C,0x30,0x18,0x18,0x03,0xC0,0x06,0x60,0x00,0x00,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00}; /* Full signal */
    static const uint8_t wifiIconHigh[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xE0,0x0C,0x30,0x18,0x18,0x03,0xC0,0x06,0x60,0x00,0x00,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00}; /* High signal */
    static const uint8_t wifiIconMedium[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xC0,0x06,0x60,0x00,0x00,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00}; /* Medium signal */
    static const uint8_t wifiIconLow[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00}; /* Low signal */
    static const uint8_t wifiIconDisconnected[] PROGMEM = {0x00,0x00,0x00,0x00,0x07,0xE0,0x0C,0x30,0x18,0x18,0x30,0x0C,0x67,0xE6,0x0C,0x30,0x18,0x18,0x03,0xC0,0x06,0x60,0x00,0x00,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00}; /* Disconnected */
    
    // Draw appropriate WiFi icon based on signal strength
    if (networkConnected) {
        int rssi = WiFi.RSSI();
        if (rssi >= -50) {
            // Full signal
            display.drawBitmap(wifiX, wifiY, wifiIconFull, 16, 16, SSD1306_WHITE);
        } else if (rssi >= -70) {
            // High signal
            display.drawBitmap(wifiX, wifiY, wifiIconHigh, 16, 16, SSD1306_WHITE);
        } else if (rssi >= -80) {
            // Medium signal
            display.drawBitmap(wifiX, wifiY, wifiIconMedium, 16, 16, SSD1306_WHITE);
        } else {
            // Low signal
            display.drawBitmap(wifiX, wifiY, wifiIconLow, 16, 16, SSD1306_WHITE);
        }
    } else {
        // Disconnected
        display.drawBitmap(wifiX, wifiY, wifiIconDisconnected, 16, 16, SSD1306_WHITE);
        // Draw a cross over the icon
        display.drawLine(wifiX + 2, wifiY + 2, wifiX + 14, wifiY + 14, SSD1306_WHITE);
        display.drawLine(wifiX + 14, wifiY + 2, wifiX + 2, wifiY + 14, SSD1306_WHITE);
    }
    
    // ------------------------------
    // Fourth line: Icons (LED, Fan, Auto, Weather)
    // ------------------------------
    int iconsY = thirdLineY + 12;
    int iconWidth = 30;
    
    // LED icons
    static const uint8_t ledIconOff[] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xE0, 0x0B, 0x10, 0x14, 0x08, 0x14, 0x08, 0x08, 0x10, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20, 0x03, 0xC0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00}; /* LED off */
    static const uint8_t ledIconOn[] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xE0, 0x0F, 0xF0, 0x1F, 0xF8, 0x1F, 0xF8, 0x0F, 0xF0, 0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x03, 0xC0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00}; /* LED on */
    
    // Fan icons
    static const uint8_t fanIconOff[] PROGMEM = {0x00, 0x00, 0x1F, 0x00, 0x0F, 0x80, 0x07, 0xC2, 0x01, 0xE6, 0x08, 0xCE, 0x1D, 0x8E, 0x3F, 0xDE, 0x7B, 0xFC, 0x71, 0xB8, 0x73, 0x10, 0x67, 0x80, 0x43, 0xE0, 0x01, 0xF0, 0x00, 0xF8, 0x00, 0x00}; /* Fan off */
    static const uint8_t fanIconOn[] PROGMEM = {0x00, 0x00, 0x1F, 0x30, 0x0F, 0x98, 0x37, 0xCA, 0x61, 0xE6, 0x48, 0xCE, 0x1D, 0x8E, 0x3F, 0xDE, 0x7B, 0xFC, 0x71, 0xB8, 0x73, 0x12, 0x67, 0x86, 0x53, 0xEC, 0x19, 0xF0, 0x0C, 0xF8, 0x00, 0x00}; /* Fan on */
    
    // Auto mode icon
    static const uint8_t autoIconOff[] PROGMEM = {0x00, 0x00, 0x07, 0xE0, 0x08, 0x10, 0x10, 0x08, 0x24, 0x34, 0x44, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x5F, 0x4A, 0x51, 0x4A, 0x51, 0x4A, 0x31, 0x34, 0x10, 0x08, 0x08, 0x10, 0x07, 0xE0, 0x00, 0x00}; /* Auto off */
    static const uint8_t autoIconOn[] PROGMEM = {0x00, 0x00, 0x07, 0xE0, 0x08, 0x10, 0x10, 0x08, 0x24, 0x34, 0x44, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x5F, 0x4A, 0x51, 0x4A, 0x51, 0x4A, 0x31, 0x34, 0x10, 0x08, 0x08, 0x10, 0x07, 0xE0, 0x00, 0x00}; /* Auto on */

    // Draw LED icon
    int ledX = 0;
    IconState lightState = getIconState(ICON_LIGHT);
    if (lightState == ICON_STATE_ON) {
        if (isLightOn()) {
            display.drawBitmap(ledX, iconsY, ledIconOn, 16, 16, SSD1306_WHITE);
        } else {
            display.drawBitmap(ledX, iconsY, ledIconOff, 16, 16, SSD1306_WHITE);
        }
    } else {
        // Light not connected
        display.drawBitmap(ledX, iconsY, ledIconOff, 16, 16, SSD1306_WHITE);
        // Draw a cross over the icon if not connected
        display.drawLine(ledX + 2, iconsY + 2, ledX + 14, iconsY + 14, SSD1306_WHITE);
        display.drawLine(ledX + 14, iconsY + 2, ledX + 2, iconsY + 14, SSD1306_WHITE);
    }
    
    // Draw Fan icon
    int fanX = iconWidth;
    IconState fanState = getIconState(ICON_FAN);
    if (fanState == ICON_STATE_ON) {
        if (isFanOn()) {
            display.drawBitmap(fanX, iconsY, fanIconOn, 16, 16, SSD1306_WHITE);
        } else {
            display.drawBitmap(fanX, iconsY, fanIconOff, 16, 16, SSD1306_WHITE);
        }
    } else {
        // Fan not connected
        display.drawBitmap(fanX, iconsY, fanIconOff, 16, 16, SSD1306_WHITE);
        // Draw a cross over the icon if not connected
        display.drawLine(fanX + 2, iconsY + 2, fanX + 14, iconsY + 14, SSD1306_WHITE);
        display.drawLine(fanX + 14, iconsY + 2, fanX + 2, iconsY + 14, SSD1306_WHITE);
    }
    
    // Draw Auto mode icon
    int autoX = iconWidth * 2;
    extern bool g_autoMode; // Reference to global auto mode variable
    if (g_autoMode) {
        display.drawBitmap(autoX, iconsY, autoIconOn, 16, 16, SSD1306_WHITE);
        // 更新自动模式图标状态
        updateIconState(ICON_AUTO, ICON_STATE_ON);
    } else {
        display.drawBitmap(autoX, iconsY, autoIconOff, 16, 16, SSD1306_WHITE);
        // Draw a cross over the icon if auto mode is off
        display.drawLine(autoX + 2, iconsY + 2, autoX + 14, iconsY + 14, SSD1306_WHITE);
        display.drawLine(autoX + 14, iconsY + 2, autoX + 2, iconsY + 14, SSD1306_WHITE);
        // 更新自动模式图标状态
        updateIconState(ICON_AUTO, ICON_STATE_OFF);
    }
    

    

    
    // Update display
    display.display();
}


