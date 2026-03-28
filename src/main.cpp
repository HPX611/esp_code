// ESP32智能桌面环境控制系统主机主文件

#define BLYNK_PRINT Serial
#include "../include/blynk.h"
#include <BlynkSimpleEsp32.h>

BlynkTimer timer;

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rtc.h>

#include "../include/wifi_config.h"
#include <slave_comm.h>
#include <sensors.h>
#include <oled_display.h>
#include "../lib/time/time_manager.h"
#include "../lib/icon/icon_manager.h"

void vWiFiTask(void *pvParameters);
void vSlaveCommTask(void *pvParameters);
void vBlynkTask(void *pvParameters);

IRAM_ATTR void autoModeButtonISR();
IRAM_ATTR void wifiResetButtonISR();

volatile bool g_lightPowerState = false;
volatile int g_lightBrightness = 255;
volatile bool g_fanPowerState = false;
volatile int g_fanSpeed = 255;
volatile bool g_autoMode = true;

volatile bool g_autoModeButtonPressed = false;
volatile bool g_wifiResetButtonPressed = false;

int LIGHT_LEVEL_THRESHOLD = 200;
int TEMPERATURE_THRESHOLD = 28;

WidgetTerminal g_terminal(VIRTUAL_PIN_TERMINAL);

const int AUTO_MODE_BUTTON_PIN = 25;
const int WIFI_RESET_BUTTON_PIN = 26;
const unsigned long DEBOUNCE_DELAY = 50;

TaskHandle_t xWiFiTaskHandle = NULL;
TaskHandle_t xSlaveCommTaskHandle = NULL;
TaskHandle_t xBlynkTaskHandle = NULL;

portMUX_TYPE stateSpinlock = portMUX_INITIALIZER_UNLOCKED;

#define LOCK_STATE() portENTER_CRITICAL(&stateSpinlock)
#define UNLOCK_STATE() portEXIT_CRITICAL(&stateSpinlock)

IRAM_ATTR void autoModeButtonISR() {
    unsigned long debounceStart = millis();
    while((millis() - debounceStart) < DEBOUNCE_DELAY);
    
    if (digitalRead(AUTO_MODE_BUTTON_PIN) == LOW) {
        g_autoMode = !g_autoMode;
        g_autoModeButtonPressed = true;
        updateIconState(ICON_AUTO, g_autoMode ? ICON_STATE_ON : ICON_STATE_OFF);
    }
}

IRAM_ATTR void wifiResetButtonISR() {
    unsigned long debounceStart = millis();
    while((millis() - debounceStart) < DEBOUNCE_DELAY);
    
    if (digitalRead(WIFI_RESET_BUTTON_PIN) == LOW) {
        g_wifiResetButtonPressed = true;
    }
}

void handleButtonPresses() {
    if (g_autoModeButtonPressed) {
        g_autoModeButtonPressed = false;
        Serial.printf("[按键] 自动模式: %s\n", g_autoMode ? "开启" : "关闭");
        g_terminal.printf("[按键] 自动模式: %s\n", g_autoMode ? "开启" : "关闭");
        
        if (Blynk.connected()) {
            Blynk.virtualWrite(VIRTUAL_PIN_AUTO_MODE, g_autoMode ? 1 : 0);
        }
    }
    
    if (g_wifiResetButtonPressed) {
        g_wifiResetButtonPressed = false;
        Serial.println("[按键] WiFi重置");
        g_terminal.println("[按键] WiFi重置");
        resetWiFiConfig();
    }
}

void controlLight(bool turnOn) {
    if (!isLightConnected()) {
        g_terminal.println("错误: 灯光从机离线");
        Serial.println("[控制] 灯光从机离线");
        return;
    }
    
    LOCK_STATE();
    g_lightPowerState = turnOn;
    UNLOCK_STATE();
    
    if (Blynk.connected()) {
        Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_SWITCH, turnOn ? 1 : 0);
    }
    
    sendCommand(DEVICE_LIGHT, CMD_POWER, turnOn ? "1" : "0");
    if (turnOn) {
        int bright;
        LOCK_STATE();
        bright = g_lightBrightness;
        UNLOCK_STATE();
        sendCommand(DEVICE_LIGHT, CMD_BRIGHTNESS, String(bright).c_str());
    }
    
    Serial.printf("[控制] 灯光: %s\n", turnOn ? "已开启" : "已关闭");
    g_terminal.printf("[控制] 灯光: %s\n", turnOn ? "已开启" : "已关闭");
}

void controlFan(bool turnOn) {
    if (!isFanConnected()) {
        g_terminal.println("错误: 风扇从机离线");
        Serial.println("[控制] 风扇从机离线");
        return;
    }
    
    LOCK_STATE();
    g_fanPowerState = turnOn;
    UNLOCK_STATE();
    
    if (Blynk.connected()) {
        Blynk.virtualWrite(VIRTUAL_PIN_FAN_SWITCH, turnOn ? 1 : 0);
    }
    
    sendCommand(DEVICE_FAN, CMD_POWER, turnOn ? "1" : "0");
    if (turnOn) {
        int speed;
        LOCK_STATE();
        speed = g_fanSpeed;
        UNLOCK_STATE();
        sendCommand(DEVICE_FAN, CMD_BRIGHTNESS, String(speed).c_str());
    }
    
    Serial.printf("[控制] 风扇: %s\n", turnOn ? "已开启" : "已关闭");
    g_terminal.printf("[控制] 风扇: %s\n", turnOn ? "已开启" : "已关闭");
}

void setLightBrightness(int value) {
    value = constrain(value, 0, 255);
    LOCK_STATE();
    g_lightBrightness = value;
    UNLOCK_STATE();
    
    if (Blynk.connected()) {
        Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_BRIGHTNESS, map(value, 0, 255, 0, 100));
    }
    
    Serial.printf("[控制] 灯光亮度: %d%%\n", map(value, 0, 255, 0, 100));
    g_terminal.printf("[控制] 灯光亮度: %d%%\n", map(value, 0, 255, 0, 100));
    
    bool isOn;
    LOCK_STATE();
    isOn = g_lightPowerState;
    UNLOCK_STATE();
    
    if (isOn && isLightConnected()) {
        sendCommand(DEVICE_LIGHT, CMD_BRIGHTNESS, String(value).c_str());
    }
}

void setFanSpeed(int value) {
    value = constrain(value, 0, 255);
    LOCK_STATE();
    g_fanSpeed = value;
    UNLOCK_STATE();
    
    if (Blynk.connected()) {
        Blynk.virtualWrite(VIRTUAL_PIN_FAN_SPEED, map(value, 0, 255, 0, 100));
    }
    
    Serial.printf("[控制] 风扇速度: %d%%\n", map(value, 0, 255, 0, 100));
    g_terminal.printf("[控制] 风扇速度: %d%%\n", map(value, 0, 255, 0, 100));
    
    bool isOn;
    LOCK_STATE();
    isOn = g_fanPowerState;
    UNLOCK_STATE();
    
    if (isOn && isFanConnected()) {
        sendCommand(DEVICE_FAN, CMD_BRIGHTNESS, String(value).c_str());
    }
}

void checkAndControlLight() {
    int light = getLightLevel();
    bool shouldBeOn = light < LIGHT_LEVEL_THRESHOLD;
    
    bool currentState;
    LOCK_STATE();
    currentState = g_lightPowerState;
    UNLOCK_STATE();
    
    Serial.printf("[自动-灯] 光照:%dlux 阈值:%d 应:%s 现:%s\n", 
        light, LIGHT_LEVEL_THRESHOLD, 
        shouldBeOn ? "开" : "关", 
        currentState ? "开" : "关");
    
    if (shouldBeOn == currentState) return;
    
    if (!isLightConnected()) {
        g_terminal.println("[自动] 灯光从机离线");
        return;
    }
    
    LOCK_STATE();
    g_lightPowerState = shouldBeOn;
    UNLOCK_STATE();
    
    if (Blynk.connected()) {
        Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_SWITCH, shouldBeOn ? 1 : 0);
    }
    sendCommand(DEVICE_LIGHT, CMD_POWER, shouldBeOn ? "1" : "0");
    if (shouldBeOn) {
        int bright;
        LOCK_STATE();
        bright = g_lightBrightness;
        UNLOCK_STATE();
        sendCommand(DEVICE_LIGHT, CMD_BRIGHTNESS, String(bright).c_str());
    }
    
    Serial.printf("[自动] %s\n", shouldBeOn ? "开灯(光照不足)" : "关灯(光照充足)");
    g_terminal.printf("[自动] %s\n", shouldBeOn ? "开灯(光照不足)" : "关灯(光照充足)");
}

void checkAndControlFan() {
    float temp = getTemperature();
    bool shouldBeOn = temp > TEMPERATURE_THRESHOLD;
    
    bool currentState;
    LOCK_STATE();
    currentState = g_fanPowerState;
    UNLOCK_STATE();
    
    Serial.printf("[自动-扇] 温度:%.1fC 阈值:%d 应:%s 现:%s\n", 
        temp, TEMPERATURE_THRESHOLD, 
        shouldBeOn ? "开" : "关", 
        currentState ? "开" : "关");
    
    if (shouldBeOn == currentState) return;
    
    if (!isFanConnected()) {
        g_terminal.println("[自动] 风扇从机离线");
        return;
    }
    
    LOCK_STATE();
    g_fanPowerState = shouldBeOn;
    UNLOCK_STATE();
    
    if (Blynk.connected()) {
        Blynk.virtualWrite(VIRTUAL_PIN_FAN_SWITCH, shouldBeOn ? 1 : 0);
    }
    sendCommand(DEVICE_FAN, CMD_POWER, shouldBeOn ? "1" : "0");
    if (shouldBeOn) {
        int speed;
        LOCK_STATE();
        speed = g_fanSpeed;
        UNLOCK_STATE();
        sendCommand(DEVICE_FAN, CMD_BRIGHTNESS, String(speed).c_str());
    }
    
    Serial.printf("[自动] %s\n", shouldBeOn ? "开风扇(温度过高)" : "关风扇(温度适宜)");
    g_terminal.printf("[自动] %s\n", shouldBeOn ? "开风扇(温度过高)" : "关风扇(温度适宜)");
}

void turnOffAllDevices() {
    bool lightState, fanState;
    LOCK_STATE();
    lightState = g_lightPowerState;
    fanState = g_fanPowerState;
    UNLOCK_STATE();
    
    if (lightState && isLightConnected()) {
        LOCK_STATE();
        g_lightPowerState = false;
        UNLOCK_STATE();
        
        if (Blynk.connected()) {
            Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_SWITCH, 0);
        }
        sendCommand(DEVICE_LIGHT, CMD_POWER, "0");
        
        Serial.println("[自动] 关灯(无人)");
        g_terminal.println("[自动] 关灯(无人)");
    }
    
    if (fanState && isFanConnected()) {
        LOCK_STATE();
        g_fanPowerState = false;
        UNLOCK_STATE();
        
        if (Blynk.connected()) {
            Blynk.virtualWrite(VIRTUAL_PIN_FAN_SWITCH, 0);
        }
        sendCommand(DEVICE_FAN, CMD_POWER, "0");
        
        Serial.println("[自动] 关风扇(无人)");
        g_terminal.println("[自动] 关风扇(无人)");
    }
}

void smartControl() {
    static unsigned long lastLogTime = 0;
    
    handleButtonPresses();
    
    bool autoMd;
    LOCK_STATE();
    autoMd = g_autoMode;
    UNLOCK_STATE();
    
    if (!autoMd) return;
    
    bool motion = isMotionDetected();
    
    if (millis() - lastLogTime > 5000) {
        lastLogTime = millis();
        Serial.printf("[自动] 人体检测: %s\n", motion ? "有人" : "无人");
    }
    
    if (motion) {
        checkAndControlLight();
        checkAndControlFan();
    } else {
        turnOffAllDevices();
    }
}

BLYNK_WRITE(VIRTUAL_PIN_LIGHT_SWITCH) {
    Serial.println("[Blynk] 灯光开关控制");
    controlLight(param.asInt() == 1);
}

BLYNK_WRITE(VIRTUAL_PIN_LIGHT_BRIGHTNESS) {
    Serial.println("[Blynk] 灯光亮度控制");
    setLightBrightness(map(param.asInt(), 0, 100, 0, 255));
}

BLYNK_WRITE(VIRTUAL_PIN_FAN_SWITCH) {
    Serial.println("[Blynk] 风扇开关控制");
    controlFan(param.asInt() == 1);
}

BLYNK_WRITE(VIRTUAL_PIN_FAN_SPEED) {
    Serial.println("[Blynk] 风扇速度控制");
    setFanSpeed(map(param.asInt(), 0, 100, 0, 255));
}

BLYNK_WRITE(VIRTUAL_PIN_AUTO_MODE) {
    LOCK_STATE();
    g_autoMode = (param.asInt() == 1);
    UNLOCK_STATE();
    
    Serial.printf("[Blynk] 自动模式: %s\n", g_autoMode ? "开启" : "关闭");
    g_terminal.printf("[Blynk] 自动模式: %s\n", g_autoMode ? "开启" : "关闭");
    updateIconState(ICON_AUTO, g_autoMode ? ICON_STATE_ON : ICON_STATE_OFF);
}

BLYNK_WRITE(VIRTUAL_PIN_TERMINAL) {
    String input = param.asStr();
    input.trim();
    
    if (input.length() > 0) {
        int space = input.indexOf(' ');
        if (space != -1) {
            int temp = input.substring(0, space).toInt();
            int light = input.substring(space + 1).toInt();
            
            if (temp > 0 && light > 0) {
                TEMPERATURE_THRESHOLD = temp;
                LIGHT_LEVEL_THRESHOLD = light;
                Serial.printf("[Blynk] 阈值: 温度%dC 光照%dlux\n", temp, light);
                g_terminal.printf("[Blynk] 阈值: 温度%dC 光照%dlux\n", temp, light);
            }
        }
    }
}

BLYNK_WRITE(VIRTUAL_PIN_WIFI_RESET) {
    if (param.asInt() == 1) {
        Serial.println("[Blynk] WiFi重置");
        g_terminal.println("[Blynk] WiFi重置");
        resetWiFiConfig();
    }
}

void syncAllStatesToBlynk() {
    if (!Blynk.connected()) {
        return;
    }
    
    bool lightState; int lightBright; bool fanState; int fanSpd; bool autoMd;
    LOCK_STATE();
    lightState = g_lightPowerState;
    lightBright = g_lightBrightness;
    fanState = g_fanPowerState;
    fanSpd = g_fanSpeed;
    autoMd = g_autoMode;
    UNLOCK_STATE();
    
    Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_SWITCH, lightState ? 1 : 0);
    Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_BRIGHTNESS, map(lightBright, 0, 255, 0, 100));
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_SWITCH, fanState ? 1 : 0);
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_SPEED, map(fanSpd, 0, 255, 0, 100));
    Blynk.virtualWrite(VIRTUAL_PIN_AUTO_MODE, autoMd ? 1 : 0);
    Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_ONLINE, isLightConnected() ? 1 : 0);
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_ONLINE, isFanConnected() ? 1 : 0);
    Blynk.virtualWrite(VIRTUAL_PIN_TEMPERATURE, (int)getTemperature());
    Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_LEVEL, getLightLevel());
    Blynk.virtualWrite(VIRTUAL_PIN_MOTION_DETECTION, isMotionDetected() ? 1 : 0);
    
    Serial.println("[同步] 状态已同步到Blynk");
}

BLYNK_CONNECTED() {
    Serial.println("[Blynk] 已连接");
    g_terminal.println("[Blynk] 已连接");
    syncAllStatesToBlynk();
}

void setup() {
    Serial.begin(115200);
    Serial.println("系统启动...");

    wifiSetup();
    slaveCommSetup();
    sensorsSetup();
    oledSetup();
    timeSetup();
    iconManagerSetup();
    
    pinMode(AUTO_MODE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(WIFI_RESET_BUTTON_PIN, INPUT_PULLUP);
    
    attachInterrupt(digitalPinToInterrupt(AUTO_MODE_BUTTON_PIN), autoModeButtonISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(WIFI_RESET_BUTTON_PIN), wifiResetButtonISR, FALLING);

    if (!isInConfigMode()) {
        Blynk.config(BLYNK_AUTH_TOKEN);
    }

    timer.setInterval(1000L, smartControl);
    timer.setInterval(1000L, updateTime);
    timer.setInterval(1000L, oledLoop);
    timer.setInterval(2000L, sensorsLoop);
    timer.setInterval(5000L, syncAllStatesToBlynk);

    xTaskCreatePinnedToCore(vBlynkTask, "Blynk Task", 4096, NULL, 2, &xBlynkTaskHandle, 1);
    xTaskCreatePinnedToCore(vWiFiTask, "WiFi Task", 4096, NULL, 1, &xWiFiTaskHandle, 1);
    xTaskCreatePinnedToCore(vSlaveCommTask, "Slave Comm Task", 4096, NULL, 3, &xSlaveCommTaskHandle, 1);
    
    Serial.println("系统初始化完成");
}

void vWiFiTask(void *pvParameters) {
    while (1) {
        wifiLoop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vSlaveCommTask(void *pvParameters) {
    while (1) {
        slaveCommLoop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vBlynkTask(void *pvParameters) {
    while(1) {
        if (isWiFiConnected()) {
            if (!Blynk.connected()) {
                Serial.println("[Blynk] 重连中...");
                Blynk.connect();
            }
            Blynk.run();
            timer.run();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void loop() {
    vTaskDelete(NULL);
}
