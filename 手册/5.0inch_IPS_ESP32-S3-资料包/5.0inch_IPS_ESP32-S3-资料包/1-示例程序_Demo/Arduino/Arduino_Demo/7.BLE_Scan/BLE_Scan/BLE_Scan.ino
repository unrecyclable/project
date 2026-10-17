/*******************************************************************************
 * ESP32 BLE Scan + RGB LCD Display
 * 功能：ESP32扫描周边蓝牙设备，显示前17个设备的序号、名称、RSSI
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

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
#define MAX_DISPLAY_BLE 17  // 最多显示前17个蓝牙设备
#define TEXT_ROW_HEIGHT 25  // 每行文字的高度（像素）
#define TEXT_START_X 10     // 文字起始X坐标
#define TEXT_START_Y 10     // 文字起始Y坐标
#define TEXT_SIZE 2         // 字体大小（1=最小，建议2~3）
#define SCAN_TIME 5         // 蓝牙扫描时间（秒）

// ====================== 全局变量 ======================
int scanTime = 5;       //Bluetooth scanning time
BLEScan* pBLEScan;                          // 蓝牙扫描对象
// 存储蓝牙设备信息的结构体
struct BLEDeviceInfo {
    String name;         // 设备名称
    int rssi;            // 信号强度
};
BLEDeviceInfo bleDevices[MAX_DISPLAY_BLE];  // 设备信息数组
int bleDeviceCount = 0;                     // 扫描到的设备总数
bool isScanning = false;                    // 扫描状态标记

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

// ====================== 蓝牙设备回调类 ======================
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // 仅在扫描中、设备有名称且未达上限时存储
        if (isScanning && advertisedDevice.haveName() && bleDeviceCount < MAX_DISPLAY_BLE) {
            bleDevices[bleDeviceCount].name = advertisedDevice.getName().c_str();
            bleDevices[bleDeviceCount].rssi = advertisedDevice.getRSSI();
            bleDeviceCount++;
        }
    }
};

// ====================== 函数声明 ======================
void initBLE(); // 初始化蓝牙
void scanAndDisplayBLE(); // 扫描蓝牙并显示信息
void clearBLEDevices(); // 清空蓝牙设备列表

// ====================== 初始化函数 ======================
void setup()
{
    // 初始化串口
    Serial.begin(115200);
    Serial.println("ESP32 BLE Scan Example");

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

    // 3. 初始化蓝牙
    initBLE();

    // 4. 扫描并显示蓝牙信息
    scanAndDisplayBLE();
}

void loop()
{
    delay(2000);
    scanAndDisplayBLE();
}

// ====================== 初始化蓝牙 ======================
void initBLE() {
    Serial.println("Initializing BLE...");
    BLEDevice::init("ESP32-S3 BLE Scanner"); // 初始化BLE设备
    pBLEScan = BLEDevice::getScan(); // 创建扫描对象
    // 设置回调函数
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); // 主动扫描
    pBLEScan->setInterval(100);    // 扫描间隔
    pBLEScan->setWindow(99);       // 扫描窗口
    Serial.println("BLE initialized.");
}

// ====================== 扫描蓝牙并显示信息======================
void scanAndDisplayBLE()
{
/*
    // 显示扫描提示
    gfx->fillScreen(RGB565_BLACK); // 清屏
    gfx->setTextSize(TEXT_SIZE);
    gfx->setTextColor(RGB565_YELLOW);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y);
    gfx->println("Scanning BLE devices...");
    Serial.println("Scanning BLE devices...");
*/
    // 开始扫描蓝牙
    isScanning = true;
    BLEScanResults *foundDevices = pBLEScan->start(scanTime, false); // 扫描SCAN_TIME秒
    isScanning = false;
    Serial.printf("Scan done! Found %d BLE devices (total), %d with name.\n", foundDevices->getCount(), bleDeviceCount);

    // 清屏并显示表头
    gfx->fillScreen(RGB565_BLACK);
    gfx->setTextSize(TEXT_SIZE);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y);
    gfx->setTextColor(RGB565_CYAN);
    // 表头：序号 | BLE名称 | RSSI
    gfx->printf("%-3s | %-50s | %-4s\n", "NO", "BLE_NAME", "RSSI");
    // 绘制分隔线（对齐WiFi例程）
    gfx->drawLine(TEXT_START_X, TEXT_START_Y + TEXT_ROW_HEIGHT, LCD_WIDTH - 10, TEXT_START_Y + TEXT_ROW_HEIGHT, RGB565_WHITE);

    // 显示前17个蓝牙设备信息
    int displayCount = min(bleDeviceCount, MAX_DISPLAY_BLE); // 实际显示的个数（最多17）
    int currentY = TEXT_START_Y + TEXT_ROW_HEIGHT * 2; // 内容起始Y坐标

    for (int i = 0; i < displayCount; i++)
    {
        // 提取蓝牙设备信息
        String name = bleDevices[i].name;
        int rssi = bleDevices[i].rssi;

        // 设置文字颜色
        gfx->setTextColor(i % 2 == 0 ? RGB565_WHITE : RGB565_GREEN);
        gfx->setCursor(TEXT_START_X, currentY);

        // 格式化输出：序号 | 名称 | RSSI
        // 限制名称长度为30个字符，避免超出列范围
        String shortName = name.substring(0, 30);
        gfx->printf("%-3d | %-50s | %-4d\n", i + 1, shortName.c_str(), rssi);

        // 串口打印（调试）
        Serial.printf("%d: %s, RSSI: %d\n", i + 1, shortName.c_str(), rssi);

        // 下移一行
        currentY += TEXT_ROW_HEIGHT;

    }


    // 停止扫描
    pBLEScan->clearResults();
}

// ====================== 清空蓝牙设备列表 ======================
void clearBLEDevices() {
    bleDeviceCount = 0;
    // 清空数组
    for (int i = 0; i < MAX_DISPLAY_BLE; i++) {
        bleDevices[i].name = "";
        bleDevices[i].rssi = 0;
    }
}