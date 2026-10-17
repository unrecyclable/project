/*******************************************************************************
 * JPEG Image Viewer
 * This is a simple JPEG image viewer example
 * Image Source: https://github.blog/2014-11-24-from-sticker-to-sculpture-the-making-of-the-octocat-figurine/
 *
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 *
 * Setup steps:
 * Change your LCD parameters in Arduino_GFX setting

 *   SD:
 *     Most Arduino system built-in support SD file system.
 *     Wio Terminal require extra dependant Libraries:
 *     - Seeed_Arduino_FS: https://github.com/Seeed-Studio/Seeed_Arduino_FS.git
 *     - Seeed_Arduino_SFUD: https://github.com/Seeed-Studio/Seeed_Arduino_SFUD.git
 ******************************************************************************/

#include <Arduino_GFX_Library.h>
#include <SPI.h> 
#include <SD.h>
#include "JpegFunc.h"


// 循环显示的4张图片路径
#define IMAGE_COUNT 4 // 图片数量
const char* imageFiles[IMAGE_COUNT] = {
  "/1.jpg",
  "/2.jpg",
  "/3.jpg",
  "/4.jpg"
};
int currentImageIndex = 0; // 当前显示的图片索引（从0开始）
const int IMAGE_DISPLAY_TIME = 3000; // 每张图片显示的时间（毫秒）

// ====================== 硬件配置 ======================
#define TFT_BL 2
// 2. RGB LCD分辨率
#define LCD_WIDTH  800
#define LCD_HEIGHT 480

// 3. 时序参数
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

// 4. ESP32-S3 RGB LCD引脚定义
// 控制引脚
#define PIN_DE    40  // DE
#define PIN_VSYNC 41  // VSYNC
#define PIN_HSYNC 39  // HSYNC
#define PIN_PCLK  42  // PCLK
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

//SD引脚配置
#define SD_CS    10
#define SD_MOSI  11
#define SD_MISO  13
#define SD_SCLK  12

// ====================== GFX初始化 ======================
// 步骤1：初始化ESP32RGBPanel
Arduino_ESP32RGBPanel *rgb_panel = new Arduino_ESP32RGBPanel(
    PIN_DE, PIN_VSYNC, PIN_HSYNC, PIN_PCLK,
    PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4,
    PIN_G0, PIN_G1, PIN_G2, PIN_G3, PIN_G4, PIN_G5,
    PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B4,
    HSYNC_POLARITY, HSYNC_FRONT_PORCH, HSYNC_PULSE_WIDTH, HSYNC_BACK_PORCH,
    VSYNC_POLARITY, VSYNC_FRONT_PORCH, VSYNC_PULSE_WIDTH, VSYNC_BACK_PORCH,
    PCLK_ACTIVE_NEG, PCLK_FREQ
);

// 步骤2：初始化Arduino_RGB_Display
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    LCD_WIDTH,        // 宽度
    LCD_HEIGHT,       // 高度
    rgb_panel,        // RGB面板对象
    0,                // 旋转角度（0/90/180/270）
    true              // 自动刷新
);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/



// pixel drawing callback
static int jpegDrawCallback(JPEGDRAW *pDraw)
{
  // Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

void displayImage(const char* filename)
{
  // 检查文件是否存在
  if (!SD.exists(filename))
  {
    Serial.printf("Error: Image file %s not found!\n", filename);
    gfx->fillScreen(RGB565_BLACK);
    gfx->setCursor(10, 10);
    gfx->setTextColor(RGB565_RED);
    gfx->printf("File %s Not Found!", filename);
    delay(IMAGE_DISPLAY_TIME);
    return;
  }


  // 解码并显示图片
  unsigned long start = millis();
  jpegDraw(filename, jpegDrawCallback, true /* 使用大端格式 */,
           0 /* x坐标：固定在左上角 */, 0 /* y坐标：固定在左上角 */,
           gfx->width() /* 宽度限制：屏幕宽度 */, gfx->height() /* 高度限制：屏幕高度 */);
  unsigned long elapsed = millis() - start;

  // 打印耗时信息
  Serial.printf("Displayed %s, Time used: %lu ms\n", filename, elapsed);

  // 保持显示指定时间
  delay(IMAGE_DISPLAY_TIME);
}

// ====================== 初始化函数 ======================
void setup()
{
  // 初始化串口
  Serial.begin(115200);
  Serial.println("Arduino_GFX JPEG Image Viewer (Cycle 4 Images)");

  // 1. 初始化显示屏
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
    while (1) delay(1000); // 初始化失败则死循环
  }
  gfx->fillScreen(RGB565_BLACK); // 初始清屏

  // 2. 开启显示屏背光
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 3. 初始化SD卡
  Serial.print("Initializing SD card...");
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI))
  {
    Serial.println(F("ERROR: SD Card Mount Failed!"));
    gfx->setCursor(10, 10);
    gfx->setTextColor(RGB565_RED);
    gfx->println(F("SD Card Error!"));
    while (1) delay(1000); // SD卡失败则死循环
  }
  Serial.println(F("SD Card OK!"));
}

// ====================== 循环函数（循环显示4张图片） ======================
void loop()
{
  // 显示当前索引的图片
  displayImage(imageFiles[currentImageIndex]);

  // 索引自增，达到数量后重置为0（循环）
  currentImageIndex++;
  if (currentImageIndex >= IMAGE_COUNT)
  {
    currentImageIndex = 0;
  }
}
