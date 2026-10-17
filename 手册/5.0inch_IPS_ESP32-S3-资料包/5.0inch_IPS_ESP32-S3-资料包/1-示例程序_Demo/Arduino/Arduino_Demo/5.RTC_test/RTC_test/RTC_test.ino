/*******************************************************************************
 * ESP32 RTC Time Display + RGB LCD (800x480)
 * 功能：ESP32使用内置RTC显示时间（时分秒）、日期（年月日）、星期
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <ESP32Time.h>  // ESP32Time库

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

// ====================== 配置参数 ======================
#define BASE_TEXT_SIZE 3        // 基础字体大小
#define TITLE_TEXT_SIZE 4       // 标题字体大小
#define TIME_TEXT_SIZE 5        // 时间核心信息字体大小
#define TEXT_ROW_HEIGHT 50      // 每行文字的高度
#define BIG_ROW_HEIGHT 70       // 大字体行高
#define MARGIN_X 50             // 左右边距
#define TITLE_Y 30              // 标题Y坐标（顶部）
#define TIME_Y 100              // 时间Y坐标（中间上半部分）
#define DATE_Y (TIME_Y + BIG_ROW_HEIGHT) // 日期Y坐标（中间下半部分）
#define WEEK_Y (DATE_Y + TEXT_ROW_HEIGHT + 20) // 星期Y坐标（底部）

// ====================== 全局变量 ======================
ESP32Time esp32_rtc;       // RTC时间对象
char t_buf[100];           // 存储时间信息的缓冲区

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

// ====================== 函数声明 ======================
void drawTimeInfo(); // 绘制时间、日期、星期信息
String getWeekString(int weekNum); // 将数字星期转换为字符串
int getTextCenterX(String text, int size); // 计算文字居中的X坐标（新增）

// ====================== 初始化函数 ======================
void setup()
{
    // 初始化串口（用于调试）
    Serial.begin(115200);
    Serial.println("ESP32 RTC Time Display Example (Big Font + New Layout)");

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

    // 3. 设置初始时间：秒, 分, 时, 日, 月, 年 (2025-12-18 15:48:28)
    esp32_rtc.setTime(28, 48, 15, 18, 12, 2025);

    // 4. 绘制标题（居中显示，大字体红色）
    gfx->setTextSize(TITLE_TEXT_SIZE);
    gfx->setTextColor(RGB565_RED); // 标题用红色
    String title = "ESP32 RTC TEST";
    int titleX = getTextCenterX(title, TITLE_TEXT_SIZE);
    gfx->setCursor(titleX, TITLE_Y);
    gfx->println(title);
}

// ====================== 循环函数 ======================
void loop()
{
    // 绘制时间、日期、星期信息
    drawTimeInfo();
    // 1秒刷新一次
    delay(1000);
}

// ====================== 绘制时间信息（新布局+大字体） ======================
void drawTimeInfo()
{
    // 1. 显示时间：时:分:秒
    gfx->setTextSize(TIME_TEXT_SIZE);
    gfx->setTextColor(RGB565_WHITE);
    // 清空上一次的时间显示区域（避免重叠）
    gfx->fillRect(MARGIN_X, TIME_Y, LCD_WIDTH - 2 * MARGIN_X, BIG_ROW_HEIGHT, RGB565_BLACK);
    sprintf(t_buf, "%02d:%02d:%02d", esp32_rtc.getHour(true), esp32_rtc.getMinute(), esp32_rtc.getSecond());
    String timeStr = String(t_buf);
    int timeX = getTextCenterX(timeStr, TIME_TEXT_SIZE);
    gfx->setCursor(timeX, TIME_Y + BIG_ROW_HEIGHT / 2 - 10); // 垂直居中微调
    gfx->println(timeStr);
    Serial.println(timeStr); // 串口调试

    // 2. 显示日期：年-月-日（大字体绿色，居中显示，交替颜色）
    gfx->setTextSize(BASE_TEXT_SIZE);
    gfx->setTextColor(RGB565_GREEN); // 绿色
    // 清空上一次的日期显示区域
    gfx->fillRect(MARGIN_X, DATE_Y, LCD_WIDTH - 2 * MARGIN_X, TEXT_ROW_HEIGHT, RGB565_BLACK);
    sprintf(t_buf, "%04d-%02d-%02d", esp32_rtc.getYear(), esp32_rtc.getMonth() + 1, esp32_rtc.getDay());
    String dateStr = String(t_buf);
    int dateX = getTextCenterX(dateStr, BASE_TEXT_SIZE);
    gfx->setCursor(dateX, DATE_Y + TEXT_ROW_HEIGHT / 2 - 5); // 垂直居中微调
    gfx->println(dateStr);
    Serial.println(dateStr);

    // 3. 显示星期：数字转字符串
    gfx->setTextSize(BASE_TEXT_SIZE);
    gfx->setTextColor(RGB565_WHITE);
    // 清空上一次的星期显示区域
    gfx->fillRect(MARGIN_X, WEEK_Y, LCD_WIDTH - 2 * MARGIN_X, TEXT_ROW_HEIGHT, RGB565_BLACK);
    int weekNum = esp32_rtc.getDayofWeek();
    sprintf(t_buf, "Week: %d (%s)", weekNum, getWeekString(weekNum).c_str());
    String weekStr = String(t_buf);
    int weekX = getTextCenterX(weekStr, BASE_TEXT_SIZE);
    gfx->setCursor(weekX, WEEK_Y + TEXT_ROW_HEIGHT / 2 - 5);
    gfx->println(weekStr);
    Serial.println(weekStr);
}

// ====================== 辅助函数：计算文字居中的X坐标 ======================
int getTextCenterX(String text, int size)
{
    // 计算文字的像素宽度：每个字符的宽度约为 6*size
    int textWidth = text.length() * 6 * size;
    return (LCD_WIDTH - textWidth) / 2;
}

// ====================== 将数字星期转换为字符串 ======================
String getWeekString(int weekNum)
{
    switch (weekNum)
    {
        case 1: return "Monday";
        case 2: return "Tuesday";
        case 3: return "Wednesday";
        case 4: return "Thursday";
        case 5: return "Friday";
        case 6: return "Saturday";
        case 7: return "Sunday";
        default: return "Unknown";
    }
}