/*******************************************************************************
 * ESP32 WiFi STA + TCP Server + RGB LCD Display
 * 功能：ESP32 STA模式连接WiFi，作为TCP服务器监听指定端口，
 *       屏幕显示WiFi/TCP连接状态、网络信息及收发的数据
 * 硬件：ESP32-S3 + RGB LCD 800*480
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WiFiServer.h>
#include <WiFiClient.h>

// ====================== WiFi STA配置 ======================
#define WIFI_SSID         "yourssid"
#define WIFI_PASSWORD     "yourpass"
#define WIFI_CONNECT_RETRY 5
#define WIFI_CONNECT_DELAY 3000

// ====================== TCP Server配置 ======================
#define TCP_SERVER_PORT   10000
#define TCP_CLIENT_TIMEOUT 5000
#define TCP_RECV_BUFFER_SIZE 1024
#define MAX_CLIENTS 1

// ====================== 硬件配置 800*480 ======================
#define LCD_WIDTH  800
#define LCD_HEIGHT 480
#define TFT_BL 2

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

// ====================== 显示布局适配 800*480 ======================
#define TEXT_SIZE_MAIN 3     // 主字号放大
#define TEXT_SIZE_SUB 2      // 子字号放大
#define TEXT_START_X 30
#define TEXT_START_Y 30
#define LINE_SPACING 50      // 行高加宽
#define WIFI_INFO_Y 120      // WiFi信息区Y坐标
#define TCP_INFO_Y  380      // TCP状态区Y坐标
#define DATA_INFO_Y 420      // 数据收发区Y坐标

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
bool isWiFiConnected = false;
WiFiServer server(TCP_SERVER_PORT);
WiFiClient client;
char t_buf[100] = {0};

String wifiSSID = WIFI_SSID;
String localIP;
String localMAC;

uint16_t serverPort = TCP_SERVER_PORT;
String recvData = "";
String sendData = "Hello, this is server!";
bool isClientConnected = false;

// ====================== 函数声明 ======================
void initWiFiSTA();
void displayWiFiStatus(const String &status, uint16_t color);
void displayWiFiInfo();
void displayTCPStatus(const String &status, uint16_t color);
void displayDataInfo();

// ====================== 初始化函数 ======================
void setup()
{
    Serial.begin(115200);
    Serial.println("ESP32 WiFi STA + TCP Server 800*480");

    if (!gfx->begin())
    {
        Serial.println("gfx->begin() failed!");
        while (1) delay(1000);
    }
    gfx->fillScreen(RGB565_WHITE);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    displayWiFiStatus("Start connecting to WiFi ...", RGB565_RED);
    Serial.printf("Connecting to WiFi: %s...\n", WIFI_SSID);

    initWiFiSTA();

    if (isWiFiConnected)
    {
        server.begin();
        server.setTimeout(TCP_CLIENT_TIMEOUT);
        displayTCPStatus("TCP Server Started, Listening...", RGB565_GREEN);
        Serial.printf("TCP Server started on %s:%d\n", localIP.c_str(), serverPort);
    }
    else
    {
        displayTCPStatus("WiFi Failed, Server Not Started", RGB565_RED);
        Serial.println("WiFi connect failed, cannot start TCP server!");
    }
}

// ====================== 循环函数 ======================
void loop()
{
    if (isWiFiConnected)
    {
        if (!isClientConnected)
        {
            client = server.available();
            if (client)
            {
                isClientConnected = true;
                String clientIP = client.remoteIP().toString();
                String statusMsg = "Client Connected: " + clientIP;
                displayTCPStatus(statusMsg, RGB565_GREEN);
                Serial.printf("New client connected: %s\n", clientIP.c_str());

                client.print(sendData);
                Serial.printf("TCP Send: %s\n", sendData.c_str());
                displayDataInfo();
            }
            else
            {
                displayTCPStatus("Waiting for client...", RGB565_ORANGE);
            }
        }
        else
        {
            if (client.connected())
            {
                if (client.available() > 0)
                {
                    String line = "";
                    while (client.available() > 0)
                    {
                        line += (char)client.read();
                    }
                    recvData = line;
                    Serial.printf("TCP Recv from client: %s\n", recvData.c_str());

                    displayDataInfo();

                    String reply = "Server received: " + recvData;
                    client.print(reply);
                    sendData = reply;
                    Serial.printf("TCP Send to client: %s\n", sendData.c_str());
                    displayDataInfo();
                }
            }
            else
            {
                isClientConnected = false;
                recvData = "";
                sendData = "Hello, this is server!";
                displayTCPStatus("Client Disconnected", RGB565_RED);
                Serial.println("Client disconnected!");
                client.stop();
            }
        }
    }
    delay(10);
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

// ====================== 显示WiFi状态 ======================
void displayWiFiStatus(const String &status, uint16_t color)
{
    gfx->fillRect(TEXT_START_X, TEXT_START_Y, LCD_WIDTH - 60, 80, RGB565_WHITE);
    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(color);
    gfx->setCursor(TEXT_START_X, TEXT_START_Y + 20);
    gfx->println(status);
    gfx->drawLine(TEXT_START_X, TEXT_START_Y + 70, LCD_WIDTH - 30, TEXT_START_Y + 70, RGB565_BLACK);
}

// ====================== 显示TCP状态 ======================
void displayTCPStatus(const String &status, uint16_t color)
{
    gfx->fillRect(TEXT_START_X, TCP_INFO_Y - 60, LCD_WIDTH - 60, 60, RGB565_WHITE);
    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(color);
    gfx->setCursor(TEXT_START_X, TCP_INFO_Y - 40);
    gfx->println(status);
}

// ====================== 显示WiFi信息 ======================
void displayWiFiInfo()
{
    gfx->fillRect(TEXT_START_X, WIFI_INFO_Y, LCD_WIDTH - 60, 240, RGB565_WHITE);

    gfx->setTextSize(TEXT_SIZE_MAIN);
    gfx->setTextColor(RGB565_BLUE);
    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y);
    gfx->println("WiFi Information");
    gfx->drawLine(TEXT_START_X, WIFI_INFO_Y + 45, LCD_WIDTH - 30, WIFI_INFO_Y + 45, RGB565_BLACK);

    gfx->setTextSize(TEXT_SIZE_SUB);
    
    gfx->setTextColor(RGB565_BLUE);
    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y + LINE_SPACING);
    sprintf(t_buf, "SSID : %s", wifiSSID.c_str());
    gfx->println(t_buf);

    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y + LINE_SPACING * 2);
    sprintf(t_buf, "  IP : %s", localIP.c_str());
    gfx->println(t_buf);

    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y + LINE_SPACING * 3);
    sprintf(t_buf, " MAC : %s", localMAC.c_str());
    gfx->println(t_buf);

    gfx->setCursor(TEXT_START_X, WIFI_INFO_Y + LINE_SPACING * 4);
    sprintf(t_buf, "SERVER PORT : %d", serverPort);
    gfx->println(t_buf);
}

// ====================== 显示收发数据 ======================
void displayDataInfo()
{
    gfx->fillRect(TEXT_START_X, DATA_INFO_Y, LCD_WIDTH - 60, 100, RGB565_WHITE);

    gfx->setTextSize(TEXT_SIZE_SUB);
    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(TEXT_START_X, DATA_INFO_Y);
    gfx->print("Send: ");
    gfx->setTextColor(RGB565_BLACK);
    gfx->println(sendData);

    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(TEXT_START_X, DATA_INFO_Y + 50);
    gfx->print("Received: ");
    gfx->setTextColor(RGB565_BLACK);
    gfx->println(recvData);
}