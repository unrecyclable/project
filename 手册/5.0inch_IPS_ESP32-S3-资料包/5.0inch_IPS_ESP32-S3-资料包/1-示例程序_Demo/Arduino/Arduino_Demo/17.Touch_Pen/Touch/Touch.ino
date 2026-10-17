/*******************************************************************************
 * GT911/XPT2046 + RGB LCD (800x480)
 * 功能：完全不重置坐标（仅清屏时重置），强制画线
 ******************************************************************************/
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>

// 全局GFX对象
Arduino_RGB_Display *gfx;
#include "touch.h"

// 硬件配置（不变）
#define LCD_WIDTH 800
#define LCD_HEIGHT 480
#define TFT_BL 2

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

// 绘画配置
#define PEN_COLOR RGB565_RED
#define PEN_WIDTH 3  // 加粗到3，确保看得见
#define CLEAR_AREA_X 10
#define CLEAR_AREA_Y 10
#define CLEAR_AREA_W 80
#define CLEAR_AREA_H 30

// 核心变量：仅保留上一帧坐标，永不主动重置（除了清屏）
int prev_x = -1, prev_y = -1;
int loop_counter=0;

// 初始化显示（不变）
void init_display() {
  Arduino_ESP32RGBPanel *rgb_panel = new Arduino_ESP32RGBPanel(
    PIN_DE, PIN_VSYNC, PIN_HSYNC, PIN_PCLK,
    PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4,
    PIN_G0, PIN_G1, PIN_G2, PIN_G3, PIN_G4, PIN_G5,
    PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B4,
    HSYNC_POLARITY, HSYNC_FRONT_PORCH, HSYNC_PULSE_WIDTH, HSYNC_BACK_PORCH,
    VSYNC_POLARITY, VSYNC_FRONT_PORCH, VSYNC_PULSE_WIDTH, VSYNC_BACK_PORCH,
    PCLK_ACTIVE_NEG, PCLK_FREQ);

  gfx = new Arduino_RGB_Display(LCD_WIDTH, LCD_HEIGHT, rgb_panel, 0, true);
  gfx->begin();
  gfx->fillScreen(RGB565_WHITE);
  gfx->setUTF8Print(true);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 清屏按钮
  gfx->fillRoundRect(CLEAR_AREA_X, CLEAR_AREA_Y, CLEAR_AREA_W, CLEAR_AREA_H, 5, RGB565_DARKGREY);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setFont(u8g2_font_unifont_t_gb2312);
  gfx->setCursor(CLEAR_AREA_X + 20, CLEAR_AREA_Y + 20);
  gfx->print("清屏");
}

// 清屏函数（仅这里重置坐标）
void clear_screen() {
  gfx->fillScreen(RGB565_WHITE);
  gfx->fillRoundRect(CLEAR_AREA_X, CLEAR_AREA_Y, CLEAR_AREA_W, CLEAR_AREA_H, 5, RGB565_DARKGREY);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setCursor(CLEAR_AREA_X + 20, CLEAR_AREA_Y + 20);
  gfx->print("清屏");
  prev_x = -1;  // 仅清屏时重置
  prev_y = -1;
}

void setup() {
  Serial.begin(115200);
  init_display();
  touch_init();
  Serial.println("完全不重置坐标版启动");
}

void loop() {
  // 1. 读取当前触摸状态和坐标（只做这一次读取）
  bool is_touching = (touch_has_signal() && touch_touched());
  int curr_x = touch_last_x;
  int curr_y = touch_last_y;
  loop_counter++;

  // 2. 只有触摸有效时，才执行所有操作（绝不处理松开状态）
  if (is_touching) {
    // 清屏按钮检测（仅这里触发重置）
    if (curr_x >= CLEAR_AREA_X && curr_x <= CLEAR_AREA_X + CLEAR_AREA_W &&
        curr_y >= CLEAR_AREA_Y && curr_y <= CLEAR_AREA_Y + CLEAR_AREA_H) {
      clear_screen();
    } else {
      // 坐标合法性校验
      if (curr_x >= 0 && curr_x < LCD_WIDTH && curr_y >= 0 && curr_y < LCD_HEIGHT) {
        // 打印：上一帧旧值 → 当前新值
        Serial.printf("实时坐标：X=%d, Y=%d | 上一帧：X=%d, Y=%d\n", curr_x, curr_y, prev_x, prev_y);

        // 核心：只要上一帧坐标不是初始值，就强制画线
        if (prev_x != -1 && prev_y != -1) {
          // 画3像素宽的线，确保绝对看得见
          gfx->drawLine(prev_x, prev_y, curr_x, curr_y, PEN_COLOR);
          gfx->drawLine(prev_x + 1, prev_y, curr_x + 1, curr_y, PEN_COLOR);
          gfx->drawLine(prev_x - 1, prev_y, curr_x - 1, curr_y, PEN_COLOR);
        }

        // 强制更新上一帧坐标（永不重置，除非清屏）
        prev_x = curr_x;
        prev_y = curr_y;
        loop_counter=0;
      }
    }
  }
if (loop_counter == 100) {
  prev_x = -1;
  prev_y = -1;
  loop_counter = 0;
}

  delayMicroseconds(200); // 极致高刷新率，保证线条连续
}