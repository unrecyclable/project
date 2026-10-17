/*******************************************************************************
 * UART Serial to RGB LCD Display
 * 功能：串口输入内容，显示屏显示；新输入则清屏显示新内容
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

// ====================== 硬件配置 ======================
#define TFT_BL 2
// RGB LCD分辨率
#define LCD_WIDTH  800
#define LCD_HEIGHT 480

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

// ESP32-S3 RGB LCD引脚定义
#define PIN_DE    40  // DE
#define PIN_VSYNC 41  // VSYNC
#define PIN_HSYNC 39  // HSYNC
#define PIN_PCLK  42  // PCLK
// R通道
#define PIN_R0 45
#define PIN_R1 48
#define PIN_R2 47
#define PIN_R3 21
#define PIN_R4 14
// G通道
#define PIN_G0 5
#define PIN_G1 6
#define PIN_G2 7
#define PIN_G3 15
#define PIN_G4 16
#define PIN_G5 4
// B通道
#define PIN_B0 8
#define PIN_B1 3
#define PIN_B2 46
#define PIN_B3 9
#define PIN_B4 1

// ====================== GFX初始化 ======================
Arduino_ESP32RGBPanel *rgb_panel = new Arduino_ESP32RGBPanel(
    PIN_DE, PIN_VSYNC, PIN_HSYNC, PIN_PCLK,
    PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4,
    PIN_G0, PIN_G1, PIN_G2, PIN_G3, PIN_G4, PIN_G5,
    PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B4,
    HSYNC_POLARITY, HSYNC_FRONT_PORCH, HSYNC_PULSE_WIDTH, HSYNC_BACK_PORCH,
    VSYNC_POLARITY, VSYNC_FRONT_PORCH, VSYNC_PULSE_WIDTH, VSYNC_BACK_PORCH,
    PCLK_ACTIVE_NEG, PCLK_FREQ
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    LCD_WIDTH,        // 宽度
    LCD_HEIGHT,       // 高度
    rgb_panel,        // RGB面板对象
    0,                // 旋转角度（0/90/180/270）
    true              // 自动刷新
);

// ====================== 全局变量 ======================
String inputString = "";  // 存储串口输入的字符串
bool newDataReceived = false; // 标记是否有新的串口数据

// ====================== 函数声明 ======================
void displaySerialContent(const String &content);

// ====================== 初始化函数 ======================
void setup()
{
    // 初始化串口
    Serial.begin(115200);
    Serial.println("UART to LCD Display Example");
    Serial.println("请输入内容，按回车确认显示！");

    // 1. 初始化显示屏
    if (!gfx->begin())
    {
        Serial.println("gfx->begin() failed!");
        while (1) delay(1000); // 初始化失败则死循环
    }
    gfx->fillScreen(RGB565_BLACK); // 初始清屏为黑色

    // 2. 开启显示屏背光
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // 初始显示提示文字
    displaySerialContent("Please enter content in the serial port, press Enter to display!");
}

// ====================== 循环函数 ======================
void loop()
{
    // 读取串口输入的字符
    while (Serial.available() > 0)
    {
        char receivedChar = Serial.read(); // 读取单个字符

        if (receivedChar == '\n' || receivedChar == '\r')
        {
            // 检测到换行/回车，标记有新数据
            newDataReceived = true;
            Serial.printf("收到输入：%s\n", inputString.c_str());
        }
        else
        {
            // 拼接字符到字符串
            inputString += receivedChar;
        }
    }

    // 有新数据时，显示内容并重置
    if (newDataReceived)
    {
        displaySerialContent(inputString); // 显示新内容
        inputString = ""; // 清空缓存
        newDataReceived = false; // 重置标记
    }

    delay(10); // 轻微延时，降低CPU占用
}

// ====================== 显示串口内容到屏幕 ======================
void displaySerialContent(const String &content)
{
    // 1. 清屏（黑色背景）
    gfx->fillScreen(RGB565_BLACK);

    // 2. 设置文本样式
    gfx->setTextSize(3); // 字体大小（可根据需求调整，1=最小，5=最大）
    gfx->setTextColor(RGB565_WHITE); // 文字颜色（白色）
    gfx->setTextWrap(true); // 自动换行
    gfx->setCursor(20, 20); // 文字起始位置（x=20, y=20）

    // 3. 显示内容
    gfx->println(content);
    gfx->setTextSize(2);
    gfx->setTextColor(RGB565_GREEN);
    gfx->printf("\n length:%d", content.length());
}