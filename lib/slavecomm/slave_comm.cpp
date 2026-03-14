/**
 * @file slave_comm.cpp
 * @brief 从机通信模块实现文件
 * @details 该文件实现了主机与从机之间的通信逻辑，包括TCP服务器、连接管理和命令处理
 * @author ESP32 Developer
 * @date 2026-02-23
 */

#include "slave_comm.h"
#include "../../include/blynk.h"
#include "../icon/icon_manager.h"

/**
 * @section 函数前向声明
 * 内部辅助函数声明
 */

/**
 * @brief 检查客户端连接状态
 * @details 检查从机连接是否超时，断开超时连接
 */
void checkClientConnections();

/**
 * @brief 处理客户端消息
 * @details 读取并处理从机发送的消息
 */
void processClientMessages();

/**
 * @brief 处理单条消息
 * @param message 接收到的消息内容
 * @param deviceId 设备ID
 */
void processMessage(String message, const char* deviceId);

/**
 * @section 全局变量
 * 从机通信模块的核心变量
 */

/**
 * @brief TCP服务器
 * @details 用于接收从机连接的TCP服务器，端口为8080
 */
WiFiServer server(8080);

/**
 * @brief 灯光客户端连接
 * @details 存储与灯光从机的TCP连接
 */
WiFiClient lightClient;

/**
 * @brief 风扇客户端连接
 * @details 存储与风扇从机的TCP连接
 */
WiFiClient fanClient;

/**
 * @brief 灯光连接状态
 * @details 标记灯光从机是否已连接
 */
bool lightConnected = false;

/**
 * @brief 风扇连接状态
 * @details 标记风扇从机是否已连接
 */
bool fanConnected = false;

/**
 * @brief 灯光最后连接时间
 * @details 用于检测灯光从机连接是否超时
 */
unsigned long lightLastConnectTime = 0;

/**
 * @brief 风扇最后连接时间
 * @details 用于检测风扇从机连接是否超时
 */
unsigned long fanLastConnectTime = 0;

/**
 * @brief 连接超时时间
 * @details 从机连接的超时时间，单位为毫秒
 */
const unsigned long CONNECTION_TIMEOUT = 100000;

/**
 * @brief 灯光从机电源状态
 * @details 存储灯光从机的电源状态，true表示开启，false表示关闭
 */
bool lightPowerState = false;

/**
 * @brief 风扇从机电源状态
 * @details 存储风扇从机的电源状态，true表示开启，false表示关闭
 */
bool fanPowerState = false;

/**
 * @brief 初始化从机通信
 * @details 启动TCP服务器，准备接收从机连接
 */
void slaveCommSetup() {
    // Start TCP server
    server.begin();
    Serial.println("TCP server started on port 8080");
}

/**
 * @brief 从机通信主循环
 * @details 处理从机连接、数据接收和发送
 * @note 该函数应在主循环中频繁调用，以确保通信的实时性
 */
void slaveCommLoop() {
    // Check for new client connections
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New client connected");
        
        // Read client ID from the first message
        unsigned long startTime = millis();
        String clientId = "";
        
        while (millis() - startTime < 5000) { // 5 second timeout
            if (client.available()) {
                clientId = client.readStringUntil('\n');
                clientId.trim();
                break;
            }
            delay(10);
        }
        
        if (clientId == DEVICE_LIGHT) {
            // Disconnect existing light client if any
            if (lightConnected && lightClient.connected()) {
                lightClient.stop();
            }
            lightClient = client;
            lightConnected = true;
            lightLastConnectTime = millis();
            Serial.println("Light slave connected");
            // 更新灯光图标状态
            updateIconState(ICON_LIGHT, ICON_STATE_ON);
        } else if (clientId == DEVICE_FAN) {
            // Disconnect existing fan client if any
            if (fanConnected && fanClient.connected()) {
                fanClient.stop();
            }
            fanClient = client;
            fanConnected = true;
            fanLastConnectTime = millis();
            Serial.println("Fan slave connected");
            // 更新风扇图标状态
            updateIconState(ICON_FAN, ICON_STATE_ON);
        } else {
            Serial.print("Unknown client ID: ");
            Serial.println(clientId);
            client.stop();
        }
    }
    
    // Check client connections
    checkClientConnections();
    
    // Process incoming messages from clients
    processClientMessages();
}

/**
 * @brief 检查客户端连接状态
 * @details 检查从机连接是否超时，断开超时连接
 */
void checkClientConnections() {
    // Check light client
    if (lightConnected) {
        if (!lightClient.connected() || millis() - lightLastConnectTime > CONNECTION_TIMEOUT) {
            lightConnected = false;
            if (lightClient.connected()) {
                lightClient.stop();
            }
            Serial.println("Light slave disconnected");
            // 更新灯光图标状态
            updateIconState(ICON_LIGHT, ICON_STATE_OFF);
        }
    }
    
    // Check fan client
    if (fanConnected) {
        if (!fanClient.connected() || millis() - fanLastConnectTime > CONNECTION_TIMEOUT) {
            fanConnected = false;
            if (fanClient.connected()) {
                fanClient.stop();
            }
            Serial.println("Fan slave disconnected");
            // 更新风扇图标状态
            updateIconState(ICON_FAN, ICON_STATE_OFF);
        }
    }
}

/**
 * @brief 处理客户端消息
 * @details 读取并处理从机发送的消息，更新连接时间
 */
void processClientMessages() {
    // Process light client messages
    if (lightConnected && lightClient.available()) {
        while (lightClient.available()) {
            String message = lightClient.readStringUntil('\n');
            message.trim();
            if (!message.isEmpty()) {
                lightLastConnectTime = millis();
                // Process the message
                processMessage(message, DEVICE_LIGHT);
            }
        }
    }
    
    // Process fan client messages
    if (fanConnected && fanClient.available()) {
        while (fanClient.available()) {
            String message = fanClient.readStringUntil('\n');
            message.trim();
            if (!message.isEmpty()) {
                fanLastConnectTime = millis();
                // Process the message
                processMessage(message, DEVICE_FAN);
            }
        }
    }
}

/**
 * @brief 处理单条消息
 * @param message 接收到的消息内容
 * @param deviceId 设备ID
 * @details 解析并处理从机发送的消息，支持格式为"deviceId|statusType|parameter"
 */
void processMessage(String message, const char* deviceId) {
    // Serial.print("Received from ");
    // Serial.print(deviceId);
    // Serial.print(": ");
    // Serial.println(message);
    
    // Parse the message: deviceId|statusType|parameter
    int firstPipe = message.indexOf('|');
    int secondPipe = message.indexOf('|', firstPipe + 1);
    
    if (firstPipe != -1 && secondPipe != -1) {
        String receivedDeviceId = message.substring(0, firstPipe);
        String statusType = message.substring(firstPipe + 1, secondPipe);
        String parameter = message.substring(secondPipe + 1);
        
        // Process the status update
        if (strcmp(deviceId, DEVICE_LIGHT) == 0) {
            if (statusType == "01") { // Power status
                lightPowerState = (parameter == "1");
                // Serial.println("Light power state updated to: " + String(lightPowerState ? "ON" : "OFF"));
            }
        } else if (strcmp(deviceId, DEVICE_FAN) == 0) {
            if (statusType == "01") { // Power status
                fanPowerState = (parameter == "1");
                // Serial.println("Fan power state updated to: " + String(fanPowerState ? "ON" : "OFF"));
            }
        }
    }
}

/**
 * @brief 发送命令到从机
 * @param deviceId 设备ID，如DEVICE_LIGHT或DEVICE_FAN
 * @param commandType 命令类型，如CMD_POWER或CMD_BRIGHTNESS
 * @param parameter 命令参数，根据命令类型不同而不同
 * @details 格式化并发送命令到指定的从机设备
 */
void sendCommand(const char* deviceId, const char* commandType, const char* parameter) {
    // Format: deviceId|commandType|parameter
    String command = String(deviceId) + "|" + String(commandType) + "|" + String(parameter) + "\n";
    
    if (strcmp(deviceId, DEVICE_LIGHT) == 0) {
        if (lightConnected && lightClient.connected()) {
            lightClient.print(command);
            lightLastConnectTime = millis();
            Serial.print("Sent to light: ");
            Serial.println(command);
        } else {
            Serial.println("Light slave not connected, cannot send command");
        }
    } else if (strcmp(deviceId, DEVICE_FAN) == 0) {
        if (fanConnected && fanClient.connected()) {
            fanClient.print(command);
            fanLastConnectTime = millis();
            Serial.print("Sent to fan: ");
            Serial.println(command);
        } else {
            Serial.println("Fan slave not connected, cannot send command");
        }
    }
}

/**
 * @brief 检查灯光从机是否连接
 * @return true表示灯光从机已连接，false表示未连接
 */
bool isLightConnected() {
    return lightConnected;
}

/**
 * @brief 检查风扇从机是否连接
 * @return true表示风扇从机已连接，false表示未连接
 */
bool isFanConnected() {
    return fanConnected;
}

/**
 * @brief 获取灯光从机的电源状态
 * @return true表示灯光开启，false表示灯光关闭
 */
bool isLightOn() {
    return lightPowerState;
}

/**
 * @brief 获取风扇从机的电源状态
 * @return true表示风扇开启，false表示风扇关闭
 */
bool isFanOn() {
    return fanPowerState;
}
