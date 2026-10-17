/*******************************************************************************
 * ESP32 WiFi STA + TCP Client + RGB LCD Display
 * 功能：ESP32 STA模式连接WiFi，作为TCP客户端连接指定服务器，
 *       屏幕显示WiFi/TCP连接状态、网络信息及收发的数据
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WiFiClient.h> // TCP客户端库

// ====================== WiFi STA配置 ======================
#define WIFI_SSID         "ESP"        // 参考代码的WiFi名称
#define WIFI_PASSWORD     "12345678"   // 参考代码的WiFi密码
#define WIFI_CONNECT_RETRY 5           // WiFi连接失败重试次数
#define WIFI_CONNECT_DELAY 3000        // WiFi重试间隔（毫秒）

// ====================== TCP Client配置 ======================
#define TCP_SERVER_IP     "192.168.1.126"// 参考代码的TCP服务器IP
#define TCP_SERVER_PORT   8080         // 参考代码的TCP服务器端口
#define TCP_CONNECT_RETRY 3            // TCP连接失败重试次数
#define TCP_CONNECT_DELAY 2000         // TCP重试间隔（毫秒）
#define TCP_RECV_BUFFER_SIZE 1024      // 接收缓冲区大小

// ====================== 硬件配置 ======================
#define TFT_BL 2
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
#define PIN_DE    40
#define PIN_VSYNC 41
#define PIN_HSYNC 39
#define PIN_PCLK  42
#define PIN_R0 45
#define PIN_R1 48
#define PIN_R2 47
#define PIN_R3 21
#define PIN_R4 14
#define PIN_G0 5
#define PIN_G1 6
#define PIN_G2 7
#define PIN_G3 15
#define PIN_G4 16
#define PIN_G5 4
#define PIN_B0 8
#define PIN_B1 3
#define PIN_B2 46
#define PIN_B3 9
#define PIN_B4 1

// ====================== 显示配置 ======================
#define TEXT_SIZE_MAIN 3    
#define TEXT_SIZE_SUB 2     
#define TEXT_START_X 20     
#define TEXT_START_Y 20     
#define LINE_SPACING 40     
#define WIFI_INFO_Y 120     // WiFi信息区起始Y
#define TCP_INFO_Y 280      // TCP信息区起始Y
#define DATA_INFO_Y 380     // 数据收发区起始Y

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
    LCD_WIDTH, LCD_HEIGHT, rgb_panel, 0, true
);

// ====================== 全局变量 ======================
bool isWiFiConnected = false;    // WiFi连接状态
WiFiClient client;               // TCP客户端对象
char t_buf[100] = {0};           // 缓存数组

// 网络信息
String wifiSSID = WIFI_SSID;
String localIP;               
String localMAC;              

// TCP相关
IPAddress serverIP(TCP_SERVER_IP); // 参考代码的服务器IP对象
uint16_t serverPort = TCP_SERVER_PORT;
String recvData = "";           // 接收的数据
String sendData = "Hello, this is client!"; // 参考代码的初始发送数据

// ====================== 函数声明 ======================
void initWiFiSTA();                // 初始化WiFi STA
void displayWiFiStatus(const String &status, uint16_t color); // 显示WiFi状态
void displayWiFiInfo();            // 显示WiFi信息
void displayTCPStatus(const String &status, uint16_t color);  // 显示TCP状态
void displayDataInfo();            // 显示收发数据

// ====================== 初始化函数 ======================
void setup()
{
    Serial.begin(115200);
    Serial.println("ESP32 WiFi STA + TCP Client Example (Adapted)");

    // 初始化显示屏
    if (!gfx->begin())
    {
        Serial.println("gfx->begin() failed!");
        while (1) delay(1000);
    }
    gfx->fillScreen(RGB565_WHITE); // 背景为白色
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // 关闭WiFi休眠
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    // 1. 显示WiFi连接中状态
    displayWiFiStatus("Start connecting to WiFi ...", RGB565_RED);
    Serial.printf("Connecting to WiFi: %s...\n", WIFI_SSID);

    // 2. 连接WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // WiFi连接成功
    isWiFiConnected = true;
    localIP = WiFi.localIP().toString();
    localMAC = WiFi.macAddress();
    
    // 更新WiFi连接成功状态
    displayWiFiStatus("Connection WIFI successful!!!", RGB565_GREEN);
    displayWiFiInfo(); // 显示WiFi详细信息

    // 3. 显示TCP连接尝试状态
    displayTCPStatus("Attempting to access server ...", RGB565_RED);
}

// ====================== 循环函数 ======================
void loop()
{
    if (isWiFiConnected)
    {
        // 尝试连接TCP服务器
        if (client.connect(serverIP, serverPort)) 
        {
            // TCP连接成功
            displayTCPStatus("Access successful !", RGB565_GREEN);
            Serial.println("TCP Server Access successful !");

            // 发送初始数据
            client.print(sendData);
            Serial.printf("TCP Send: %s\n", sendData.c_str());
            sendData = "Hello, this is client!"; // 重置发送数据
            displayDataInfo(); // 更新发送数据显示

            // 保持连接并处理数据
            while (client.connected() || client.available()) 
            {
                if (client.available() > 0)  // 有可读取数据
                {
                    // 读取所有可用数据
                    String line = "";
                    while (client.available() > 0) {
                        line += (char)client.read();
                    }
                    recvData = line;
                    Serial.printf("TCP Recv: %s\n", recvData.c_str());

                    // 更新接收数据显示
                    displayDataInfo();

                    // 回显数据给服务器
                    client.write(recvData.c_str());
                    Serial.printf("TCP Echo: %s\n", recvData.c_str());
                }
                delay(10); // 短延时避免占用过多资源
            }

            // 连接断开
            displayTCPStatus("Close current connection.", RGB565_RED);
            Serial.println("TCP Connection closed.");
            client.stop(); // 关闭客户端
        }
        else
        {
            // TCP连接失败
            displayTCPStatus("Access failed, Close client !", RGB565_RED);
            Serial.println("TCP Server Access failed !");
            client.stop(); // 关闭客户端
        }
    }

    // 5秒后重试连接
    delay(5000);
}

// ====================== 初始化WiFi STA======================
void initWiFiSTA()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    displayWiFiStatus("Connecting to WiFi: " + wifiSSID, RGB565_RED);
    Serial.printf("Connecting to WiFi: %s...\n", WIFI_SSID);

    int retryCount = 0;
    while (retryCount < WIFI_CONNECT_RETRY)
    {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        unsigned long start = millis();
        while (millis() - start < 5000)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                isWiFiConnected = true;
                localIP = WiFi.localIP().toString();
                localMAC = WiFi.macAddress();
                
                displayWiFiStatus("WiFi Connected!", RGB565_GREEN);
                displayWiFiInfo();
                Serial.println("WiFi connected successfully!");
                return;
            }
            delay(100);
        }

        retryCount++;
        String statusMsg = "WiFi Retry: " + String(retryCount) + "/" + String(WIFI_CONNECT_RETRY);
        displayWiFiStatus(statusMsg, RGB565_ORANGE);
        Serial.println(statusMsg);
        delay(WIFI_CONNECT_DELAY);
    }

    displayWiFiStatus("WiFi Connect Failed!", RGB565_RED);
    Serial.println("WiFi connect failed!");
}

// ====================== 显示WiFi状态======================
void displayWiFiStatus(const String &status, uint16_t color)
{
    // 清空WiFi状态区
    gfx->fillRect(TEXT_START_X, TEXT_START_Y, LCD_WIDTH - 40, 80, RGB565_WHITE);
    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(color);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y + 35); 
    gfx->println(status);
    gfx->drawLine(TEXT_START_X, TEXT_START_Y + 60, LCD_WIDTH - 20, TEXT_START_Y + 60, RGB565_BLACK);
}

// ====================== 显示TCP状态======================
void displayTCPStatus(const String &status, uint16_t color)
{
    // 清空TCP状态区
    gfx->fillRect(TEXT_START_X, TCP_INFO_Y - 60, LCD_WIDTH - 40, 50, RGB565_WHITE);
    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(color);
    gfx->setCursor(TEXT_START_X, TCP_INFO_Y - 60 + 20);
    gfx->println(status);
    gfx->drawLine(TEXT_START_X, TCP_INFO_Y - 50, LCD_WIDTH - 20, TCP_INFO_Y - 50, RGB565_BLACK);
}

// ====================== 显示WiFi信息======================
void displayWiFiInfo()
{
    // 清空WiFi信息区
    gfx->fillRect(TEXT_START_X, WIFI_INFO_Y, LCD_WIDTH - 40, 150, RGB565_WHITE);

    // 标题
    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(RGB565_BLUE);
    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y);
    gfx->println("WiFi Information");
    gfx->drawLine(TEXT_START_X, WIFI_INFO_Y + 30, LCD_WIDTH - 20, WIFI_INFO_Y + 30, RGB565_BLACK);

    // 参考代码风格的WiFi信息项
    gfx->setTextSize(TEXT_SIZE_SUB);
    
    // SSID
    gfx->setTextColor(RGB565_BLUE);
    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y + LINE_SPACING);
    sprintf(t_buf, "SSID : %s", wifiSSID.c_str());
    gfx->println(t_buf);

    // Local IP
    gfx->setTextColor(RGB565_BLUE);
    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y + LINE_SPACING * 2);
    sprintf(t_buf, "  IP : %s", localIP.c_str());
    gfx->println(t_buf);

    // MAC Address
    gfx->setTextColor(RGB565_BLUE);
    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y + LINE_SPACING * 3);
    sprintf(t_buf, " MAC : %s", localMAC.c_str());
    gfx->println(t_buf);

    // Server Port
    gfx->setTextColor(RGB565_BLUE);
    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y + LINE_SPACING * 4);
    sprintf(t_buf, "SERVER PORT : %d", serverPort);
    gfx->println(t_buf);
}

// ====================== 显示收发数据 ======================
void displayDataInfo()
{
    // 清空数据区
    gfx->fillRect(TEXT_START_X, DATA_INFO_Y, LCD_WIDTH - 40, 80, RGB565_WHITE);

    gfx->setTextSize(TEXT_SIZE_SUB);
    // 发送的数据
    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(TEXT_START_X, DATA_INFO_Y);
    gfx->print("Send: ");
    gfx->setTextColor(RGB565_BLACK);
    gfx->println(sendData);

    // 接收的数据
    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(TEXT_START_X, DATA_INFO_Y + 40);
    gfx->print("Received: ");
    gfx->setTextColor(RGB565_BLACK);
    gfx->println(recvData);
}