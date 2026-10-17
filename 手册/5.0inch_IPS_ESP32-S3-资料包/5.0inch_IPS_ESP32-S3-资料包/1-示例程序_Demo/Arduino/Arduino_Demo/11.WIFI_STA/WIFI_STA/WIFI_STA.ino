/*******************************************************************************
 * ESP32 WiFi STA Mode + RGB LCD Display
 * 功能：ESP32 STA模式连接指定WiFi，屏幕显示连接状态和网络信息（IP、MAC、网关等）
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <WiFi.h> // ESP32 WiFi库

// ====================== WiFi STA配置（修改为你的WiFi信息） ======================
#define WIFI_SSID "ESP"    // 要连接的WiFi名称
#define WIFI_PASSWORD "12345678" // 要连接的WiFi密码
#define WIFI_CONNECT_RETRY 5          // 连接失败重试次数
#define WIFI_CONNECT_DELAY 3000       // 重试间隔（毫秒）

// ====================== 硬件配置（保留原有RGB LCD配置） ======================
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

// ====================== 显示配置 ======================
#define TEXT_SIZE_MAIN 3    // 主文字大小（状态、标题）
#define TEXT_SIZE_SUB 2     // 副文字大小（具体信息）
#define TEXT_START_X 20     // 文字起始X坐标
#define TEXT_START_Y 20     // 文字起始Y坐标
#define LINE_SPACING 40     // 行间距（像素）
#define INFO_AREA_Y 150     // 网络信息区起始Y坐标

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
bool isWiFiConnected = false; // WiFi是否连接成功
String wifiSSID = WIFI_SSID;
String localIP;               // 本地IP地址
String localMAC;              // 本地MAC地址
String gatewayIP;             // 网关IP
String subnetMask;            // 子网掩码
String dnsIP;                 // DNS服务器IP

// ====================== 函数声明 ======================
void initWiFiSTA(); // 初始化WiFi STA模式并连接
void displayConnectStatus(const String &status, uint16_t color); // 显示连接状态
void displayNetworkInfo(); // 显示网络信息
void checkWiFiStatus(); // 实时检查WiFi连接状态

// ====================== 初始化函数 ======================
void setup()
{
    // 初始化串口（用于调试）
    Serial.begin(115200);
    Serial.println("ESP32 WiFi STA Mode Example");

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

    // 3. 初始化WiFi STA模式并连接
    initWiFiSTA();
}

// ====================== 循环函数（实时检查WiFi状态） ======================
void loop()
{
    checkWiFiStatus(); // 实时监测WiFi连接状态
    delay(1000); // 每秒检查一次
}

// ====================== 初始化WiFi STA模式并连接 ======================
void initWiFiSTA()
{
    // 设置为STA模式
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // 断开原有连接
    delay(100);

    // 显示连接提示
    displayConnectStatus("Connecting to WiFi: " + wifiSSID, RGB565_YELLOW);
    Serial.printf("Connecting to %s...\n", WIFI_SSID);

    // 连接WiFi，带重试机制
    int retryCount = 0;
    while (retryCount < WIFI_CONNECT_RETRY)
    {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // 等待连接（最多5秒）
        unsigned long start = millis();
        while (millis() - start < 5000)
        {
            wl_status_t status = WiFi.status();
            if (status == WL_CONNECTED)
            {
                // 连接成功
                isWiFiConnected = true;
                // 获取网络信息
                localIP = WiFi.localIP().toString();
                localMAC = WiFi.macAddress();
                gatewayIP = WiFi.gatewayIP().toString();
                subnetMask = WiFi.subnetMask().toString();
                dnsIP = WiFi.dnsIP().toString();
                // 显示成功状态
                displayConnectStatus("WiFi Connected Successfully!", RGB565_GREEN);
                // 显示网络信息
                displayNetworkInfo();
                Serial.println("WiFi connected successfully!");
                return;
            }
            else if (status == WL_CONNECT_FAILED)
            {
                break; // 连接失败，退出等待
            }
            delay(100);
        }

        // 连接失败，重试
        retryCount++;
        String statusMsg = "Connect failed! Retry: " + String(retryCount) + "/" + String(WIFI_CONNECT_RETRY);
        displayConnectStatus(statusMsg, RGB565_ORANGE);
        Serial.println(statusMsg);
        delay(WIFI_CONNECT_DELAY);
    }

    // 所有重试都失败
    displayConnectStatus("WiFi Connect Failed! All retries used.", RGB565_RED);
    Serial.println("WiFi connect failed! All retries used.");
}

// ====================== 显示连接状态 ======================
void displayConnectStatus(const String &status, uint16_t color)
{
    // 清空状态区
    gfx->fillRect(TEXT_START_X, TEXT_START_Y, LCD_WIDTH - 40, 100, RGB565_BLACK);
    // 显示状态
    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(color);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y);
    gfx->println(status);
    // 分隔线
    gfx->drawLine(TEXT_START_X, TEXT_START_Y + 50, LCD_WIDTH - 20, TEXT_START_Y + 50, RGB565_WHITE);
}

// ====================== 显示网络信息 ======================
void displayNetworkInfo()
{
    // 清空信息区
    gfx->fillRect(TEXT_START_X, INFO_AREA_Y, LCD_WIDTH - 40, LCD_HEIGHT - INFO_AREA_Y - 20, RGB565_BLACK);

    // 标题
    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(RGB565_CYAN);
    gfx->setCursor(TEXT_START_X, INFO_AREA_Y);
    gfx->println("Network Information");
    gfx->drawLine(TEXT_START_X, INFO_AREA_Y + 30, LCD_WIDTH - 20, INFO_AREA_Y + 30, RGB565_WHITE);

    // SSID
    gfx->setTextSize(TEXT_SIZE_SUB);
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, INFO_AREA_Y + LINE_SPACING);
    gfx->print("Connected SSID: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(wifiSSID);

    // Local IP
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, INFO_AREA_Y + LINE_SPACING * 2);
    gfx->print("Local IP Address: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(localIP);

    // MAC Address
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, INFO_AREA_Y + LINE_SPACING * 3);
    gfx->print("MAC Address: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(localMAC);

    // Gateway IP
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, INFO_AREA_Y + LINE_SPACING * 4);
    gfx->print("Gateway IP: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(gatewayIP);

    // Subnet Mask
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, INFO_AREA_Y + LINE_SPACING * 5);
    gfx->print("Subnet Mask: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(subnetMask);

    // DNS IP
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(TEXT_START_X, INFO_AREA_Y + LINE_SPACING * 6);
    gfx->print("DNS Server IP: ");
    gfx->setTextColor(RGB565_GREEN);
    gfx->println(dnsIP);

    // 串口打印所有信息
    Serial.println("\nNetwork Information:");
    Serial.printf("Connected SSID: %s\n", wifiSSID.c_str());
    Serial.printf("Local IP: %s\n", localIP.c_str());
    Serial.printf("MAC Address: %s\n", localMAC.c_str());
    Serial.printf("Gateway IP: %s\n", gatewayIP.c_str());
    Serial.printf("Subnet Mask: %s\n", subnetMask.c_str());
    Serial.printf("DNS IP: %s\n", dnsIP.c_str());
}

// ====================== 实时检查WiFi连接状态 ======================
void checkWiFiStatus()
{
    if (isWiFiConnected && WiFi.status() != WL_CONNECTED)
    {
        // 连接丢失，更新状态
        isWiFiConnected = false;
        displayConnectStatus("WiFi Connection Lost! Reconnecting...", RGB565_RED);
        Serial.println("WiFi connection lost! Reconnecting...");
        // 重新连接
        initWiFiSTA();
    }
}