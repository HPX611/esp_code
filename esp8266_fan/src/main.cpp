#include <ESP8266WiFi.h>

// 函数原型声明
void connectToMaster();
void connectToTCPServer();
void processCommands();
void controlFan();
void sendStatus();

// ==================== 配置常量 ====================

// 主机热点信息
const char* masterSSID = "ESP32_Master";
const char* masterPassword = "smartdesk123";

// 主机IP和端口
const char* masterIP = "192.168.4.1";
const int masterPort = 8080;

// 设备ID
const char* deviceID = "02"; // 风扇设备

// 引脚定义
const int FAN_PIN = 5;          // D1 on NodeMCU (风扇控制)
const int STATUS_PIN = 2;        // GPIO2 (D4) on NodeMCU (状态指示)
const int POWER_BUTTON_PIN = 4;  // D2 on NodeMCU (电源按键)
const int SPEED_UP_BUTTON_PIN = 0;  // D3 on NodeMCU (升速按键)
const int SPEED_DOWN_BUTTON_PIN = 14; // D5 on NodeMCU (降速按键)

// 时间常量
const unsigned long debounceDelay = 50;         // 按键去抖延迟
const unsigned long RECONNECT_INTERVAL = 5000;  // 重连间隔
const unsigned long BLINK_INTERVAL_WIFI = 1000; // WiFi未连接时LED闪烁间隔
const unsigned long BLINK_INTERVAL_TCP = 500;   // TCP未连接时LED闪烁间隔

// ==================== 全局变量 ====================

// WiFi客户端
WiFiClient client;

// 设备状态
volatile bool powerState = false;  // 电源状态
volatile int speed = 255;          // 风扇速度 (0-255)
volatile bool statusChanged = false; // 状态变更标志

// 重连控制
unsigned long lastReconnectAttempt = 0;

// LED状态控制
unsigned long lastBlinkTime = 0;
bool ledState = false;

// ==================== 中断处理函数 ====================

IRAM_ATTR void powerButtonISR() {
    unsigned long debounceStart = millis();
    while((millis() - debounceStart) < debounceDelay);
    
    // 检查按键是否仍然按下
    if (digitalRead(POWER_BUTTON_PIN) == LOW) {
        powerState = !powerState;
        controlFan();
        statusChanged = true;
    }
}

IRAM_ATTR void speedUpButtonISR() {
    unsigned long debounceStart = millis();
    while((millis() - debounceStart) < debounceDelay);
    
    // 检查按键是否仍然按下
    if (digitalRead(SPEED_UP_BUTTON_PIN) == LOW) {
        speed = constrain(speed + 51, 0, 255);
        controlFan();
        statusChanged = true;
    }
}

IRAM_ATTR void speedDownButtonISR() {
    unsigned long debounceStart = millis();
    while((millis() - debounceStart) < debounceDelay);
    
    // 检查按键是否仍然按下
    if (digitalRead(SPEED_DOWN_BUTTON_PIN) == LOW) {
        speed = constrain(speed - 51, 0, 255);
        controlFan();
        statusChanged = true;
    }
}

// ==================== 初始化函数 ====================

void setup() {
    // 初始化串口
    Serial.begin(115200);
    Serial.println("ESP8266 Fan Slave Starting...");
    
    // 初始化风扇引脚
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);
    
    // 初始化状态指示引脚（上拉电阻，低电平点亮）
    pinMode(STATUS_PIN, OUTPUT);
    digitalWrite(STATUS_PIN, HIGH); // 初始状态熄灭
     
    // 初始化按键引脚
    pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SPEED_UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SPEED_DOWN_BUTTON_PIN, INPUT_PULLUP);
    
    // 附加中断
    attachInterrupt(digitalPinToInterrupt(POWER_BUTTON_PIN), powerButtonISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(SPEED_UP_BUTTON_PIN), speedUpButtonISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(SPEED_DOWN_BUTTON_PIN), speedDownButtonISR, FALLING);
    
    // 连接到主机热点
    connectToMaster();
}

// ==================== 主循环 ====================

void loop() {
    // 检查WiFi连接
    if (WiFi.status() != WL_CONNECTED) {
        // WiFi未连接，LED闪烁
        if (millis() - lastBlinkTime > BLINK_INTERVAL_WIFI) {
            lastBlinkTime = millis();
            ledState = !ledState;
            digitalWrite(STATUS_PIN, !ledState); // 上拉电阻，低电平点亮
        }
        
        // 尝试重连
        if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = millis();
            connectToMaster();
        }
    } else {
        // 检查TCP连接
        if (!client.connected()) {
            Serial.println("ESP8266 Fan Slave - TCP Connection Failed!");
            // WiFi连接但TCP未连接，LED闪烁
            if (millis() - lastBlinkTime > BLINK_INTERVAL_TCP) {
                lastBlinkTime = millis();
                ledState = !ledState;
                digitalWrite(STATUS_PIN, !ledState); // 上拉电阻，低电平点亮
            }
            connectToTCPServer();
        } else {
            // 连接成功，LED常亮（低电平）
            digitalWrite(STATUS_PIN, LOW);
            // 处理收到的命令
            processCommands();
        }
    }
    
    // 处理状态变化
    if (statusChanged) {
        statusChanged = false;
        sendStatus();
        Serial.println("Status changed - Power: " + String(powerState ? "ON" : "OFF") + ", Speed: " + String(speed));
    }
    
    // 短暂延时
    delay(10);
}

// ==================== 网络连接函数 ====================

void connectToMaster() {
    Serial.print("Connecting to master AP: ");
    Serial.println(masterSSID);
    
    WiFi.begin(masterSSID, masterPassword);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(100);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to master AP!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        // 连接到TCP服务器
        connectToTCPServer();
    } else {
        Serial.println("\nFailed to connect to master AP!");
    }
}

void connectToTCPServer() {
    Serial.print("Connecting to TCP server (" + String(masterIP) + ":" + String(masterPort) + ")...");
    
    if (client.connect(masterIP, masterPort)) {
        Serial.println(" Connected!");
        
        // 发送设备ID
        client.println(deviceID);
        Serial.println("Sent device ID: " + String(deviceID));
        
        // 发送当前状态
        sendStatus();
    } else {
        Serial.println(" Failed!");
    }
}

// ==================== 命令处理函数 ====================

void processCommands() {
    if (client.available()) {
        String command = client.readStringUntil('\n');
        command.trim();
        
        Serial.println("Received command: " + command);
        
        // 解析命令：deviceID|commandType|parameter
        int firstPipe = command.indexOf('|');
        int secondPipe = command.indexOf('|', firstPipe + 1);
        
        if (firstPipe != -1 && secondPipe != -1) {
            String receivedDeviceID = command.substring(0, firstPipe);
            String commandType = command.substring(firstPipe + 1, secondPipe);
            String parameter = command.substring(secondPipe + 1);
            
            // 检查设备ID是否匹配
            if (receivedDeviceID == deviceID) {
                // 处理电源控制命令
                if (commandType == "01") {
                    powerState = (parameter == "1");
                    controlFan();
                    sendStatus();
                    Serial.println("Power state changed to: " + String(powerState ? "ON" : "OFF"));
                }
                // 处理速度控制命令
                else if (commandType == "02") {
                    speed = parameter.toInt();
                    speed = constrain(speed, 0, 255);
                    if (powerState) {
                        controlFan();
                    }
                    sendStatus();
                    Serial.println("Speed changed to: " + String(speed));
                }
            }
        }
    }
}

// ==================== 设备控制函数 ====================

void controlFan() {
    if (powerState) {
        // 使用PWM控制速度
        if(speed == 0) speed = 10;
        analogWrite(FAN_PIN, speed);
    } else {
        // 关闭风扇
        digitalWrite(FAN_PIN, LOW);
    }
}

void sendStatus() {
    if (client.connected()) {
        // 发送电源状态
        String powerStatus = String(deviceID) + "|01|" + String(powerState ? "1" : "0") + "\n";
        client.print(powerStatus);
        
        // 发送速度状态
        String speedStatus = String(deviceID) + "|02|" + String(speed) + "\n";
        client.print(speedStatus);
        
        Serial.println("Sent status: Power=" + String(powerState ? "1" : "0") + ", Speed=" + String(speed));
    }
}