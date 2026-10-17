/*******************************************************************************
 * ESP32 WiFi Scan + RGB LCD Display
 * 功能：ESP32 STA模式扫描周边WiFi，显示前17个WiFi的SSID、RSSI、CHANNEL、ENC_TYPE，最后显示总数
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <WiFi.h> // ESP32 WiFi库

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

// ====================== 配置参数 ======================
#define MAX_DISPLAY_WIFI 17 // 最多显示前17个WiFi
#define TEXT_ROW_HEIGHT 25  // 每行文字的高度（像素）
#define TEXT_START_X 10     // 文字起始X坐标
#define TEXT_START_Y 10     // 文字起始Y坐标
#define TEXT_SIZE 2         // 字体大小（1=最小，建议2~3）

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
String getEncryptionTypeString(wifi_auth_mode_t encType); // 转换加密类型为字符串
void scanAndDisplayWiFi(); // 扫描WiFi并显示信息

// ====================== 初始化函数 ======================
void setup()
{
    // 初始化串口（用于调试）
    Serial.begin(115200);
    Serial.println("ESP32 WiFi Scan Example");

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

    // 3. 初始化WiFi（STA模式，关闭AP模式）
    WiFi.mode(WIFI_STA); // 设置为STA模式（默认是STA，此处显式设置更清晰）
    WiFi.disconnect();   // 断开原有连接（如果有）
    delay(100);

    // 4. 扫描并显示WiFi信息
    scanAndDisplayWiFi();
}

// ====================== 循环函数（空循环，扫描只执行一次） ======================
void loop()
{
    // 如需定时扫描，可在此添加延时后调用scanAndDisplayWiFi()
    // 例如：delay(5000); scanAndDisplayWiFi();
    delay(1000);
}

// ====================== 扫描WiFi并显示信息 ======================
void scanAndDisplayWiFi()
{
    // 1. 清屏
    gfx->fillScreen(RGB565_BLACK);

    // 2. 显示扫描提示
    gfx->setTextSize(TEXT_SIZE);
    gfx->setTextColor(RGB565_YELLOW);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y);
    gfx->println("Scanning WiFi networks...");
    Serial.println("Scanning WiFi networks...");

    // 3. 开始扫描WiFi（参数true表示隐藏网络也扫描）
    int n = WiFi.scanNetworks(false, true);
    Serial.printf("Scan done! Found %d WiFi networks.\n", n);

    // 4. 清屏并显示表头
    gfx->fillScreen(RGB565_BLACK);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y);
    gfx->setTextColor(RGB565_CYAN);
    // 表头：序号 | SSID | RSSI | CH | ENC
    gfx->printf("%-3s | %-30s | %-4s | %-2s | %-10s\n", "NO", "SSID", "RSSI", "CH", "ENC");
    gfx->drawLine(TEXT_START_X, TEXT_START_Y + TEXT_ROW_HEIGHT, LCD_WIDTH - 10, TEXT_START_Y + TEXT_ROW_HEIGHT, RGB565_WHITE); // 分隔线

    // 5. 显示前17个WiFi信息
    int displayCount = min(n, MAX_DISPLAY_WIFI); // 实际显示的个数（最多17）
    int currentY = TEXT_START_Y + TEXT_ROW_HEIGHT * 2; // 内容起始Y坐标

    for (int i = 0; i < displayCount; i++)
    {
        // 提取WiFi信息
        String ssid = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        int channel = WiFi.channel(i);
        wifi_auth_mode_t encType = WiFi.encryptionType(i);
        String encStr = getEncryptionTypeString(encType);

        // 设置文字颜色（交替颜色，便于阅读）
        gfx->setTextColor(i % 2 == 0 ? RGB565_WHITE : RGB565_GREEN);
        gfx->setCursor(TEXT_START_X, currentY);

        // 显示：序号 | SSID | RSSI | 信道 | 加密类型
        // 格式化输出，对齐显示
        gfx->printf("%-3d | %-30s | %-4d | %-2d | %-10s\n", i + 1, ssid.c_str(), rssi, channel, encStr.c_str());

        // 串口打印（调试）
        Serial.printf("%d: %s, RSSI: %d, CH: %d, ENC: %s\n", i + 1, ssid.c_str(), rssi, channel, encStr.c_str());

        // 下移一行
        currentY += TEXT_ROW_HEIGHT;

        // 防止超出屏幕
        if (currentY > LCD_HEIGHT - TEXT_ROW_HEIGHT * 2)
        {
            break;
        }
    }

    // 6. 显示扫描总数
    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(TEXT_START_X, LCD_HEIGHT - TEXT_ROW_HEIGHT);
    gfx->printf("Total WiFi found: %d (Displayed: %d / Max: %d)\n", n, displayCount, MAX_DISPLAY_WIFI);

    // 7. 停止扫描（释放资源）
    WiFi.scanDelete();
}

// ====================== 转换加密类型为字符串 ======================
String getEncryptionTypeString(wifi_auth_mode_t encType)
{
    switch (encType)
    {
        case WIFI_AUTH_OPEN:
            return "OPEN"; // 开放网络（无加密）
        case WIFI_AUTH_WEP:
            return "WEP";  // WEP加密
        case WIFI_AUTH_WPA_PSK:
            return "WPA";  // WPA加密
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2"; // WPA2加密
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2"; // WPA+WPA2加密
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3"; // WPA3加密
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3"; // WPA2+WPA3加密
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI"; // WAPI加密
        default:
            return "UNKNOWN"; // 未知加密类型
    }
}