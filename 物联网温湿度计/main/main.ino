#include <ESP8266WiFi.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <ArduinoMqttClient.h>
#include <time.h>
#include <WiFiUdp.h>

#include "main_private.h"

// DHT11 接 4脚
DHT dht4(4, 11);
// SCL 接 0 脚，SDA 接 2 脚
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,  0, 2, U8X8_PIN_NONE);
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// MQTT 配置
const char* broker = BROKER;        // broker地址
int port = 1883;                    // 服务端口
const char* topic = TOPIC;          // 主题

// NTP 配置
const char* ntpServer = "pool.ntp.org";  // NTP 服务器
const long gmtOffset_sec = 8 * 3600;     // 东八区（北京时间）UTC+8
const int daylightOffset_sec = 0;        // 夏令时偏移（中国不使用夏令时）

// 定义业务变量
volatile float temperature; // 温度
volatile float humidity;    // 湿度
volatile int wifi_status;   // WIFI连接状态
volatile bool timeSynced = false; // NTP时间同步状态

// u8g2显示函数
void page1() {
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.setFontPosTop();
  u8g2.setCursor(8,6);
  u8g2.printf("温度：%.1f", temperature);
  u8g2.setCursor(8,26);
  u8g2.printf("湿度：%.1f", humidity);
  u8g2.setCursor(8,46);
  if (wifi_status == WL_CONNECTED && timeSynced) {
    u8g2.printf("WIFI/校时：ON √");
  } else if (wifi_status == WL_CONNECTED && !timeSynced) {
    u8g2.printf("WIFI/校时：ON ×");
  } else {
    u8g2.printf("WIFI/校时：OFF ×");
  }
}

// 获取当前时间戳（Unix时间戳，秒）
unsigned long getTimestamp() {
  time_t now = time(nullptr);
  return (unsigned long)now;
}

// 同步NTP时间
void syncNTPTime() {
  if (WiFi.status() == WL_CONNECTED && !timeSynced) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // 等待时间同步（最多等待5秒）
    int retry = 0;
    while (time(nullptr) < 100000 && retry < 50) {
      delay(100);
      retry++;
    }
    if (time(nullptr) > 100000) {
      timeSynced = true;
      Serial.println("NTP时间同步成功");
    } else {
      Serial.println("NTP时间同步失败");
    }
  }
}

void setup(){
  // 显示器初始化
  u8g2.setI2CAddress(0x3C * 2); // i2c 地址 0x3c
  u8g2.begin();
  u8g2.enableUTF8Print();

  // 温湿度传感器初始化
  dht4.begin();

  // 串口初始化
  Serial.begin(9600);
  
  // 初始化NTP时间同步
  syncNTPTime();
}

void loop(){
  // 尝试链接WIFI 并作为采集间隔
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    timeSynced = false; // WIFI断开时重置时间同步状态
  }
  delay(10 * 1000);
  
  // WIFI 状态
  wifi_status = WiFi.status();
  
  // 如果WIFI已连接但时间未同步，尝试同步NTP时间
  if (WiFi.status() == WL_CONNECTED && !timeSynced) {
    syncNTPTime();
  }
  
  // 获得温湿度数据
  temperature = dht4.readTemperature();
  humidity = dht4.readHumidity();

  // 尝试连接MQTT发数据
  if (WiFi.status() == WL_CONNECTED) {
    if (mqttClient.connect(broker, port)) {
      mqttClient.beginMessage(topic, true);
      // 获取时间戳
      unsigned long timestamp = getTimestamp();
      // 发送包含时间戳的JSON数据
      mqttClient.printf("{\"temperature\":%.1f,\"humidity\":%.1f,\"timestamp\":%lu}", 
                        temperature, humidity, timestamp);
      mqttClient.endMessage();
    }
  }

  u8g2.firstPage();
  do {
    page1();
  } while (u8g2.nextPage());

  // 每隔1分钟上报一次
  delay(50 * 1000);
}