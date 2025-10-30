#include <ESP8266WiFi.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <ArduinoMqttClient.h>

DHT dht4(4, 11);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,  0, 2, U8X8_PIN_NONE);
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "bemfa.com";
int        port     = 9501;
const char topic[]  = "wenshidu01II004";

volatile float temperature;
volatile float humidity;
volatile int wifi_status;

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
  u8g2.setI2CAddress(0x3C * 2);
  u8g2.begin();
  u8g2.enableUTF8Print();

  // 温湿度传感器初始化
  dht4.begin();
  Serial.begin(9600);

  // MQTT客户端初始化
  mqttClient.setId("30aa96063666414fa36879cd4535e615");
}

void loop(){
  // 尝试链接WIFI 并作为采集间隔
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin("xingxingcode", "daxingxing");
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
      mqttClient.printf("#%.1f#%.1f#", temperature, humidity);
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