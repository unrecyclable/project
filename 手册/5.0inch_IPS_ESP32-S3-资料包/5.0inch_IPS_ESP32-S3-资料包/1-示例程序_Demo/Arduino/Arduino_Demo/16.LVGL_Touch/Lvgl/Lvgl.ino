
/*******************************************************************************
 * LVGL Widgets
 * This is a widgets demo for LVGL - Light and Versatile Graphics Library
 * import from: https://github.com/lvgl/lv_demos.git
 *
 * Dependent libraries:
 * LVGL: https://github.com/lvgl/lvgl.git

 * Touch libraries:
 * FT6X36: https://github.com/strange-v/FT6X36.git
 * GT911: https://github.com/TAMCTec/gt911-arduino.git
 * XPT2046: https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
 *
 * LVGL Configuration file:
 * Copy your_arduino_path/libraries/lvgl/lv_conf_template.h
 * to your_arduino_path/libraries/lv_conf.h
 * Then find and set:
 * #define LV_COLOR_DEPTH     16
 * #define LV_TICK_CUSTOM     1
 *
 * For SPI display set color swap can be faster, parallel screen don't set!
 * #define LV_COLOR_16_SWAP   1
 *
 * Optional: Show CPU usage and FPS count
 * #define LV_USE_PERF_MONITOR 1
 ******************************************************************************/
//#include "lv_demo_widgets.h"
#include <lvgl.h>
#include <demos/lv_demos.h>
/*******************************************************************************
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
// ====================== 硬件配置 ======================
// 1. 背光引脚
#define TFT_BL 2
// 2. RGB LCD分辨率
#define LCD_WIDTH 800
#define LCD_HEIGHT 480

// 3. IPS时序参数
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


// 4. ESP32-S3 RGB LCD引脚定义（根据硬件接线修改）
// 控制引脚
#define PIN_DE 40     // DE
#define PIN_VSYNC 41  // VSYNC
#define PIN_HSYNC 39  // HSYNC
#define PIN_PCLK 42   // PCLK
// R通道（R0~R4，5位）
#define PIN_R0 45
#define PIN_R1 48
#define PIN_R2 47
#define PIN_R3 21
#define PIN_R4 14
// G通道（G0~G5，6位）
#define PIN_G0 5
#define PIN_G1 6
#define PIN_G2 7
#define PIN_G3 15
#define PIN_G4 16
#define PIN_G5 4
// B通道（B0~B4，5位）
#define PIN_B0 8
#define PIN_B1 3
#define PIN_B2 46
#define PIN_B3 9
#define PIN_B4 1

// ====================== GFX初始化 ======================
// 步骤1：初始化ESP32RGBPanel
Arduino_ESP32RGBPanel *rgb_panel = new Arduino_ESP32RGBPanel(
  PIN_DE, PIN_VSYNC, PIN_HSYNC, PIN_PCLK,
  PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4,
  PIN_G0, PIN_G1, PIN_G2, PIN_G3, PIN_G4, PIN_G5,
  PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B4,
  HSYNC_POLARITY, HSYNC_FRONT_PORCH, HSYNC_PULSE_WIDTH, HSYNC_BACK_PORCH,
  VSYNC_POLARITY, VSYNC_FRONT_PORCH, VSYNC_PULSE_WIDTH, VSYNC_BACK_PORCH,
  PCLK_ACTIVE_NEG, PCLK_FREQ);

// 步骤2：初始化Arduino_RGB_Display（匹配构造函数参数顺序）
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  LCD_WIDTH,   // 宽度
  LCD_HEIGHT,  // 高度
  rgb_panel,   // RGB面板对象
  0,           // 旋转角度（0/90/180/270）
  true         // 自动刷新
);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

/*******************************************************************************
 * Please config the touch panel in touch.h
 ******************************************************************************/
#include "touch.h"

/* Change to your screen resolution */
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  if (touch_has_signal()) {
    if (touch_touched()) {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
    } else if (touch_released()) {
      data->state = LV_INDEV_STATE_REL;
    }
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void setup() {
  Serial.begin(115200);
  // while (!Serial);
  Serial.println("LVGL Widgets Demo");

  // Init touch device


  // Init Display
  gfx->begin();
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif
  lv_init();
  delay(10);
  touch_init();
  screenWidth = gfx->width();
  screenHeight = gfx->height();
#ifdef ESP32
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
  disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4);
#endif
  if (!disp_draw_buf) {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  } else {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 4);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Initialize the (dummy) input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    lv_demo_widgets();
    //lv_demo_benchmark();
    //lv_demo_music();
    //lv_demo_stress();

    Serial.println("Setup done");
  }
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}
