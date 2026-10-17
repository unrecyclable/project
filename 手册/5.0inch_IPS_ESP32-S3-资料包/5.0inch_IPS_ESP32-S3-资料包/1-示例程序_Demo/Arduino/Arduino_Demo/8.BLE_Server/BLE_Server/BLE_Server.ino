/*******************************************************************************
 * ESP32 BLE Serial Server + RGB LCD (800x480)
 * 功能：ESP32创建BLE服务，接收蓝牙数据并显示，显示连接/断开状态
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ====================== BLE配置参数 ======================
#define SERVICE_UUID             "DFCD0001-36E1-4688-B7F5-EA07361B26A8"
#define CHARACTERISTIC1_UUID     "DFCD000A-36E1-4688-B7F5-EA07361B26A8"
bool deviceConnected = false;
BLEServer *pServer = nullptr;
BLEService *pService = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
uint8_t dataLineIndex = 0; // 接收数据的显示行索引
const uint8_t MAX_DATA_LINES = 15; // 最多显示15行数据

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

// ====================== 显示配置参数 ======================
#define TEXT_START_X 20            // 文字起始X坐标
#define STATUS_BAR_Y 30            // 连接状态栏Y坐标
#define DATA_TITLE_Y 70            // 接收数据标题Y坐标
#define DATA_START_Y 100           // 数据显示起始Y坐标
#define TEXT_ROW_HEIGHT 40         // 每行文字高度
#define TEXT_SIZE 2                // 字体大小
#define TITLE_SIZE 2               // 标题字体大小

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

// ====================== BLE服务器回调类======================
class MyServerCallbacks: public BLEServerCallbacks 
{
    void onConnect(BLEServer* pServer) 
    {
        // 清空状态栏并显示连接状态
        gfx->fillRect(TEXT_START_X, STATUS_BAR_Y - 10, LCD_WIDTH - 2 * TEXT_START_X, TEXT_ROW_HEIGHT, RGB565_BLACK);
        gfx->setTextColor(RGB565_CYAN);
        gfx->setTextSize(TEXT_SIZE);
        gfx->setCursor(TEXT_START_X, STATUS_BAR_Y);
        gfx->print("BLE device connected.");
        deviceConnected = true;
        Serial.println("BLE Connected");
    }

    void onDisconnect(BLEServer* pServer) 
    {
        // 清空状态栏并显示断开状态（红色，提示色）
        gfx->fillRect(TEXT_START_X, STATUS_BAR_Y - 10, LCD_WIDTH - 2 * TEXT_START_X, TEXT_ROW_HEIGHT, RGB565_BLACK);
        gfx->setTextColor(RGB565_RED);
        gfx->setTextSize(TEXT_SIZE);
        gfx->setCursor(TEXT_START_X, STATUS_BAR_Y);
        gfx->print("BLE device disconnected.");
        deviceConnected = false;
        Serial.println("BLE Disconnected");
        // 断开后重启，允许重新连接
        pServer->getAdvertising()->start();
    }
};

// ====================== BLE特征值回调类 ======================
class MyCallbacks: public BLECharacteristicCallbacks 
{
    void onWrite(BLECharacteristic *pCharacteristic) 
    {
        String value = pCharacteristic->getValue();
        if (value.length() > 0) 
        {
            // 超出最大行数时，清空数据显示区域并重置索引
            if (dataLineIndex >= MAX_DATA_LINES)
            {
                dataLineIndex = 0;
                gfx->fillRect(TEXT_START_X, DATA_START_Y - 10, LCD_WIDTH - 2 * TEXT_START_X, MAX_DATA_LINES * TEXT_ROW_HEIGHT, RGB565_BLACK);
            }

            // 显示接收的数据
            int currentY = DATA_START_Y + dataLineIndex * TEXT_ROW_HEIGHT;
            gfx->setTextColor((dataLineIndex % 2 == 0) ? RGB565_WHITE : RGB565_GREEN);
            gfx->setTextSize(TEXT_SIZE);
            gfx->setCursor(TEXT_START_X, currentY);
            gfx->print(String(dataLineIndex + 1) + ": " + value); 

            // 串口打印数据
            Serial.printf("Received Data %d: %s\n", dataLineIndex + 1, value.c_str());

            // 发送通知
            pCharacteristic->notify();
            dataLineIndex++;
        }
    }
};

// ====================== BLE初始化函数 ======================
void setupBLE()
{
    // 初始化BLE设备
    BLEDevice::init("ESP32_BT_BLE");
    // 创建BLE服务器
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    // 创建BLE服务
    pService = pServer->createService(SERVICE_UUID);
    // 创建特征值（支持读、通知、写）
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC1_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_WRITE
    );
    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("ESP32 BT BLE");
    // 启动服务和广告
    pService->start();
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
    Serial.println("BLE Server Started, Advertising...");
}

// ====================== 初始化函数 ======================
void setup() 
{
    // 初始化串口
    Serial.begin(115200);

    // 1. 初始化显示屏
    if (!gfx->begin())
    {
        Serial.println("gfx->begin() failed!");
        while (1) delay(1000); // 初始化失败则死循环
    }
    gfx->fillScreen(RGB565_BLACK); // 黑色背景
    gfx->setUTF8Print(true);

    // 2. 开启显示屏背光
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    gfx->setTextSize(TITLE_SIZE);
    gfx->setFont(u8g2_font_unifont_t_gb2312);

    // 4. 初始化BLE
    setupBLE();

    

    // 5. 绘制接收数据标题
    gfx->setTextSize(TEXT_SIZE);
    gfx->setTextColor(RGB565_CYAN);
    gfx->setCursor(TEXT_START_X, DATA_TITLE_Y);
    gfx->print("Received Data:");

}

// ====================== 循环函数 ======================
void loop() 
{
    delay(1000);
}