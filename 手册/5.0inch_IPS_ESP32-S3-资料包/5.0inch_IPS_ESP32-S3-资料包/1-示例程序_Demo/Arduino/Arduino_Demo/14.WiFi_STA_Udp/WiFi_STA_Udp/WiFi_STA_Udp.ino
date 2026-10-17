/*******************************************************************************
 * ESP32 WiFi STA + WiFiUdp（同步） + RGB LCD Display
 * 适配：800*480 分辨率 | 布局全屏适配
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ====================== WiFi配置 ======================
const char *ssid = "yourssid";
const char *password = "yourpass";

// ====================== UDP配置 ======================
WiFiUDP udp;                      
unsigned int localUdpPort = 10000;
unsigned int broadcastPort = localUdpPort;
const char *broadcastData = "broadcast UDP data";
char udpRecvBuffer[256] = {0};    

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
char t_buf[100] = {0};
unsigned long lastBroadcastTime = 0;
const unsigned long BROADCAST_INTERVAL = 5000;

// ====================== 函数声明 ======================
void initWiFi();
void initUDP();
void handleUdpPacket();
void displayWiFiInfo();
void displayUdpStatus(const char* status, uint16_t color);
void displayUdpPacketInfo(IPAddress remoteIP, uint16_t remotePort, int dataLen, const char* data);

// ====================== 初始化函数 ======================
void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("ESP32 WiFiUdp 800*480");

  if (!gfx->begin())
  {
    Serial.println("LCD init failed!");
    while (1) delay(1000);
  }
  gfx->fillScreen(RGB565_WHITE);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  initWiFi();

  if(WiFi.status() == WL_CONNECTED)
  {
    initUDP();
  }
}

// ====================== 循环函数 ======================
void loop()
{
  handleUdpPacket();

  if(WiFi.status() == WL_CONNECTED && millis() - lastBroadcastTime > BROADCAST_INTERVAL)
  {
    udp.beginPacket(IPAddress(255,255,255,255), broadcastPort);
    udp.print(broadcastData);
    udp.endPacket();
    
    Serial.printf("Broadcast UDP: %s to port %d\n", broadcastData, broadcastPort);
    lastBroadcastTime = millis();
    displayUdpStatus("UDP Broadcast Sent", RGB565_GREEN);
  }

  delay(10);
}

// ====================== WiFi初始化 ======================
void initWiFi()
{
  gfx->setTextColor(RGB565_RED);
  gfx->setTextSize(3);
  gfx->setCursor(40, 80);
  gfx->println("Connecting WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  int retryCount = 0;
  while(WiFi.status() != WL_CONNECTED && retryCount < 20)
  {
    delay(500);
    Serial.print(".");
    retryCount++;
  }

  if(WiFi.status() == WL_CONNECTED)
  {
    gfx->fillRect(40, 80, LCD_WIDTH - 80, 40, RGB565_WHITE);
    gfx->setTextColor(RGB565_GREEN);
    gfx->setCursor(40, 80);
    gfx->println("WiFi Connected!");
    displayWiFiInfo();
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
  }
  else
  {
    gfx->fillRect(40, 80, LCD_WIDTH - 80, 40, RGB565_WHITE);
    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(40, 80);
    gfx->println("WiFi Connect Failed!");
    Serial.println("WiFi connect failed!");
  }
}

// ====================== UDP初始化 ======================
void initUDP()
{
  if(udp.begin(localUdpPort))
  {
    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(40, 280);
    gfx->println("UDP Listen Success!");
    
    String statusStr = "UDP Listening on " + WiFi.localIP().toString() + ":" + String(localUdpPort);
    displayUdpStatus(statusStr.c_str(), RGB565_BLUE); 
    
    Serial.println("UDP listening on port: " + String(localUdpPort));
  }
  else
  {
    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(40, 280);
    gfx->println("UDP Listen Failed!");
    Serial.println("UDP listen failed!");
  }
}

// ====================== 同步处理UDP数据包 ======================
void handleUdpPacket()
{
  int packetSize = udp.parsePacket();
  if(packetSize > 0 && packetSize < sizeof(udpRecvBuffer))
  {
    IPAddress remoteIP = udp.remoteIP();
    uint16_t remotePort = udp.remotePort();
    
    int len = udp.read(udpRecvBuffer, sizeof(udpRecvBuffer) - 1);
    udpRecvBuffer[len] = '\0';

    displayUdpPacketInfo(remoteIP, remotePort, len, udpRecvBuffer);
    
    udp.beginPacket(remoteIP, remotePort);
    udp.print("reply data");
    udp.endPacket();

    broadcastPort = remotePort;

    Serial.printf("\nUDP Packet from %s:%d, len:%d\n", remoteIP.toString().c_str(), remotePort, len);
    Serial.print("Data: ");
    Serial.println(udpRecvBuffer);
  }
}

// ====================== 显示WiFi信息 ======================
void displayWiFiInfo()
{
  gfx->setTextColor(RGB565_BLUE);
  gfx->setTextSize(3);
  
  sprintf(t_buf, "SSID : %s", ssid);
  gfx->setCursor(40, 130);
  gfx->println(t_buf);
  
  sprintf(t_buf, "  IP : %s", WiFi.localIP().toString().c_str());
  gfx->setCursor(40, 170);
  gfx->println(t_buf);
  
  sprintf(t_buf, "LOCAL PORT : %d", localUdpPort);
  gfx->setCursor(40, 210);
  gfx->println(t_buf);
}

// ====================== 显示UDP状态 ======================
void displayUdpStatus(const char* status, uint16_t color)
{
  gfx->fillRect(40, 320, LCD_WIDTH - 80, 30, RGB565_WHITE);
  gfx->setTextColor(color);
  gfx->setTextSize(2); 
  gfx->setCursor(40, 320);
  gfx->println(status);
}

// ====================== 显示UDP数据包信息 ======================
void displayUdpPacketInfo(IPAddress remoteIP, uint16_t remotePort, int dataLen, const char* data)
{
  gfx->fillRect(40, 360, LCD_WIDTH - 80, 100, RGB565_WHITE);
  gfx->setTextSize(3);
  gfx->setTextColor(RGB565_BLACK);
  
  sprintf(t_buf, "REMOTE : %s:%d", remoteIP.toString().c_str(), remotePort);
  gfx->setCursor(40, 360);
  gfx->println(t_buf);
  
  sprintf(t_buf, "LEN : %d", dataLen);
  gfx->setCursor(40, 400);
  gfx->println(t_buf);
  
  gfx->setCursor(40, 440);
  gfx->print("DATA : ");
  gfx->println(String(data).substring(0, 25));
}