/*******************************************************************************
 * ESP32 WiFi AP Mode + RGB LCD Display
 * 功能：ESP32作为AP热点，屏幕显示AP的SSID、密码、IP、MAC，实时显示已连接终端数
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <WiFi.h> // ESP32 WiFi库

// ====================== WiFi AP配置 ======================
#define AP_SSID "ESP32-S3_AP"    // AP热点名称
#define AP_PASSWORD "12345678"   // AP密码（至少8位，否则无法启动AP）
#define AP_CHANNEL 6             // AP信道（1~13）
#define AP_MAX_CONNECT 4         // 最大连接数（ESP32最多支持4个）

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

// ESP32-S3 RGB LCD引脚定义（保留原有配置）
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

// ====================== 显示配置 ======================
#define TEXT_SIZE_MAIN 3    // 主文字大小（用于标题、关键信息）
#define TEXT_SIZE_SUB 2     // 副文字大小（用于具体内容）
#define TEXT_START_X 20     // 文字起始X坐标
#define TEXT_START_Y 20     // 文字起始Y坐标
#define LINE_SPACING 40     // 行间距（像素）
#define DYNAMIC_AREA_Y 300  // 动态信息区起始Y坐标

// ====================== GFX初始化（保留原有配置） ======================
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
String apSSID = AP_SSID;
String apPassword = AP_PASSWORD;
String apIP;          // AP的IP地址
String apMAC;         // AP的MAC地址
int lastConnectedNum = -1; // 上一次的连接数（用于判断是否需要刷新）

// ====================== 函数声明 ======================
void initWiFiAP(); // 初始化WiFi AP模式
void displayAPInfo(); // 显示AP固定信息
void updateConnectedNum(); // 更新并显示已连接终端数

// ====================== 初始化函数 ======================
void setup()
{
    // 初始化串口（用于调试）
    Serial.begin(115200);
    Serial.println("ESP32 WiFi AP Mode Example");

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

    // 3. 初始化WiFi AP模式
    initWiFiAP();

    // 4. 显示AP固定信息
    displayAPInfo();

    // 5. 初始化连接数显示
    updateConnectedNum();
}

// ====================== 循环函数（实时更新连接数） ======================
void loop()
{
    // 实时检测连接数变化，仅在变化时刷新（减少屏幕闪烁）
    updateConnectedNum();
    delay(500); // 每500ms检测一次
}

// ====================== 初始化WiFi AP模式 ======================
void initWiFiAP()
{
    WiFi.mode(WIFI_AP);
    // 配置AP参数：SSID、密码、信道、是否隐藏、最大连接数
    WiFi.softAP(apSSID.c_str(), apPassword.c_str(), AP_CHANNEL, false, AP_MAX_CONNECT);

    // 获取AP的IP地址和MAC地址
    apIP = WiFi.softAPIP().toString(); // AP默认IP是192.168.4.1
    apMAC = WiFi.softAPmacAddress();  // AP的MAC地址

    // 串口打印AP信息
    Serial.println("\nWiFi AP Mode Initialized:");
    Serial.printf("SSID: %s\n", apSSID.c_str());
    Serial.printf("Password: %s\n", apPassword.c_str());
    Serial.printf("IP Address: %s\n", apIP.c_str());
    Serial.printf("MAC Address: %s\n", apMAC.c_str());
    Serial.printf("Channel: %d\n", AP_CHANNEL);
    Serial.printf("Max Connections: %d\n", AP_MAX_CONNECT);
}

// ====================== 显示AP固定信息（SSID、密码、IP、MAC） ======================
void displayAPInfo()
{
    // 标题
    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(RGB565_CYAN);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y);
    gfx->println("ESP32 WiFi AP Information");
    gfx->drawLine(TEXT_START_X, TEXT_START_Y + 30, LCD_WIDTH - 20, TEXT_START_Y + 30, RGB565_WHITE); // 分隔线

    // SSID
    gfx->setTextSize(TEXT_SIZE_SUB);
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y + LINE_SPACING);
    gfx->print("AP SSID: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(apSSID);

    // Password
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y + LINE_SPACING * 2);
    gfx->print("AP Password: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(apPassword);

    // IP Address
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y + LINE_SPACING * 3);
    gfx->print("AP IP Address: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(apIP);

    // MAC Address
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y + LINE_SPACING * 4);
    gfx->print("AP MAC Address: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(apMAC);

    // 动态信息区标题
    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(RGB565_YELLOW);
    gfx->setCursor(TEXT_START_X, DYNAMIC_AREA_Y);
    gfx->println("Connected Terminals:");
    gfx->drawLine(TEXT_START_X, DYNAMIC_AREA_Y + 30, LCD_WIDTH - 20, DYNAMIC_AREA_Y + 30, RGB565_WHITE);
}

// ====================== 更新并显示已连接终端数 ======================
void updateConnectedNum()
{
    int currentConnectedNum = WiFi.softAPgetStationNum(); // 获取当前连接数

    // 仅在连接数变化时刷新，避免屏幕闪烁
    if (currentConnectedNum != lastConnectedNum)
    {
        // 清空动态信息区的旧内容
        gfx->fillRect(TEXT_START_X, DYNAMIC_AREA_Y + 40, LCD_WIDTH - 40, 100, RGB565_BLACK);

        // 显示当前连接数
        gfx->setTextSize(TEXT_SIZE_MAIN);
        gfx->setTextColor(RGB565_RED);
        gfx->setCursor(TEXT_START_X, DYNAMIC_AREA_Y + 50);
        gfx->printf("Current Connections: %d / %d\n", currentConnectedNum, AP_MAX_CONNECT);

        // 串口打印
        Serial.printf("Connected terminals: %d (max: %d)\n", currentConnectedNum, AP_MAX_CONNECT);

        // 更新上一次的连接数
        lastConnectedNum = currentConnectedNum;
    }
}