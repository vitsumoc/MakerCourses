/*
 * 梦中的婚礼 (Mariage d'Amour) - 6个无源蜂鸣器多声部演奏版本
 * 使用定时器中断实现真正的多声部同时播放
 * 引脚：2, 3, 4, 5, 6, 7
 * 
 * 注意：Arduino UNO的tone()函数只能同时在一个引脚上工作（使用Timer2）
 * 因此我们使用Timer1中断来手动控制多个引脚，实现真正的多声部
 */

#include <avr/interrupt.h>

// 定义蜂鸣器引脚数组
int buzzerPins[] = {2, 3, 4, 5, 6, 7};
int buzzerCount = 6;

// 蜂鸣器状态结构（用于定时器中断）
struct BuzzerState {
  unsigned int frequency;      // 当前频率（Hz）
  unsigned int periodHalf;    // 半周期计数（微秒/2）
  unsigned int counter;        // 当前计数器
  bool pinState;               // 当前引脚状态
  bool active;                 // 是否激活
};

BuzzerState buzzerStates[6];

// 演奏控制
unsigned long performanceStartTime = 0;
bool performanceStarted = false;

// 音符频率定义（单位：Hz）
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_B2  123
#define NOTE_REST 0

// 节拍定义
#define WHOLE_NOTE    1600
#define HALF_NOTE     800
#define QUARTER_NOTE  400
#define EIGHTH_NOTE   200
#define SIXTEENTH_NOTE 100

// 声部1：主旋律（高音）- 蜂鸣器 0, 1
// 梦中的婚礼主旋律（简化版，但保持原曲特色）
int melody1[][2] = {
  {NOTE_B4, QUARTER_NOTE}, {NOTE_A4, QUARTER_NOTE}, {NOTE_G4, QUARTER_NOTE}, {NOTE_F4, QUARTER_NOTE},
  {NOTE_E4, QUARTER_NOTE}, {NOTE_D4, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE}, {NOTE_B3, QUARTER_NOTE},
  {NOTE_B4, QUARTER_NOTE}, {NOTE_A4, QUARTER_NOTE}, {NOTE_G4, QUARTER_NOTE}, {NOTE_F4, QUARTER_NOTE},
  {NOTE_E4, QUARTER_NOTE}, {NOTE_D4, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE}, {NOTE_B3, QUARTER_NOTE},
  {NOTE_C5, EIGHTH_NOTE}, {NOTE_D5, EIGHTH_NOTE}, {NOTE_E5, EIGHTH_NOTE}, {NOTE_F5, EIGHTH_NOTE},
  {NOTE_G5, EIGHTH_NOTE}, {NOTE_A5, EIGHTH_NOTE}, {NOTE_B5, EIGHTH_NOTE}, {NOTE_C6, EIGHTH_NOTE},
  {NOTE_B4, QUARTER_NOTE}, {NOTE_A4, QUARTER_NOTE}, {NOTE_G4, QUARTER_NOTE}, {NOTE_F4, QUARTER_NOTE},
  {NOTE_E4, QUARTER_NOTE}, {NOTE_D4, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE}, {NOTE_B3, QUARTER_NOTE},
  {NOTE_C5, EIGHTH_NOTE}, {NOTE_D5, EIGHTH_NOTE}, {NOTE_E5, EIGHTH_NOTE}, {NOTE_F5, EIGHTH_NOTE},
  {NOTE_G5, EIGHTH_NOTE}, {NOTE_A5, EIGHTH_NOTE}, {NOTE_B5, EIGHTH_NOTE}, {NOTE_C6, EIGHTH_NOTE},
  {NOTE_B4, HALF_NOTE}, {NOTE_REST, QUARTER_NOTE}
};

// 声部2：和声（中音）- 蜂鸣器 2, 3
// 提供和声支持，与主旋律形成和声
int melody2[][2] = {
  {NOTE_D4, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE}, {NOTE_B3, QUARTER_NOTE}, {NOTE_A3, QUARTER_NOTE},
  {NOTE_G3, QUARTER_NOTE}, {NOTE_F3, QUARTER_NOTE}, {NOTE_E3, QUARTER_NOTE}, {NOTE_D3, QUARTER_NOTE},
  {NOTE_D4, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE}, {NOTE_B3, QUARTER_NOTE}, {NOTE_A3, QUARTER_NOTE},
  {NOTE_G3, QUARTER_NOTE}, {NOTE_F3, QUARTER_NOTE}, {NOTE_E3, QUARTER_NOTE}, {NOTE_D3, QUARTER_NOTE},
  {NOTE_E4, EIGHTH_NOTE}, {NOTE_F4, EIGHTH_NOTE}, {NOTE_G4, EIGHTH_NOTE}, {NOTE_A4, EIGHTH_NOTE},
  {NOTE_B4, EIGHTH_NOTE}, {NOTE_C5, EIGHTH_NOTE}, {NOTE_D5, EIGHTH_NOTE}, {NOTE_E5, EIGHTH_NOTE},
  {NOTE_D4, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE}, {NOTE_B3, QUARTER_NOTE}, {NOTE_A3, QUARTER_NOTE},
  {NOTE_G3, QUARTER_NOTE}, {NOTE_F3, QUARTER_NOTE}, {NOTE_E3, QUARTER_NOTE}, {NOTE_D3, QUARTER_NOTE},
  {NOTE_E4, EIGHTH_NOTE}, {NOTE_F4, EIGHTH_NOTE}, {NOTE_G4, EIGHTH_NOTE}, {NOTE_A4, EIGHTH_NOTE},
  {NOTE_B4, EIGHTH_NOTE}, {NOTE_C5, EIGHTH_NOTE}, {NOTE_D5, EIGHTH_NOTE}, {NOTE_E5, EIGHTH_NOTE},
  {NOTE_D4, HALF_NOTE}, {NOTE_REST, QUARTER_NOTE}
};

// 声部3：低音 - 蜂鸣器 4, 5
// 提供稳定的低音基础
int bass[][2] = {
  {NOTE_B2, HALF_NOTE}, {NOTE_B2, HALF_NOTE},
  {NOTE_B2, HALF_NOTE}, {NOTE_B2, HALF_NOTE},
  {NOTE_C3, QUARTER_NOTE}, {NOTE_D3, QUARTER_NOTE}, {NOTE_E3, QUARTER_NOTE}, {NOTE_F3, QUARTER_NOTE},
  {NOTE_G3, QUARTER_NOTE}, {NOTE_A3, QUARTER_NOTE}, {NOTE_B3, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE},
  {NOTE_B2, HALF_NOTE}, {NOTE_B2, HALF_NOTE},
  {NOTE_C3, QUARTER_NOTE}, {NOTE_D3, QUARTER_NOTE}, {NOTE_E3, QUARTER_NOTE}, {NOTE_F3, QUARTER_NOTE},
  {NOTE_G3, QUARTER_NOTE}, {NOTE_A3, QUARTER_NOTE}, {NOTE_B3, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE},
  {NOTE_B2, HALF_NOTE}, {NOTE_REST, QUARTER_NOTE}
};

int melody1Length = sizeof(melody1) / sizeof(melody1[0]);
int melody2Length = sizeof(melody2) / sizeof(melody2[0]);
int bassLength = sizeof(bass) / sizeof(bass[0]);

int* melodies[] = {(int*)melody1, (int*)melody1, (int*)melody2, (int*)melody2, (int*)bass, (int*)bass};
int melodyLengths[] = {melody1Length, melody1Length, melody2Length, melody2Length, bassLength, bassLength};

// 音符跟踪
int currentNoteIndex[6] = {0, 0, 0, 0, 0, 0};
unsigned long noteStartTime[6] = {0, 0, 0, 0, 0, 0};
unsigned long maxDuration = 0;  // 缓存最长声部的总时长
bool maxDurationCalculated = false;

// Timer1中断服务程序 - 用于产生多声部音调
// 优化：使用直接端口操作以提高速度，减少中断执行时间
// 注意：引脚2-7都在PORTD上，所以只需要操作PORTD
// 使用位操作来避免读取整个PORTD寄存器，减少竞争条件
ISR(TIMER1_COMPA_vect) {
  // 引脚2-7都在PORTD上，使用位操作直接修改
  for (int i = 0; i < buzzerCount; i++) {
    if (buzzerStates[i].active && buzzerStates[i].frequency > 0) {
      buzzerStates[i].counter++;
      if (buzzerStates[i].counter >= buzzerStates[i].periodHalf) {
        buzzerStates[i].counter = 0;
        buzzerStates[i].pinState = !buzzerStates[i].pinState;
        
        // 引脚2-7都在PORTD上，使用位操作直接修改（不读取整个寄存器）
        uint8_t pin = buzzerPins[i];
        uint8_t bit = (1 << pin);
        if (buzzerStates[i].pinState) {
          PORTD |= bit;   // 设置位
        } else {
          PORTD &= ~bit;  // 清除位
        }
      }
    }
  }
}

// 设置蜂鸣器频率
void setBuzzerFrequency(int index, unsigned int frequency) {
  if (frequency == 0 || frequency == NOTE_REST) {
    buzzerStates[index].active = false;
    buzzerStates[index].frequency = 0;
    digitalWrite(buzzerPins[index], LOW);
  } else {
    buzzerStates[index].active = true;
    buzzerStates[index].frequency = frequency;
    // 计算半周期计数：
    // 中断频率 = 10kHz = 10000次/秒，每次中断间隔 = 1/10000秒 = 100微秒
    // 频率f的周期 = 1/f秒，半周期 = 1/(2*f)秒
    // 半周期计数 = (1/(2*f)) / (1/10000) = 10000 / (2*f) = 5000 / f
    buzzerStates[index].periodHalf = 5000UL / frequency;
    buzzerStates[index].counter = 0;
    buzzerStates[index].pinState = false;
  }
}

// 获取指定时间点应该播放的音符频率
int getNoteAtTime(int* melody, int length, unsigned long elapsedTime) {
  unsigned long accumulatedTime = 0;
  
  for (int i = 0; i < length; i++) {
    int duration = melody[i * 2 + 1];
    
    if (elapsedTime < accumulatedTime + duration) {
      return melody[i * 2];
    }
    
    accumulatedTime += duration;
  }
  
  return NOTE_REST;
}

// 更新所有蜂鸣器的音符
void updateAllBuzzers() {
  if (!performanceStarted) {
    return;
  }
  
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - performanceStartTime;
  
  // 同时更新所有蜂鸣器
  for (int i = 0; i < buzzerCount; i++) {
    int* melody = melodies[i];
    int length = melodyLengths[i];
    
    // 获取当前时间点应该播放的音符
    int targetFrequency = getNoteAtTime(melody, length, elapsed);
    
    // 计算当前应该播放的音符索引
    unsigned long accTime = 0;
    int targetNoteIndex = length;
    for (int j = 0; j < length; j++) {
      if (elapsed < accTime + melody[j * 2 + 1]) {
        targetNoteIndex = j;
        break;
      }
      accTime += melody[j * 2 + 1];
    }
    
    // 如果音符改变了，更新频率
    if (currentNoteIndex[i] != targetNoteIndex) {
      setBuzzerFrequency(i, targetFrequency);
      currentNoteIndex[i] = targetNoteIndex;
      noteStartTime[i] = currentTime;
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("梦中的婚礼多声部演奏程序启动（使用Timer1中断）");
  
  // 初始化所有蜂鸣器引脚
  for (int i = 0; i < buzzerCount; i++) {
    pinMode(buzzerPins[i], OUTPUT);
    digitalWrite(buzzerPins[i], LOW);
    buzzerStates[i].active = false;
    buzzerStates[i].frequency = 0;
    buzzerStates[i].counter = 0;
    buzzerStates[i].pinState = false;
  }
  
  // 配置Timer1用于产生多声部音调
  // 使用CTC模式，每100微秒触发一次中断（10kHz）
  // 降低中断频率以减少CPU负载，避免看门狗定时器重启
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  // 16MHz / 8分频 = 2MHz，2MHz / 10kHz = 200，所以OCR1A = 200 - 1 = 199
  OCR1A = 199;
  TCCR1B |= (1 << WGM12);  // CTC模式
  TCCR1B |= (1 << CS11);    // 8分频
  TIMSK1 |= (1 << OCIE1A);  // 启用比较匹配中断
  interrupts();
  
  delay(1000);
  Serial.println("开始多声部演奏...");
  Serial.println("蜂鸣器分配：");
  Serial.println("  0,1: 主旋律（高音）");
  Serial.println("  2,3: 和声（中音）");
  Serial.println("  4,5: 低音");
}

void loop() {
  // 如果还没有开始演奏，启动演奏
  if (!performanceStarted) {
    performanceStartTime = millis();
    performanceStarted = true;
    Serial.println("开始多声部同步演奏...");
    
    // 初始化所有音符索引
    for (int i = 0; i < buzzerCount; i++) {
      currentNoteIndex[i] = -1;
      noteStartTime[i] = 0;
    }
  }
  
  // 更新所有蜂鸣器的音符
  updateAllBuzzers();
  
  // 检查是否所有声部都演奏完毕
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - performanceStartTime;
  
  // 计算最长声部的总时长（只计算一次）
  if (!maxDurationCalculated) {
    for (int i = 0; i < buzzerCount; i++) {
      int* melody = melodies[i];
      int length = melodyLengths[i];
      unsigned long total = 0;
      for (int j = 0; j < length; j++) {
        total += melody[j * 2 + 1];
      }
      if (total > maxDuration) {
        maxDuration = total;
      }
    }
    maxDurationCalculated = true;
  }
  
  // 如果演奏时间超过最长声部的时长，结束演奏
  if (elapsed >= maxDuration) {
    Serial.println("\n一轮多声部演奏完成，等待3秒后重新开始...\n");
    
    // 停止所有蜂鸣器
    for (int i = 0; i < buzzerCount; i++) {
      setBuzzerFrequency(i, 0);
    }
    
    delay(3000);
    
    // 重置演奏状态
    performanceStarted = false;
    maxDurationCalculated = false;
    maxDuration = 0;
    
    // 重置所有音符索引
    for (int i = 0; i < buzzerCount; i++) {
      currentNoteIndex[i] = 0;
      noteStartTime[i] = 0;
    }
  }
  
  // 增加延迟，确保主循环有足够时间执行，避免看门狗定时器重启
  delay(20);
}

