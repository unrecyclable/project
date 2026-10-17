/*******************************************************************************
 * GT911/XPT2046 + RGB LCD (800x480)
 * 功能：简易计算器，可以实现简单的加减乘除
 * 硬件：ESP32-S3 + 800x480 RGB LCD + TP
 ******************************************************************************/
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#include <cmath>

extern Arduino_RGB_Display *gfx;

#include "touch.h"

// 硬件配置
#define LCD_WIDTH 800
#define LCD_HEIGHT 480
#define TFT_BL 2  // 背光控制引脚

// 屏幕引脚定义
#define PIN_DE 40
#define PIN_VSYNC 41
#define PIN_HSYNC 39
#define PIN_PCLK 42
#define PIN_R0 45
#define PIN_R1 48
#define PIN_R2 47
#define PIN_R3 21
#define PIN_R4 14
#define PIN_G0 5
#define PIN_G1 6
#define PIN_G2 7
#define PIN_G3 15
#define PIN_G4 16
#define PIN_G5 4
#define PIN_B0 8
#define PIN_B1 3
#define PIN_B2 46
#define PIN_B3 9
#define PIN_B4 1

// 时序参数
#define HSYNC_POLARITY 0
#define HSYNC_FRONT_PORCH 12
#define HSYNC_PULSE_WIDTH 4
#define HSYNC_BACK_PORCH 16
#define VSYNC_POLARITY 0
#define VSYNC_FRONT_PORCH 10
#define VSYNC_PULSE_WIDTH 4
#define VSYNC_BACK_PORCH 10
#define PCLK_FREQ 13300000
#define PCLK_ACTIVE_NEG 1

// 计算器按钮配置
#define BUTTON_ROWS 5
#define BUTTON_COLS 4
#define BUTTON_MARGIN 8
#define BUTTON_WIDTH (LCD_WIDTH - (BUTTON_COLS + 1) * BUTTON_MARGIN) / BUTTON_COLS
#define BUTTON_HEIGHT (LCD_HEIGHT - 140 - (BUTTON_ROWS + 1) * BUTTON_MARGIN) / BUTTON_ROWS
#define DISPLAY_Y 20
#define DISPLAY_HEIGHT 100
// 显示区的边界定义
#define DISPLAY_X_START BUTTON_MARGIN
#define DISPLAY_X_END (LCD_WIDTH - BUTTON_MARGIN)
#define DISPLAY_Y_START DISPLAY_Y
#define DISPLAY_Y_END (DISPLAY_Y + DISPLAY_HEIGHT)

// 按钮布局与标签
const char *buttonLabels[BUTTON_ROWS][BUTTON_COLS] = {
  {"C", "<", "%", "/"},
  {"7", "8", "9", "x"},
  {"4", "5", "6", "-"},
  {"1", "2", "3", "+"},
  {"±", "0", ".", "="}
};

// 按钮颜色配置
uint16_t buttonColors[BUTTON_ROWS][BUTTON_COLS] = {
  {RGB565_RED, RGB565_OLIVE, RGB565_CYAN, RGB565_NAVY},
  {RGB565_DARKGREY, RGB565_DARKGREY, RGB565_DARKGREY, RGB565_NAVY},
  {RGB565_DARKGREY, RGB565_DARKGREY, RGB565_DARKGREY, RGB565_NAVY},
  {RGB565_DARKGREY, RGB565_DARKGREY, RGB565_DARKGREY, RGB565_NAVY},
  {RGB565_DARKGREY, RGB565_DARKGREY, RGB565_DARKGREY, RGB565_GREEN}
};

// 全局对象与变量
Arduino_ESP32RGBPanel *rgb_panel;
Arduino_RGB_Display *gfx;
String displayText = "0";
double num1 = 0, num2 = 0;
char op = ' ';
bool newNumber = true;  // 是否开始输入新数字
bool isTouching = false;
// 触摸防抖相关变量
unsigned long lastTouchTime = 0;
const unsigned long TOUCH_DEBOUNCE_DELAY = 200; // 触摸防抖延时200ms

//判断字符串是否包含指定字符
bool stringContains(const String &str, const char *substr) {
  return str.indexOf(substr) != -1;
}

// 初始化显示与
void setupDisplay() {
  rgb_panel = new Arduino_ESP32RGBPanel(
    PIN_DE, PIN_VSYNC, PIN_HSYNC, PIN_PCLK,
    PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4,
    PIN_G0, PIN_G1, PIN_G2, PIN_G3, PIN_G4, PIN_G5,
    PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B4,
    HSYNC_POLARITY, HSYNC_FRONT_PORCH, HSYNC_PULSE_WIDTH, HSYNC_BACK_PORCH,
    VSYNC_POLARITY, VSYNC_FRONT_PORCH, VSYNC_PULSE_WIDTH, VSYNC_BACK_PORCH,
    PCLK_ACTIVE_NEG, PCLK_FREQ);
  
  gfx = new Arduino_RGB_Display(LCD_WIDTH, LCD_HEIGHT, rgb_panel, 0, true);
  gfx->begin();
  gfx->fillScreen(RGB565_BLACK);
  gfx->setUTF8Print(true);
  
  // 初始化背光
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 首次绘制整个计算器界面
  drawCalculatorFull();
}

// 绘制整个计算器界面（仅在初始化时调用）
void drawCalculatorFull() {
  // 1. 清空整个屏幕
  gfx->fillScreen(RGB565_BLACK);

  // 2. 绘制显示区域背景
  gfx->fillRect(DISPLAY_X_START, DISPLAY_Y_START, 
                DISPLAY_X_END - DISPLAY_X_START, DISPLAY_Y_END - DISPLAY_Y_START, 
                RGB565_DARKGREY);
  gfx->drawRect(DISPLAY_X_START, DISPLAY_Y_START, 
                DISPLAY_X_END - DISPLAY_X_START, DISPLAY_Y_END - DISPLAY_Y_START, 
                RGB565_WHITE);
  
  // 3. 绘制按钮
  for (int row = 0; row < BUTTON_ROWS; row++) {
    for (int col = 0; col < BUTTON_COLS; col++) {
      int x = BUTTON_MARGIN + col * (BUTTON_WIDTH + BUTTON_MARGIN);
      int y = DISPLAY_Y_END + BUTTON_MARGIN + row * (BUTTON_HEIGHT + BUTTON_MARGIN);
      
      // 绘制按钮背景
      gfx->fillRoundRect(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, 5, buttonColors[row][col]);
      gfx->drawRoundRect(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, 5, RGB565_WHITE);
      
      // 绘制按钮文本（居中）
      gfx->setTextColor(RGB565_WHITE);
      gfx->setFont(u8g2_font_unifont_t_gb2312);
      gfx->setTextSize(3);
      
      // 手动计算文本居中位置
      int labelLen = strlen(buttonLabels[row][col]);
      int labelCharWidth = 12 * 3; // 字号3的字符宽度
      int labelWidth = labelLen * labelCharWidth;
      int labelX = x + (BUTTON_WIDTH - labelWidth) / 2;
      int labelY = y + (BUTTON_HEIGHT / 2) + 10;
      gfx->setCursor(labelX, labelY);
      gfx->print(buttonLabels[row][col]);
    }
  }

  // 首次绘制显示区的数字
  drawDisplayAreaOnly();
}

// 仅刷新数字显示区域
void drawDisplayAreaOnly() {
  // 1. 清空显示区域的旧内容
  gfx->fillRect(DISPLAY_X_START + 5, DISPLAY_Y_START + 5, 
                DISPLAY_X_END - DISPLAY_X_START - 10, DISPLAY_Y_END - DISPLAY_Y_START - 10, 
                RGB565_DARKGREY);
  
  // 2. 显示文本
  gfx->setTextColor(RGB565_WHITE);
  gfx->setFont(u8g2_font_unifont_t_gb2312);
  gfx->setTextSize(4);
  
  int charWidth = 12; 
  int textWidth = displayText.length() * charWidth * 4; 
  int x = max(DISPLAY_X_START + 10, (DISPLAY_X_END - textWidth) - 10);
  int y = DISPLAY_Y_START + (DISPLAY_HEIGHT / 2) + 12; 
  
  // 限制显示字符数
  String showText = displayText.substring(0, 12);
  gfx->setCursor(x, y);
  gfx->print(showText);
}

// 处理数字输入
void handleNumberInput(char num) {
  if (newNumber) {
    displayText = String(num);
    newNumber = false;
  } else {
    if (displayText == "-") {
      displayText += num;
    } else if (displayText.length() < 12) {  // 限制最大位数到12位
      displayText += num;
    }
  }
  drawDisplayAreaOnly();
}

// 处理负号输入的函数
void handleNegativeSign() {
  if (newNumber) {
    // 新数字输入阶段，直接显示'-'
    displayText = "-";
    newNumber = false; // 标记为非新数字，后续输入数字直接拼接
  } else {
    // 非新数字阶段：仅当当前显示为0时，切换为'-'
    if (displayText == "0") {
      displayText = "-";
    }
  }
  // 刷新显示区
  drawDisplayAreaOnly();
}

// 单独的计算函数
double calculateResult(double n1, double n2, char operation) {
  switch (operation) {
    case '+': return n1 + n2;
    case '-': return n1 - n2;
    case 'x': return n1 * n2;
    case '/': return (n2 != 0) ? n1 / n2 : 0; // 除零保护
    case '%': return fmod(n1, n2);
    default: return n1;
  }
}

// 处理运算符（刷新显示区）
void handleOperator(char operation) {
  if (op == ' ') {
    // 首次选择运算符，记录第一个数（兼容负数转换）
    num1 = displayText.toDouble();
    op = operation;
    newNumber = true;
  } else {
    // 连续选择运算符，仅更新运算符（不计算）
    op = operation;
    newNumber = true;
  }
  // 刷新显示区
  drawDisplayAreaOnly();
}

// 处理特殊功能（刷新显示区）
void handleFunction(char func) {
  switch (func) {
    case 'C':  // 清除
      displayText = "0";
      num1 = 0;
      num2 = 0;
      op = ' ';
      newNumber = true;
      // 仅刷新显示区
      drawDisplayAreaOnly();
      break;
    case '<':  // 退格（适配负号：如果是单独的'-'，退格后回到0）
      if (displayText.length() > 1) {
        displayText = displayText.substring(0, displayText.length() - 1);
      } else {
        // 如果是单独的'-'，退格后重置为0
        displayText = "0";
        newNumber = true;
      }
      // 仅刷新显示区
      drawDisplayAreaOnly();
      break;
    case '±':  // 正负号：直接输出'-'
      handleNegativeSign();
      break;
    case '.':  // 小数点
       if (!stringContains(displayText, ".")) {
        // 如果当前是单独的'-'，直接拼接小数点
        if (displayText == "-") {
          displayText += ".";
        } else {
          displayText += ".";
        }
        newNumber = false;
      }
      // 仅刷新显示区
      drawDisplayAreaOnly();
      break;
    case '=':  // 等于（仅此时执行计算）
      if (op != ' ') {
        num2 = displayText.toDouble();
        // 处理除零错误
        if (op == '/' && num2 == 0) {
          displayText = "错误";
        } else {
          double result = calculateResult(num1, num2, op);
          String temp = String(result, 6);
          temp.replace(".000000", "");
          if (temp.endsWith(".")) {
            temp = temp.substring(0, temp.length() - 1);
          }
          displayText = temp;
        }
        // 计算后重置状态，准备新的计算
        op = ' ';
        newNumber = true;
      }
      // 仅刷新显示区
      drawDisplayAreaOnly();
      break;
    case '%': // 取模运算符，归为运算符处理
      handleOperator(func);
      break;
  }
}

// 检测触摸并处理（增加防抖，解决多次触发问题）
void handleTouchInput() {
  if (touch_has_signal()) {
    if (touch_touched() && !isTouching) {
      // 防抖
      unsigned long currentTime = millis();
      if (currentTime - lastTouchTime < TOUCH_DEBOUNCE_DELAY) {
        return; 
      }
      lastTouchTime = currentTime; // 更新上次触摸时间
      
      isTouching = true;
      int x = touch_last_x;
      int y = touch_last_y;
      
      // 检查是否触摸到按钮区域
      int buttonStartY = DISPLAY_Y_END + BUTTON_MARGIN;
      if (y >= buttonStartY) {
        // 计算触摸的按钮行列
        int row = (y - buttonStartY) / (BUTTON_HEIGHT + BUTTON_MARGIN);
        int col = (x - BUTTON_MARGIN) / (BUTTON_WIDTH + BUTTON_MARGIN);
        
        // 严格检查行列有效性
        if (row >= 0 && row < BUTTON_ROWS && col >= 0 && col < BUTTON_COLS) {
          String label = buttonLabels[row][col];
          
          // 判断按钮类型并处理
          if (label >= "0" && label <= "9") {
            handleNumberInput(label[0]);
          } else if (label == "+" || label == "-" || label == "x" || label == "/") {
            handleOperator(label[0]);
          } else {
            handleFunction(label[0]);
          }
        }
      }
    } else if (!touch_touched() && isTouching) {
      isTouching = false;
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupDisplay();
  touch_init();
}

void loop() {
  handleTouchInput();
  delay(10);
}