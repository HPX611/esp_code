// ESP32智能桌面环境控制系统主机主文件

// Blynk所需文件
#define BLYNK_PRINT Serial
#include "../include/blynk.h"
#include <BlynkSimpleEsp32.h>
// #include <BlynkTimer.h>

// 全局定时器对象
BlynkTimer timer;

// FreeRTOS头文件 - 用于多线程任务管理
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// 引入模块
#include "../include/wifi_config.h"
#include <slave_comm.h>
#include <sensors.h>
#include <oled_display.h>
#include "../lib/time/time_manager.h"
#include "../lib/icon/icon_manager.h"

// FreeRTOS任务函数声明
void vWiFiTask(void *pvParameters);
void vSlaveCommTask(void *pvParameters);
void vAutoModeButtonTask(void *pvParameters);
void vBlynkTask(void *pvParameters);

// 全局状态变量
bool g_lightPowerState = false;     // 灯光电源开关状态（开/关）
int g_lightBrightness = 255;          // 灯光亮度值（0-255）
bool g_fanPowerState = false;       // 风扇电源开关状态（开/关）
int g_fanSpeed = 255;               // 风扇速度值（0-255）
bool g_autoMode = true;             // 自动模式状态

// 智能控制阈值
int LIGHT_LEVEL_THRESHOLD = 30;    // 光照阈值（低于此值自动开灯）
int TEMPERATURE_THRESHOLD = 28;     // 温度阈值（高于此值自动开风扇）

// Blynk应用程序的界面组件
WidgetTerminal g_terminal(VIRTUAL_PIN_TERMINAL);

// 自动模式物理按键
const int AUTO_MODE_BUTTON_PIN = 25; // 自动模式物理按键

// FreeRTOS任务句柄
TaskHandle_t xWiFiTaskHandle = NULL;
TaskHandle_t xSlaveCommTaskHandle = NULL;
TaskHandle_t xAutoModeButtonTaskHandle = NULL;
TaskHandle_t xBlynkTaskHandle = NULL;

// 同步原语 - 用于多线程环境下的资源保护
SemaphoreHandle_t xStateMutex = NULL;

// 互斥锁操作宏，简化代码
#define LOCK_STATE() xSemaphoreTake(xStateMutex, portMAX_DELAY)
#define UNLOCK_STATE() xSemaphoreGive(xStateMutex)

// 任务创建宏，简化代码
#define CREATE_TASK(task_func, task_name, stack_size, priority, task_handle, core_id) \
    xTaskCreatePinnedToCore( \
        task_func, \
        task_name, \
        stack_size, \
        NULL, \
        priority, \
        &task_handle, \
        core_id \
    )

/**
 * @brief 处理设备状态控制
 * @param device 设备类型
 * @param state 设备状态
 * @param stateVar 状态变量
 * @param switchPin 开关引脚
 * @param brightnessPin 亮度引脚
 * @param brightnessVar 亮度变量
 * @param deviceName 设备名称
 */
void handleDeviceControl(const char* device, bool state, bool &stateVar, int switchPin, int brightnessPin, int &brightnessVar, const char* deviceName) {
    LOCK_STATE();
    stateVar = state;
    
    Serial.print(deviceName);
    Serial.print("状态: ");
    Serial.println(stateVar ? "ON" : "OFF");
    g_terminal.print(deviceName);
    g_terminal.print("状态: ");
    g_terminal.println(stateVar ? "ON" : "OFF");
    
    // 发送命令到从机
    sendCommand(device, CMD_POWER, stateVar ? "1" : "0");
    
    // 如果开启设备，应用当前亮度/速度
    if (stateVar) {
        sendCommand(device, CMD_BRIGHTNESS, String(brightnessVar).c_str());
    }
    
    UNLOCK_STATE();
}

/**
 * @brief 处理设备亮度/速度控制
 * @param device 设备类型
 * @param value 亮度/速度值
 * @param valueVar 亮度/速度变量
 * @param powerState 电源状态
 * @param controlPin 控制引脚
 * @param controlName 控制名称
 */
void handleDeviceControlValue(const char* device, int value, int &valueVar, bool powerState, int controlPin, const char* controlName) {
    value = constrain(value, 0, 255);  // 确保在0-255范围内
    
    LOCK_STATE();
    valueVar = value;
    
    Serial.print(controlName);
    Serial.print(": ");
    Serial.println(valueVar);
    g_terminal.print(controlName);
    g_terminal.print(": ");
    g_terminal.println(valueVar);
    
    // 发送命令到从机
    if (powerState) {
        sendCommand(device, CMD_BRIGHTNESS, String(valueVar).c_str());
    }
    
    UNLOCK_STATE();
}

/**
 * @brief 处理自动模式下的设备控制
 * @param device 设备类型
 * @param shouldTurnOn 是否应该开启设备
 * @param currentState 当前设备状态
 * @param stateVar 状态变量
 * @param switchPin 开关引脚
 * @param brightnessVar 亮度/速度变量
 * @param actionName 操作名称
 */
void handleAutoDeviceControl(const char* device, bool shouldTurnOn, bool currentState, bool &stateVar, int switchPin, int brightnessVar, const char* actionName) {
    if (shouldTurnOn && !currentState) {
        // 开启设备
        stateVar = true;
        Blynk.virtualWrite(switchPin, 1);
        sendCommand(device, CMD_POWER, "1");
        sendCommand(device, CMD_BRIGHTNESS, String(brightnessVar).c_str());
        Serial.print("自动模式: ");
        Serial.println(actionName);
    } else if (!shouldTurnOn && currentState) {
        // 关闭设备
        stateVar = false;
        Blynk.virtualWrite(switchPin, 0);
        sendCommand(device, CMD_POWER, "0");
        Serial.print("自动模式: ");
        Serial.println(actionName);
    }
}

// 灯光开关控制
BLYNK_WRITE(VIRTUAL_PIN_LIGHT_SWITCH)
{
    int value = param.asInt();
    Serial.print("[Blynk] Light switch pressed: ");
    Serial.println(value);
    handleDeviceControl(DEVICE_LIGHT, (value == 1), g_lightPowerState, VIRTUAL_PIN_LIGHT_SWITCH, VIRTUAL_PIN_LIGHT_BRIGHTNESS, g_lightBrightness, "灯光开关");
}

// 灯光亮度控制
BLYNK_WRITE(VIRTUAL_PIN_LIGHT_BRIGHTNESS)
{
    int value = param.asInt();
    int mappedValue = map(value, 0, 100, 0, 255);
    handleDeviceControlValue(DEVICE_LIGHT, mappedValue, g_lightBrightness, g_lightPowerState, VIRTUAL_PIN_LIGHT_BRIGHTNESS, "灯光亮度");
}

// 风扇开关控制
BLYNK_WRITE(VIRTUAL_PIN_FAN_SWITCH)
{
    int value = param.asInt();
    handleDeviceControl(DEVICE_FAN, (value == 1), g_fanPowerState, VIRTUAL_PIN_FAN_SWITCH, VIRTUAL_PIN_FAN_SPEED, g_fanSpeed, "风扇开关");
}

// 风扇速度控制
BLYNK_WRITE(VIRTUAL_PIN_FAN_SPEED)
{
    int value = param.asInt();
    int mappedValue = map(value, 0, 100, 0, 255);
    handleDeviceControlValue(DEVICE_FAN, mappedValue, g_fanSpeed, g_fanPowerState, VIRTUAL_PIN_FAN_SPEED, "风扇速度");
}

// 自动模式控制
BLYNK_WRITE(VIRTUAL_PIN_AUTO_MODE)
{
    int value = param.asInt();
    
    // 获取互斥锁保护全局变量
    LOCK_STATE();
    g_autoMode = (value == 1);
    
    Serial.print("自动模式: ");
    Serial.println(g_autoMode ? "ON" : "OFF");
    g_terminal.print("自动模式: ");
    g_terminal.println(g_autoMode ? "ON" : "OFF");
    
    // 更新自动模式图标状态
    updateIconState(ICON_AUTO, g_autoMode ? ICON_STATE_ON : ICON_STATE_OFF);
    
    // 释放互斥锁
    UNLOCK_STATE();
}

// 终端输入处理
BLYNK_WRITE(VIRTUAL_PIN_TERMINAL)
{
    String input = param.asStr();
    input.trim();
    
    // 处理阈值调整命令
    if (input.length() > 0) {
        // 解析输入的两个整数
        int firstSpace = input.indexOf(' ');
        if (firstSpace != -1) {
            String tempStr = input.substring(0, firstSpace);
            String lightStr = input.substring(firstSpace + 1);
            
            int newTemp = tempStr.toInt();
            int newLight = lightStr.toInt();
            
            if (newTemp > 0 && newLight > 0) {
                TEMPERATURE_THRESHOLD = newTemp;
                LIGHT_LEVEL_THRESHOLD = newLight;
                
                Serial.print("阈值已更新 - 温度: ");
                Serial.print(TEMPERATURE_THRESHOLD);
                Serial.print(", 光照: ");
                Serial.println(LIGHT_LEVEL_THRESHOLD);
                
                g_terminal.print("阈值已更新 - 温度: ");
                g_terminal.print(TEMPERATURE_THRESHOLD);
                g_terminal.print(", 光照: ");
                g_terminal.println(LIGHT_LEVEL_THRESHOLD);
            }
        }
    }
    g_terminal.flush();
}

// WiFi配置重置控制
BLYNK_WRITE(VIRTUAL_PIN_WIFI_RESET)
{
    int value = param.asInt();
    
    if (value == 1) {
        Serial.println("WiFi配置重置请求...");
        g_terminal.println("WiFi配置重置请求...");
        
        // 重置WiFi配置
        resetWiFiConfig();
        // 注意：resetWiFiConfig()函数会重启设备，所以下面的代码不会执行
    }
}

// 连接成功时同步状态
BLYNK_CONNECTED()
{
    Serial.println("[Blynk] Connected, syncing state...");
    g_terminal.println("Blynk连接成功,同步状态...");
    
    // 获取互斥锁保护全局变量
    LOCK_STATE();
    // 同步设备状态
    Serial.print("[Blynk] Syncing light switch: ");
    Serial.println(g_lightPowerState ? 1 : 0);
    Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_SWITCH, g_lightPowerState ? 1 : 0);
    
    Serial.print("[Blynk] Syncing light brightness: ");
    Serial.println(g_lightBrightness);
    Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_BRIGHTNESS, g_lightBrightness);
    
    Serial.print("[Blynk] Syncing fan switch: ");
    Serial.println(g_fanPowerState ? 1 : 0);
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_SWITCH, g_fanPowerState ? 1 : 0);
    
    Serial.print("[Blynk] Syncing fan speed: ");
    Serial.println(g_fanSpeed);
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_SPEED, g_fanSpeed);
    
    Serial.print("[Blynk] Syncing auto mode: ");
    Serial.println(g_autoMode ? 1 : 0);
    Blynk.virtualWrite(VIRTUAL_PIN_AUTO_MODE, g_autoMode ? 1 : 0);
    
    // 同步设备在线状态
    Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_ONLINE, isLightConnected() ? 1 : 0);
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_ONLINE, isFanConnected() ? 1 : 0);
    
    // 释放互斥锁
    UNLOCK_STATE();
    
    Serial.println("[Blynk] State sync completed");
}

// 智能控制逻辑
void smartControl() {
    // 获取互斥锁保护全局变量
    LOCK_STATE();
    if (!g_autoMode) {
        UNLOCK_STATE();
        return;
    }
    
    bool motion = isMotionDetected();
    int lightLevel = getLightLevel();
    float temperature = getTemperature();
    
    // 基于人体检测和光照的灯光控制
    bool shouldTurnOnLight = motion && (lightLevel < LIGHT_LEVEL_THRESHOLD);
    handleAutoDeviceControl(DEVICE_LIGHT, shouldTurnOnLight, g_lightPowerState, g_lightPowerState, VIRTUAL_PIN_LIGHT_SWITCH, g_lightBrightness, shouldTurnOnLight ? "开灯" : "关灯");
    
    // 基于温度的风扇控制
    bool shouldTurnOnFan = motion && (temperature > TEMPERATURE_THRESHOLD);
    handleAutoDeviceControl(DEVICE_FAN, shouldTurnOnFan, g_fanPowerState, g_fanPowerState, VIRTUAL_PIN_FAN_SWITCH, g_fanSpeed, shouldTurnOnFan ? "开风扇" : "关风扇");
    
    // 释放互斥锁
    UNLOCK_STATE();
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  Serial.println("ESP32 Starting...");

  // 初始化互斥锁
  xStateMutex = xSemaphoreCreateMutex();
  if (xStateMutex == NULL) {
    Serial.println("Error creating mutex!");
  }

  // 初始化WiFi：连接或进入配网模式
  wifiSetup();
  
  // 初始化从机通信
  slaveCommSetup();
  
  // 初始化传感器
  sensorsSetup();
  
  // 初始化OLED显示
  oledSetup();

  // 初始化时间管理
  timeSetup();
  
  // 初始化图标管理器
  iconManagerSetup();
  
  // 初始化自动模式物理按键
  pinMode(AUTO_MODE_BUTTON_PIN, INPUT_PULLUP);

  // 如果WiFi未处于配网模式，则配置Blynk
  if (!isInConfigMode()) {
    Blynk.config(BLYNK_AUTH_TOKEN);
  }

  // 使用BlynkTimer替代部分FreeRTOS任务
  // 传感器任务 - 每3秒执行一次
  timer.setInterval(3000L, []() {
    sensorsLoop();
  });
  
  // 智能控制任务 - 每500毫秒执行一次
  timer.setInterval(500L, []() {
    smartControl();
  });
  
  // 在线状态更新 - 每10秒执行一次
  timer.setInterval(10000L, []() {
    if (isWiFiConnected()) {
      LOCK_STATE();
      Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_ONLINE, isLightConnected() ? 1 : 0);
      Blynk.virtualWrite(VIRTUAL_PIN_FAN_ONLINE, isFanConnected() ? 1 : 0);
      UNLOCK_STATE();
    } else {
      Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_ONLINE, 0);
      Blynk.virtualWrite(VIRTUAL_PIN_FAN_ONLINE, 0);
    }
  });
  
  // 传感器数据更新 - 每3秒执行一次
  timer.setInterval(3000L, []() {
    if (isWiFiConnected()) {
      float temp = getTemperature();
      int light = getLightLevel();
      bool motion = isMotionDetected();
      
      Blynk.virtualWrite(VIRTUAL_PIN_TEMPERATURE, temp);
      Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_LEVEL, light);
      Blynk.virtualWrite(VIRTUAL_PIN_MOTION_DETECTION, motion ? 1 : 0);
      
      // 发送映射后的速度和亮度数据
      LOCK_STATE();
      int mappedFanSpeed = map(g_fanSpeed, 0, 255, 0, 100);
      int mappedLightBrightness = map(g_lightBrightness, 0, 255, 0, 100);
      Blynk.virtualWrite(VIRTUAL_PIN_FAN_SPEED, mappedFanSpeed);
      Blynk.virtualWrite(VIRTUAL_PIN_LIGHT_BRIGHTNESS, mappedLightBrightness);
      UNLOCK_STATE();
    }
  });
  
  // 时间管理任务 - 每秒执行一次
  timer.setInterval(1000L, []() {
    updateTime();
  });
  
  // OLED显示任务 - 每500毫秒执行一次
  timer.setInterval(500L, []() {
    oledLoop();
  });

  // 创建FreeRTOS任务
  // Blynk任务 - 核心1，优先级2（按照最佳实践）
  CREATE_TASK(vBlynkTask, "Blynk Task", 8192, 2, xBlynkTaskHandle, 1);

  // 核心1：运行所有其他任务
  // WiFi任务 - 核心1，优先级1
  CREATE_TASK(vWiFiTask, "WiFi Task", 4096, 1, xWiFiTaskHandle, 1);

  // 从机通信任务 - 核心1，优先级3
  CREATE_TASK(vSlaveCommTask, "Slave Comm Task", 8192, 3, xSlaveCommTaskHandle, 1);


  
  // 自动模式按键处理任务 - 核心1，优先级3
  CREATE_TASK(vAutoModeButtonTask, "Auto Mode Button Task", 2048, 3, xAutoModeButtonTaskHandle, 1);

  Serial.println("All tasks created successfully!");
  Serial.println("物理按键已配置 - 用于离线控制自动模式");
}

// WiFi任务函数
void vWiFiTask(void *pvParameters)
{
    while (1) {
        wifiLoop();
        vTaskDelay(pdMS_TO_TICKS(10)); // 增加延迟，减少任务切换
    }
}

// 从机通信任务函数
void vSlaveCommTask(void *pvParameters)
{
    while (1) {
        slaveCommLoop();
        vTaskDelay(pdMS_TO_TICKS(50)); // 增加延迟，减少任务切换
    }
}





// 打印内存使用情况
void printMemoryInfo() {
    // 打印堆内存使用情况
    size_t freeHeap = ESP.getFreeHeap();
    size_t minimumFreeHeap = ESP.getMinFreeHeap();
    size_t heapSize = ESP.getHeapSize();
    
    Serial.print("[Memory] Free heap: ");
    Serial.print(freeHeap);
    Serial.print(" bytes, Min free heap: ");
    Serial.print(minimumFreeHeap);
    Serial.print(" bytes, Total heap: ");
    Serial.print(heapSize);
    Serial.println(" bytes");
    
    // 打印任务信息
    UBaseType_t uxCurrentNumberOfTasks = uxTaskGetNumberOfTasks();
    Serial.print("[Memory] Number of tasks: ");
    Serial.println(uxCurrentNumberOfTasks);
}

// Blynk任务函数（核心1版本）
void vBlynkTask(void *pvParameters)
{
    // 如果WiFi未处于配网模式，则配置Blynk
    if (!isInConfigMode()) {
        Blynk.config(BLYNK_AUTH_TOKEN);
    }
    
    while(1) {
        // 核心操作：频繁调用 Blynk.run()
        Blynk.run();
        // 运行Blynk定时器任务
        timer.run();
        // 短延迟，确保任务响应及时
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}





// 自动模式按键处理任务
void vAutoModeButtonTask(void *pvParameters)
{
    bool lastButtonState = HIGH;
    unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 50;
    
    while (1) {
        int buttonState = digitalRead(AUTO_MODE_BUTTON_PIN);
        
        if (buttonState != lastButtonState) {
            lastDebounceTime = millis();
        }
        
        if ((millis() - lastDebounceTime) > debounceDelay) {
            if (buttonState == LOW && lastButtonState == HIGH) {
                // 按键按下
                LOCK_STATE();
                g_autoMode = !g_autoMode;
                Serial.print("物理按键 - 自动模式: ");
                Serial.println(g_autoMode ? "ON" : "OFF");
                updateIconState(ICON_AUTO, g_autoMode ? ICON_STATE_ON : ICON_STATE_OFF);
                UNLOCK_STATE();
            }
        }
        
        lastButtonState = buttonState;
        vTaskDelay(pdMS_TO_TICKS(50)); // 增加延迟，减少任务切换
    }
}

void loop()
{
    // // 运行Blynk和定时器
    // Blynk.run();
    // timer.run();
    vTaskDelay(pdMS_TO_TICKS(100000));
//       // 打印loop()所在核心
//   Serial.print("loop() 运行在核心：");
//   Serial.println(xPortGetCoreID());
}