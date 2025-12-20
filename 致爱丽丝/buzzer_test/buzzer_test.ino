/*
 * 无源蜂鸣器测试程序
 * 引脚：2, 3, 4, 5, 6, 7
 * 功能：让6个蜂鸣器逐个响起
 */

// 定义蜂鸣器引脚数组
int buzzerPins[] = {2, 3, 4, 5, 6, 7};
int buzzerCount = 6;  // 蜂鸣器数量

// 测试频率（Hz）- 可以调整这个值来改变音调
int frequency = 1000;  // 1000Hz，中音调

// 每个蜂鸣器响的时间（毫秒）
int duration = 500;  // 500毫秒

void setup() {
  // 初始化串口，用于调试
  Serial.begin(9600);
  Serial.println("蜂鸣器测试程序启动");
  
  // 初始化所有蜂鸣器引脚为输出模式
  for (int i = 0; i < buzzerCount; i++) {
    pinMode(buzzerPins[i], OUTPUT);
    Serial.print("初始化引脚 ");
    Serial.println(buzzerPins[i]);
  }
  
  delay(1000);  // 等待1秒后开始测试
}

void loop() {
  Serial.println("\n开始测试所有蜂鸣器...");
  
  // 逐个测试每个蜂鸣器
  for (int i = 0; i < buzzerCount; i++) {
    Serial.print("测试蜂鸣器 ");
    Serial.print(i + 1);
    Serial.print(" (引脚 ");
    Serial.print(buzzerPins[i]);
    Serial.println(")");
    
    // 播放指定频率的声音
    tone(buzzerPins[i], frequency);
    
    // 持续指定时间
    delay(duration);
    
    // 停止声音
    noTone(buzzerPins[i]);
    
    // 蜂鸣器之间的间隔
    delay(200);
  }
  
  Serial.println("一轮测试完成，等待3秒后继续...\n");
  delay(3000);  // 等待3秒后重新开始
}

