#include "slave_comm.h"
#include "../../include/blynk.h"
#include "../icon/icon_manager.h"
#include <soc/rtc.h>

extern volatile bool g_lightPowerState;
extern volatile int g_lightBrightness;
extern volatile bool g_fanPowerState;
extern volatile int g_fanSpeed;

extern portMUX_TYPE stateSpinlock;

WiFiServer server(8080);
WiFiClient lightClient;
WiFiClient fanClient;

bool lightConnected = false;
bool fanConnected = false;
unsigned long lightLastConnectTime = 0;
unsigned long fanLastConnectTime = 0;
const unsigned long CONNECTION_TIMEOUT = 10000;

void checkClientConnections();
void processClientMessages();
void handleSlaveMessage(const String& deviceId, const String& type, const String& value);

void slaveCommSetup() {
    server.begin();
}

void slaveCommLoop() {
    WiFiClient client = server.available();
    if (client) {
        unsigned long startTime = millis();
        String clientId = "";
        
        while (millis() - startTime < 5000) {
            if (client.available()) {
                clientId = client.readStringUntil('\n');
                clientId.trim();
                break;
            }
            delay(10);
        }
        
        if (clientId == DEVICE_LIGHT) {
            if (lightConnected && lightClient.connected()) {
                lightClient.stop();
            }
            lightClient = client;
            lightConnected = true;
            lightLastConnectTime = millis();
            updateIconState(ICON_LIGHT, ICON_STATE_ON);
        } else if (clientId == DEVICE_FAN) {
            if (fanConnected && fanClient.connected()) {
                fanClient.stop();
            }
            fanClient = client;
            fanConnected = true;
            fanLastConnectTime = millis();
            updateIconState(ICON_FAN, ICON_STATE_ON);
        } else {
            client.stop();
        }
    }
    
    checkClientConnections();
    processClientMessages();
}

void checkClientConnections() {
    if (lightConnected) {
        if (!lightClient.connected() || millis() - lightLastConnectTime > CONNECTION_TIMEOUT) {
            lightConnected = false;
            if (lightClient.connected()) {
                lightClient.stop();
            }
            updateIconState(ICON_LIGHT, ICON_STATE_OFF);
        }
    }
    
    if (fanConnected) {
        if (!fanClient.connected() || millis() - fanLastConnectTime > CONNECTION_TIMEOUT) {
            fanConnected = false;
            if (fanClient.connected()) {
                fanClient.stop();
            }
            updateIconState(ICON_FAN, ICON_STATE_OFF);
        }
    }
}

void processClientMessages() {
    if (lightConnected && lightClient.available()) {
        while (lightClient.available()) {
            String message = lightClient.readStringUntil('\n');
            message.trim();
            if (!message.isEmpty()) {
                lightLastConnectTime = millis();
                int p1 = message.indexOf('|');
                int p2 = message.indexOf('|', p1 + 1);
                if (p1 > 0 && p2 > p1) {
                    handleSlaveMessage(
                        message.substring(0, p1),
                        message.substring(p1 + 1, p2),
                        message.substring(p2 + 1)
                    );
                }
            }
        }
    }
    
    if (fanConnected && fanClient.available()) {
        while (fanClient.available()) {
            String message = fanClient.readStringUntil('\n');
            message.trim();
            if (!message.isEmpty()) {
                fanLastConnectTime = millis();
                int p1 = message.indexOf('|');
                int p2 = message.indexOf('|', p1 + 1);
                if (p1 > 0 && p2 > p1) {
                    handleSlaveMessage(
                        message.substring(0, p1),
                        message.substring(p1 + 1, p2),
                        message.substring(p2 + 1)
                    );
                }
            }
        }
    }
}

void handleSlaveMessage(const String& deviceId, const String& type, const String& value) {
    if (deviceId == DEVICE_LIGHT) {
        if (type == "01") {
            portENTER_CRITICAL(&stateSpinlock);
            g_lightPowerState = (value == "1");
            portEXIT_CRITICAL(&stateSpinlock);
        } else if (type == "02") {
            portENTER_CRITICAL(&stateSpinlock);
            g_lightBrightness = value.toInt();
            portEXIT_CRITICAL(&stateSpinlock);
        }
    } else if (deviceId == DEVICE_FAN) {
        if (type == "01") {
            portENTER_CRITICAL(&stateSpinlock);
            g_fanPowerState = (value == "1");
            portEXIT_CRITICAL(&stateSpinlock);
        } else if (type == "02") {
            portENTER_CRITICAL(&stateSpinlock);
            g_fanSpeed = value.toInt();
            portEXIT_CRITICAL(&stateSpinlock);
        }
    }
}

void sendCommand(const char* deviceId, const char* commandType, const char* parameter) {
    String command = String(deviceId) + "|" + String(commandType) + "|" + String(parameter) + "\n";
    
    if (strcmp(deviceId, DEVICE_LIGHT) == 0) {
        if (lightConnected && lightClient.connected()) {
            lightClient.print(command);
            lightLastConnectTime = millis();
        }
    } else if (strcmp(deviceId, DEVICE_FAN) == 0) {
        if (fanConnected && fanClient.connected()) {
            fanClient.print(command);
            fanLastConnectTime = millis();
        }
    }
}

bool isLightConnected() {
    return lightConnected;
}

bool isFanConnected() {
    return fanConnected;
}

bool isLightOn() {
    return g_lightPowerState;
}

bool isFanOn() {
    return g_fanPowerState;
}
