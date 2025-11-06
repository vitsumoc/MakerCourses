#include <ESP8266WiFi.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <ArduinoMqttClient.h>

#include "main_private.h"

// DHT11 接 4脚
DHT dht4(4, 11);
// SCL 接 0 脚，SDA 接 2 脚
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,  0, 2, U8X8_PIN_NONE);
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// MQTT 配置
const char broker[] = "8.154.32.180";        // borker地址
int        port     = 1883;               // 服务端口
const char topic[]  = "/wenshidu/1.0/vc";  // 主题

// 定义业务变量
volatile float temperature; // 温度
volatile float humidity;    // 湿度
volatile int wifi_status;   // WIFI连接状态

// u8g2显示函数
void page1() {
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.setFontPosTop();
  u8g2.setCursor(8,6);
  u8g2.printf("温度：%.1f", temperature);
  u8g2.setCursor(8,26);
  u8g2.printf("湿度：%.1f", humidity);
  u8g2.setCursor(8,46);
  if (wifi_status == WL_CONNECTED) {
    u8g2.printf("WIFI状态：ON");
  } else {
    u8g2.printf("WIFI状态：OFF");
  }
}

void setup(){
  // 显示器初始化
  u8g2.setI2CAddress(0x3C * 2); // i2c 地址 0x3c
  u8g2.begin();
  u8g2.enableUTF8Print();

  // 温湿度传感器初始化
  dht4.begin();

  // MQTT客户端初始化
  // mqttClient.setId(BAFA_KEY);

  // 串口初始化
  Serial.begin(9600);
}

void loop(){
  // 尝试链接WIFI 并作为采集间隔
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
  delay(10 * 1000);
  
  // WIFI 状态
  wifi_status = WiFi.status();
  // 获得温湿度数据
  temperature = dht4.readTemperature();
  humidity = dht4.readHumidity();

  // 尝试连接MQTT发数据
  if (WiFi.status() == WL_CONNECTED) {
    if (mqttClient.connect(broker, port)) {
      mqttClient.beginMessage(topic);
      mqttClient.printf("{\"temperature\":%.1f,\"humidity\":%.1f}", temperature, humidity);
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