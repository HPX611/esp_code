#include "wifi_config.h"

// 配置模式AP设置
const char* CONFIG_AP_SSID = "ESP32_Config";
const char* CONFIG_AP_PASSWORD = "12345678";

// LED引脚定义
const int LED_PIN = 2; // D2引脚对应ESP32的GPIO4
void (*ledCallback)(int state) = NULL;

// 从机通信AP设置
const char* MASTER_AP_SSID = "ESP32_Master";
const char* MASTER_AP_PASSWORD = "smartdesk123";
IPAddress MASTER_AP_IP(192, 168, 4, 1);

// Web服务器和DNS服务器
WebServer webServer(80);
DNSServer dnsServer;

// 存储WiFi凭证的偏好设置
Preferences preferences;
const char* PREF_NAMESPACE = "wifi-creds";
const char* PREF_KEY_SSID = "ssid";
const char* PREF_KEY_PASS = "password";

// 当前WiFi状态
WiFiStatus currentStatus = WIFI_NOT_CONFIGURED;

// Web处理函数前向声明
void handleRoot();
void handleSave();
void handleNotFound();

// 配置页面HTML
const char* configPage = "<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <title>ESP32 WiFi Config</title>\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
"    <style>\n"
"        body { font-family: Arial, sans-serif; margin: 20px; }\n"
"        .container { max-width: 400px; margin: auto; padding: 20px; border: 1px solid #ccc; border-radius: 10px; }\n"
"        input[type=text], input[type=password] { width: 100%; padding: 12px 20px; margin: 8px 0; display: inline-block; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }\n"
"        button { background-color: #4CAF50; color: white; padding: 14px 20px; margin: 8px 0; border: none; border-radius: 4px; cursor: pointer; width: 100%; }\n"
"        button:hover { background-color: #45a049; }\n"
"        .btn-secondary { background-color: #008CBA; }\n"
"        .btn-secondary:hover { background-color: #007B9E; }\n"
"        .form-group { margin-bottom: 15px; }\n"
"        .location-buttons { display: flex; flex-wrap: wrap; gap: 10px; margin-bottom: 15px; }\n"
"        .location-buttons button { flex: 1 1 calc(33.333% - 7px); }\n"
"        .info-box { background-color: #f0f8ff; padding: 10px; border-radius: 4px; margin-bottom: 15px; font-size: 14px; }\n"
"    </style>\n"
"    <script>\n"
"        function getLocation() {\n"
"            if (navigator.geolocation) {\n"
"                alert('请在浏览器提示中允许位置访问权限\\n\\n步骤：\\n1. 当浏览器弹出权限请求时，选择\"允许\"\\n2. 等待几秒钟获取位置信息\\n3. 系统会自动填充城市名称');\n"
"                var options = { enableHighAccuracy: true, timeout: 15000, maximumAge: 0 };\n"
"                navigator.geolocation.getCurrentPosition(showPosition, showError, options);\n"
"            } else {\n"
"                alert('您的浏览器不支持GPS定位功能\\n\\n请尝试使用Chrome、Safari或Firefox浏览器，或手动输入城市名称');\n"
"            }\n"
"        }\n"
"        function showPosition(position) {\n"
"            var lat = position.coords.latitude;\n"
"            var lon = position.coords.longitude;\n"
"            var accuracy = position.coords.accuracy;\n"
"            alert('正在获取位置信息，请稍候...\\n\\n精度: ' + accuracy.toFixed(2) + '米');\n"
"            var url = 'https://nominatim.openstreetmap.org/reverse?format=json&lat=' + lat + '&lon=' + lon + '&zoom=10&addressdetails=1&accept-language=zh-CN';\n"
"            fetch(url, { method: 'GET', headers: { 'User-Agent': 'ESP32-Config/1.0' } })\n" 
"            .then(response => { if (!response.ok) throw new Error('网络请求失败: ' + response.status); return response.json(); })\n" 
"            .then(data => {\n" 
"                var city = data.address.city || data.address.town || data.address.village || data.address.county || data.address.province || '未知城市';\n" 
"                document.getElementById('city').value = city;\n" 
"                alert('已成功获取您的位置: ' + city);\n" 
"            })\n" 
"            .catch(error => {\n" 
"                alert('获取位置信息失败: ' + error.message + '\\n\\n请尝试手动输入城市名称');\n" 
"            });\n" 
"        }\n"
"        function showError(error) {\n"
"            switch(error.code) {\n"
"                case error.PERMISSION_DENIED:\n"
"                    alert('用户拒绝了地理位置请求\\n\\n请在浏览器设置中允许位置访问权限，或手动输入城市名称');\n"
"                    break;\n"
"                case error.POSITION_UNAVAILABLE:\n"
"                    alert('位置信息不可用\\n\\n请检查设备GPS是否开启，或手动输入城市名称');\n"
"                    break;\n"
"                case error.TIMEOUT:\n"
"                    alert('获取位置超时\\n\\n请检查网络连接，或手动输入城市名称');\n"
"                    break;\n"
"                case error.UNKNOWN_ERROR:\n"
"                    alert('发生未知错误\\n\\n请尝试手动输入城市名称');\n"
"                    break;\n"
"                default:\n"
"                    alert('定位失败: 错误代码 ' + error.code + '\\n\\n请手动输入城市名称');\n"
"                    break;\n"
"            }\n"
"        }\n"
"    </script>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <h2>ESP32 网络配置</h2>\n"
"        <div class=\"info-box\">\n" 
"            <strong>GPS定位说明：</strong><br>\n" 
"            - 请在浏览器提示中允许位置访问权限<br>\n" 
"            - 如果获取失败，请手动选择或输入城市名称<br>\n" 
"        </div>\n"
"        <form action=\"/save\" method=\"POST\">\n"
"            <div class=\"form-group\">\n"
"                <label for=\"ssid\">WiFi 名称</label>\n"
"                <input type=\"text\" id=\"ssid\" name=\"ssid\" placeholder=\"请输入WiFi名称...\">\n"
"            </div>\n"
"            <div class=\"form-group\">\n"
"                <label for=\"pass\">WiFi 密码</label>\n"
"                <input type=\"password\" id=\"pass\" name=\"password\" placeholder=\"请输入WiFi密码...\">\n"
"            </div>\n"
"            <div class=\"form-group\">\n"
"                <label for=\"city\">城市地址</label>\n"
"                <input type=\"text\" id=\"city\" name=\"city\" placeholder=\"请输入城市名称（如：北京、上海）...\">\n"
"            </div>\n"
"            <div class=\"location-buttons\">\n"
"                <button type=\"button\" class=\"btn-secondary\" onclick=\"getLocation()\">获取GPS位置</button>\n"
"                <button type=\"button\" class=\"btn-secondary\" onclick=\"document.getElementById('city').value='北京'\">北京</button>\n"
"                <button type=\"button\" class=\"btn-secondary\" onclick=\"document.getElementById('city').value='上海'\">上海</button>\n"
"                <button type=\"button\" class=\"btn-secondary\" onclick=\"document.getElementById('city').value='广州'\">广州</button>\n"
"                <button type=\"button\" class=\"btn-secondary\" onclick=\"document.getElementById('city').value='深圳'\">深圳</button>\n"
"                <button type=\"button\" class=\"btn-secondary\" onclick=\"document.getElementById('city').value='成都'\">成都</button>\n"
"                <button type=\"button\" class=\"btn-secondary\" onclick=\"document.getElementById('city').value='杭州'\">杭州</button>\n"
"                <button type=\"button\" class=\"btn-secondary\" onclick=\"document.getElementById('city').value='武汉'\">武汉</button>\n"
"                <button type=\"button\" class=\"btn-secondary\" onclick=\"document.getElementById('city').value='西安'\">西安</button>\n"
"            </div>\n"
"            <button type=\"submit\">保存并重启</button>\n"
"        </form>\n"
"    </div>\n"
"</body>\n"
"</html>\n";

// WiFi连接状态变量
unsigned long wifiConnectStartTime = 0;
int wifiConnectAttempts = 0;
const int MAX_WIFI_ATTEMPTS = 30; // 30秒超时
String currentSsid = "";
String currentPass = "";

void wifiSetup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    preferences.begin(PREF_NAMESPACE, false);
    String storedSsid = preferences.getString(PREF_KEY_SSID, "");
    String storedPass = preferences.getString(PREF_KEY_PASS, "");
    preferences.end();

    WiFi.mode(WIFI_AP_STA);
    
    WiFi.softAPConfig(MASTER_AP_IP, MASTER_AP_IP, IPAddress(255, 255, 255, 0));
    boolean apStarted = WiFi.softAP(MASTER_AP_SSID, MASTER_AP_PASSWORD);

    if (storedSsid.length() > 0) {
        Serial.println("\n发现存储的WiFi凭证，正在连接...");

        currentStatus = WIFI_CONNECTING;
        currentSsid = storedSsid;
        currentPass = storedPass;
        wifiConnectStartTime = millis();
        wifiConnectAttempts = 0;
        
        WiFi.begin(currentSsid.c_str(), currentPass.c_str());
    } else {
        Serial.println("\n未存储WiFi凭证，进入配网模式...");
        currentStatus = WIFI_NOT_CONFIGURED;
        startConfigMode();
    }
}

// 网络连接检查间隔（毫秒）
const unsigned long CONNECTION_CHECK_INTERVAL = 30000; // 30秒
// 上次网络连接检查时间
unsigned long lastConnectionCheckTime = 0;
// LED闪烁状态变量
unsigned long lastLEDBlinkTime = 0;
bool ledState = false;

void wifiLoop() {
    unsigned long currentTime = millis();
    
    // 更新LED状态
    updateLEDState(currentStatus);
    
    if (currentStatus == WIFI_CONFIG_MODE) {
        dnsServer.processNextRequest();
        webServer.handleClient();
    } else if (currentStatus == WIFI_CONNECTING) {
        // 非阻塞式检查WiFi连接状态
        if (WiFi.status() == WL_CONNECTED) {
            // 连接成功
            Serial.println("\nWiFi连接成功！");
            Serial.print("SSID: ");
            Serial.println(WiFi.SSID());
            Serial.print("IP地址: ");
            Serial.println(WiFi.localIP());
            Serial.print("信号强度: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            currentStatus = WIFI_CONNECTED;
        } else if (currentTime - wifiConnectStartTime > MAX_WIFI_ATTEMPTS * 1000) {
            // 连接超时
            Serial.println("\n连接WiFi失败，进入配网模式...");
            currentStatus = WIFI_FAILED;
            blinkLED(3, 200);
            startConfigMode();
        } else if (currentTime - wifiConnectStartTime > wifiConnectAttempts * 1000) {
            // 每秒钟打印一次连接状态
            wifiConnectAttempts++;
            Serial.print(".");
            
            // 每10次尝试打印一次当前状态
            if (wifiConnectAttempts % 10 == 0) {
                Serial.print("\n尝试连接中...");
            }
        }
    } else if (currentStatus == WIFI_CONNECTED) {
        // 定期检查网络连接状态
        if (currentTime - lastConnectionCheckTime > CONNECTION_CHECK_INTERVAL) {
            lastConnectionCheckTime = currentTime;
            
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("\n网络连接已断开，尝试重新连接...");
                
                // 重新获取存储的WiFi凭证
                preferences.begin(PREF_NAMESPACE, false);
                String storedSsid = preferences.getString(PREF_KEY_SSID, "");
                String storedPass = preferences.getString(PREF_KEY_PASS, "");
                preferences.end();
                
                if (storedSsid.length() > 0) {
                    Serial.print("重新连接到: ");
                    Serial.println(storedSsid);
                    
                    currentStatus = WIFI_CONNECTING;
                    currentSsid = storedSsid;
                    currentPass = storedPass;
                    wifiConnectStartTime = millis();
                    wifiConnectAttempts = 0;
                    
                    WiFi.begin(currentSsid.c_str(), currentPass.c_str());
                } else {
                    Serial.println("\n无存储的WiFi凭证，进入配网模式...");
                    currentStatus = WIFI_NOT_CONFIGURED;
                    startConfigMode();
                }
            }
        }
    }
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool isInConfigMode() {
    return currentStatus == WIFI_CONFIG_MODE;
}

// 初始化Web服务器
void initWebServer(IPAddress apIP) {
    // 启动DNS服务器用于 captive portal
    dnsServer.start(53, "*", apIP);

    // 设置Web服务器路由
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/save", HTTP_POST, handleSave);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
}

void startConfigMode() {
    Serial.println("\n启动WiFi配置模式...");
    currentStatus = WIFI_CONFIG_MODE;

    // 配置模式下LED开始闪烁
    blinkLED(2, 300);

    boolean apStarted = WiFi.softAP(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);
    if (apStarted) {
        IPAddress apIP = WiFi.softAPIP();
        Serial.print("配置AP IP地址: ");
        Serial.println(apIP);

        // 初始化Web服务器
        initWebServer(apIP);

        Serial.println("请连接到 'ESP32_Config' AP 并访问 http://192.168.4.1");
    } else {
        Serial.println("配置AP启动失败！");
        // AP启动失败，LED快速闪烁5次
        blinkLED(5, 100);
    }
}

void handleRoot() {
    webServer.send(200, "text/html", configPage);
}

void handleSave() {
    String ssid = webServer.arg("ssid");
    String password = webServer.arg("password");
    String city = webServer.arg("city");

    Serial.println("收到新的网络配置信息:");
    Serial.print("WiFi名称: ");
    Serial.println(ssid);
    Serial.print("城市: ");
    Serial.println(city);

    preferences.begin(PREF_NAMESPACE, false);
    preferences.putString(PREF_KEY_SSID, ssid);
    preferences.putString(PREF_KEY_PASS, password);
    if (city.length() > 0) {
        preferences.putString("city", city);
    }
    preferences.end();

    String response = "<html><body><h2>配置保存成功!</h2><p>设备将重启并尝试连接到新网络。</p></body></html>";
    webServer.send(200, "text/html", response);

    delay(1000);
    ESP.restart();
}

void handleNotFound() {
    // 重定向所有其他请求到根页面 (captive portal)
    webServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    webServer.send(302, "text/plain", "");
}

// 其他从.h文件声明但未完全实现的函数
String getWiFiSSID() {
    if (isWiFiConnected()) {
        return WiFi.SSID();
    }
    return "";
}

String getLocalIP() {
    if (isWiFiConnected()) {
        return WiFi.localIP().toString();
    }
    return "";
}

void stopConfigMode() {
    // 未实现，因为我们会重启
}

void resetWiFiConfig() {
    preferences.begin(PREF_NAMESPACE, false);
    preferences.clear();
    preferences.end();
    Serial.println("WiFi配置已重置。重启中...");
    delay(500);
    ESP.restart();
}

void setLEDCallback(void (*callback)(int state)) {
    ledCallback = callback;
}

// LED控制管理
void updateLEDState(WiFiStatus status) {
    unsigned long currentTime = millis();
    
    switch (status) {
        case WIFI_CONFIG_MODE:
            // 配置模式：快闪（200ms间隔）
            if (currentTime - lastLEDBlinkTime > 200) {
                lastLEDBlinkTime = currentTime;
                ledState = !ledState;
                setLED(ledState ? HIGH : LOW);
            }
            break;
        case WIFI_CONNECTING:
            // 尝试连接：慢闪（500ms间隔）
            if (currentTime - lastLEDBlinkTime > 500) {
                lastLEDBlinkTime = currentTime;
                ledState = !ledState;
                setLED(ledState ? HIGH : LOW);
            }
            break;
        case WIFI_CONNECTED:
            // 连接成功：常亮
            setLED(HIGH);
            break;
        case WIFI_FAILED:
        case WIFI_NOT_CONFIGURED:
            // 其他状态：关闭LED
            setLED(LOW);
            break;
    }
}

// 控制LED状态
void setLED(int state) {
    digitalWrite(LED_PIN, state);
    if (ledCallback) {
        ledCallback(state);
    }
}

// 闪烁LED
void blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        setLED(HIGH);
        delay(delayMs);
        setLED(LOW);
        if (i < times - 1) {
            delay(delayMs);
        }
    }
}
