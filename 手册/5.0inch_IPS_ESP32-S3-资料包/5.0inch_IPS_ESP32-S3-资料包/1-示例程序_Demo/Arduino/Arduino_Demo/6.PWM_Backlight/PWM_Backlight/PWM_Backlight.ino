/*******************************************************************************
 * GT911/XPT2046 + RGB LCD (800x480)
 * 功能：PWM调光
 * 硬件：ESP32-S3 + 800x480 RGB LCD + TP
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <cmath>

extern Arduino_RGB_Display *gfx;

#include "touch.h"

// ====================== 硬件配置 ======================
#define TFT_BL 2         // 背光控制引脚
#define LCD_WIDTH 800    // 屏幕宽度
#define LCD_HEIGHT 480   // 屏幕高度

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

// 引脚定义
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

// 滑块配置
#define SLIDER_HEIGHT 50        // 滑块高度
#define SLIDER_Y (LCD_HEIGHT / 2 - SLIDER_HEIGHT / 2)  // 滑块Y坐标（居中）
#define SLIDER_MARGIN 50        // 滑块左右边距

// 显示面板初始化
Arduino_ESP32RGBPanel *rgb_panel = new Arduino_ESP32RGBPanel(
  PIN_DE, PIN_VSYNC, PIN_HSYNC, PIN_PCLK,
  PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4,
  PIN_G0, PIN_G1, PIN_G2, PIN_G3, PIN_G4, PIN_G5,
  PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B4,
  HSYNC_POLARITY, HSYNC_FRONT_PORCH, HSYNC_PULSE_WIDTH, HSYNC_BACK_PORCH,
  VSYNC_POLARITY, VSYNC_FRONT_PORCH, VSYNC_PULSE_WIDTH, VSYNC_BACK_PORCH,
  PCLK_ACTIVE_NEG, PCLK_FREQ);

// 全局显示对象（与touch.h中的extern声明对应）
Arduino_RGB_Display *gfx;

// 全局变量
int brightness = 128;        // 初始亮度(0-255)
bool isDragging = false;     // 滑块拖动状态

// 计算字符串宽度
int calculateTextWidth(const char* text, uint8_t textSize) {
  return strlen(text) * 8 * textSize; 
}
// 绘制滑块界面
void drawSlider() {
  // 绘制白色背景
  gfx->fillScreen(WHITE);
  
  // 绘制滑块轨道（深灰色）
  gfx->fillRect(
    SLIDER_MARGIN, SLIDER_Y, 
    LCD_WIDTH - 2 * SLIDER_MARGIN, SLIDER_HEIGHT, 
    0x8410  // 深灰色
  );
  
  // 计算滑块X坐标
  int sliderX = map(brightness, 0, 255, 
                    SLIDER_MARGIN, 
                    LCD_WIDTH - SLIDER_MARGIN - 30);
  
  // 绘制滑块按钮（蓝色）
  gfx->fillRoundRect(
    sliderX, SLIDER_Y - 10, 
    30, SLIDER_HEIGHT + 20, 15, 
    0x001F  // 蓝色
  );
  
  // 显示亮度值（黑色文字）
  gfx->setTextColor(BLACK);
  gfx->setTextSize(3);
  char text[20];
  sprintf(text, "Backlight: %d%%", map(brightness, 0, 255, 0, 100));
  int textWidth = calculateTextWidth(text, 3); 
  gfx->setCursor((LCD_WIDTH - textWidth) / 2, SLIDER_Y - 40);
  gfx->print(text);
}

// 处理触摸输入
void handleTouch() {
  if (touch_has_signal()) {
    if (touch_touched()) {
      int x = touch_last_x;
      int y = touch_last_y;
      
      // 检查是否触摸到滑块区域
      if (y >= SLIDER_Y - 10 && y <= SLIDER_Y + SLIDER_HEIGHT + 10 &&
          x >= SLIDER_MARGIN && x <= LCD_WIDTH - SLIDER_MARGIN) {
        isDragging = true;
      }
      
      // 拖动过程中更新亮度
      if (isDragging) {
        // 限制x坐标在有效范围内
        x = constrain(x, SLIDER_MARGIN, LCD_WIDTH - SLIDER_MARGIN - 30);
        // 将x坐标映射为亮度值(0-255)
        brightness = map(x, SLIDER_MARGIN, LCD_WIDTH - SLIDER_MARGIN - 30, 0, 255);
        // 更新背光PWM
        analogWrite(TFT_BL, brightness);
        // 重绘界面
        drawSlider();
      }
    } else {
      // 触摸释放时结束拖动
      isDragging = false;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("背光亮度调节示例");

  // 初始化显示屏
  gfx = new Arduino_RGB_Display(LCD_WIDTH, LCD_HEIGHT, rgb_panel, 0, true);
  gfx->begin();
  gfx->fillScreen(BLACK);  // 初始黑屏
  
  // 初始化背光
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, brightness);  // 初始化亮度

  // 初始化触摸
  touch_init();
  Serial.println("触摸初始化完成");
  
  // 绘制初始界面
  drawSlider();
}

void loop() {
  handleTouch();  // 处理触摸输入
  delay(10);  // 适当延时，避免频繁刷新
}