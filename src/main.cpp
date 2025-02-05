#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

// WiFi & Firebase 配置信息
const char* ssid = "Verizon_XJ3K6K";
const char* password = "row-gray7-stave";
#define DATABASE_SECRET "AIzaSyDlubGe0gj2T2JZzInPVnozmO9yHcZ_awY"
#define DATABASE_URL "https://console.firebase.google.com/u/0/project/power-management-yushi/database/power-management-yushi-default-rtdb/data/~2F"

// Firebase 相关对象
WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

// 超声波传感器引脚
#define TRIG_PIN 2
#define ECHO_PIN 3

// 计时变量
unsigned long previousMillis = 0;
const long wifiConnectTime = 30000;      // WiFi 连接 30 秒
const long firebaseUploadTime = 30000;   // Firebase 上传数据 30 秒
const long ultrasonicTime = 12000;       // 仅超声波测距 12 秒
const long idleTime = 12000;             // 空闲 12 秒
const long deepSleepTime = 12000;        // 深度睡眠 12 秒

// **提前声明 measureDistance()**
float measureDistance();

void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    Serial.println("Starting ESP32...");
}

void loop() {
    unsigned long currentMillis = millis();

    // **阶段 1：连接 WiFi（30 秒）**
    previousMillis = currentMillis;
    while (millis() - previousMillis < wifiConnectTime) {
        Serial.println("Connecting to WiFi...");
        WiFi.begin(ssid, password);

        int retry = 0;
        while (WiFi.status() != WL_CONNECTED && retry < 10) {  
            Serial.print(".");
            delay(1000);
            retry++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi Connected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("\nWiFi Connection FAILED!");
        }
    }

    delay(100); // 小延时切换阶段

    // **阶段 2：发送数据到 Firebase（30 秒）**
    previousMillis = millis();
    while (millis() - previousMillis < firebaseUploadTime) {
        Serial.println("Uploading data to Firebase...");

        ssl.setInsecure();
        initializeApp(client, app, getAuth(dbSecret));
        app.getApp<RealtimeDatabase>(Database);
        Database.url(DATABASE_URL);
        client.setAsyncResult(result);

        float distance = measureDistance();  // ✅ 现在 measureDistance() 可用了
        bool status = Database.set<float>(client, "/sensor/distance", distance);
        if (status)
            Serial.println("Data uploaded to Firebase!");
        else
            Serial.println("Failed to upload data.");

        delay(5000);  // 每 5 秒上传一次数据
    }

    WiFi.disconnect(true); // true to disable auto-reconnect
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi Disconnected and turned OFF.");

    delay(100);

    // **阶段 3：超声波测距（12 秒）**
    previousMillis = millis();
    while (millis() - previousMillis < ultrasonicTime) {
        Serial.println("Measuring distance...");
        float distance = measureDistance();
        Serial.print("Measured Distance: ");
        Serial.print(distance);
        Serial.println(" cm");
        delay(1000);
    }

    delay(100);

    // **阶段 4：空闲（12 秒）**
    previousMillis = millis();
    while (millis() - previousMillis < idleTime) {
        Serial.println("Idle mode...");
        delay(1000);
    }

    delay(100);

    // **阶段 5：深度睡眠（12 秒）**
    Serial.println("Entering Deep Sleep mode...");
    esp_sleep_enable_timer_wakeup(deepSleepTime * 1000); // 12 秒后唤醒
    esp_deep_sleep_start();
}

// **定义 measureDistance()**
float measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    return duration * 0.034 / 2;  // 计算距离（单位：cm）
}