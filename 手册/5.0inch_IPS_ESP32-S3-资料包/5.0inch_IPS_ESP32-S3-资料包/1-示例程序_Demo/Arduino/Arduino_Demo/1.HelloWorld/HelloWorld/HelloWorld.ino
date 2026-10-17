#include <Arduino_GFX_Library.h>

// ====================== 硬件配置======================
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
/*******************************************************************************
 * Setup函数
 ******************************************************************************/
void setup(void)
{
    // 初始化串口
    Serial.begin(115200);
    delay(100);

    // 初始化显示屏
    if (!gfx->begin())
    {
        Serial.println("gfx->begin() failed!");
        while (1) delay(1000); // 初始化失败则死循环
    }

    // 清屏（黑色）
    gfx->fillScreen(BLACK);

    // 开启背光
#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif

    // 显示Hello World
    gfx->setCursor(10, 10);
    gfx->setTextColor(RED);
    gfx->setTextSize(2);
    gfx->println("Hello World!");

    delay(5000);
}

/*******************************************************************************
 * Loop函数
 ******************************************************************************/
void loop()
{
    // 随机位置显示文字
    gfx->setCursor(random(gfx->width() - 100), random(gfx->height() - 20));
    // 随机颜色
    gfx->setTextColor(random(0xFFFF), random(0xFFFF));
    // 随机文字大小
    gfx->setTextSize(random(1, 6));
    gfx->println("Hello World!");

    delay(1000);
}