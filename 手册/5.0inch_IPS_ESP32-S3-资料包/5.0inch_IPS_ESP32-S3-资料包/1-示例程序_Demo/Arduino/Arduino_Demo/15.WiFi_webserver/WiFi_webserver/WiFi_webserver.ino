/*******************************************************************************
 * ESP32-S3 AP模式 固定显示AP信息 + 网页调光
 * 功能：显示AP信息+连接数 网页后台调光/调色
 * 适配：800*480 RGB LCD 全屏布局
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WebServer.h>
#include <cmath>

// ====================== 硬件配置 800*480 ======================
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

// 引脚定义
#define PIN_DE 40
#define PIN_VSYNC 41
#define PIN_HSYNC 39
#define PIN_PCLK 42
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

// ====================== WiFi AP 配置 ======================
const char *ap_ssid = "yourssid";
const char *ap_password = "yourpassword";
WebServer server(80);

// ====================== 全局变量 ======================
Arduino_ESP32RGBPanel *rgb_panel;
Arduino_RGB_Display *gfx;
int brightness = 128;
uint16_t current_color = RGB565_WHITE;
char t_buf[100] = {0};

// ====================== 网页HTML内容 ======================
const char* html_page = R"HTML(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 LCD调光</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 50px; }
    .slider { width: 300px; margin: 20px auto; }
    .btn {
      width: 80px; height: 40px; margin: 10px;
      border: none; border-radius: 5px; color: white; font-size: 16px;
    }
    .red { background-color: red; }
    .green { background-color: green; }
    .blue { background-color: blue; }
    .white { background-color: gray; }
    .brightness { font-size: 20px; margin: 20px; }
  </style>
</head>
<body>
  <h1>ESP32 LCD 调光控制</h1>

  <div class="brightness">亮度: <span id="brightness_val">128</span></div>
  <input type="range" min="0" max="255" value="128" class="slider" id="brightness" onchange="setBrightness(this.value)">

  <div>
    <button class="btn red" onclick="setColor('red')">红色</button>
    <button class="btn green" onclick="setColor('green')">绿色</button>
    <button class="btn blue" onclick="setColor('blue')">蓝色</button>
    <button class="btn white" onclick="setColor('white')">白色</button>
  </div>

  <script>
    function setBrightness(val) {
      document.getElementById('brightness_val').innerText = val;
      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/setBrightness?val=' + val, true);
      xhr.send();
    }
    function setColor(color) {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/setColor?color=' + color, true);
      xhr.send();
    }
  </script>
</body>
</html>
)HTML";

// ====================== Web服务器处理函数 ======================
void handleRoot() {
  server.send(200, "text/html", html_page);
}

void handleSetBrightness() {
  if (server.hasArg("val")) {
    brightness = server.arg("val").toInt();
    brightness = constrain(brightness, 0, 255);
    analogWrite(TFT_BL, brightness);
    Serial.print("亮度已设置为: ");
    Serial.println(brightness);
  }
  server.send(200, "text/plain", "OK");
}

void handleSetColor() {
  if (server.hasArg("color")) {
    String color = server.arg("color");
    if (color == "red") current_color = RGB565_RED;
    else if (color == "green") current_color = RGB565_GREEN;
    else if (color == "blue") current_color = RGB565_BLUE;
    else if (color == "white") current_color = RGB565_WHITE;

    drawAPInfo();
    Serial.print("颜色已设置为: ");
    Serial.println(color);
  }
  server.send(200, "text/plain", "OK");
}

// ====================== 绘制AP信息界面 ======================
void drawAPInfo() {
  gfx->fillScreen(current_color);

  // 标题
  gfx->setTextColor(RGB565_GREEN);
  gfx->setTextSize(3);
  gfx->setCursor(50, 120);
  gfx->println("Enable ESP32 WIFI AP mode...");

  // SSID
  gfx->setTextColor(RGB565_GREEN);
  sprintf(t_buf, "SSID : %s", ap_ssid);
  gfx->setCursor(50, 180);
  gfx->println(t_buf);

  // PASSWORD
  sprintf(t_buf, "PASSWORD : %s", ap_password);
  gfx->setCursor(50, 230);
  gfx->println(t_buf);

  // HOST IP
  sprintf(t_buf, "HOST IP : %s", WiFi.softAPIP().toString().c_str());
  gfx->setCursor(50, 280);
  gfx->println(t_buf);

  // HOST MAC
  sprintf(t_buf, "HOST MAC : %s", WiFi.softAPmacAddress().c_str());
  gfx->setCursor(50, 330);
  gfx->println(t_buf);

  // 连接数
  gfx->setCursor(50, 380);
  gfx->print("CONNECTED NUMBER : ");
}

// ====================== 更新连接数 ======================
void updateConnectedNum() {
  gfx->fillRect(400, 380, gfx->width() - 450, 30, current_color);
  gfx->setTextColor(RGB565_GREEN);
  gfx->setTextSize(3);
  sprintf(t_buf, "%d", WiFi.softAPgetStationNum());
  gfx->setCursor(400, 380);
  gfx->print(t_buf);
}

// ====================== WiFi AP初始化 ======================
void initWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.disconnect(true);
  delay(500);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("AP SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());
}

// ====================== 初始化函数 ======================
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("ESP32 AP Mode 800*480");

  // LCD初始化
  rgb_panel = new Arduino_ESP32RGBPanel(
    PIN_DE, PIN_VSYNC, PIN_HSYNC, PIN_PCLK,
    PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4,
    PIN_G0, PIN_G1, PIN_G2, PIN_G3, PIN_G4, PIN_G5,
    PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B4,
    HSYNC_POLARITY, HSYNC_FRONT_PORCH, HSYNC_PULSE_WIDTH, HSYNC_BACK_PORCH,
    VSYNC_POLARITY, VSYNC_FRONT_PORCH, VSYNC_PULSE_WIDTH, VSYNC_BACK_PORCH,
    PCLK_ACTIVE_NEG, PCLK_FREQ);

  gfx = new Arduino_RGB_Display(LCD_WIDTH, LCD_HEIGHT, rgb_panel, 0, true);
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
    while (1) delay(1000);
  }

  // 背光初始化
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, brightness);

  initWiFiAP();
  drawAPInfo();

  // Web服务器
  server.on("/", handleRoot);
  server.on("/setBrightness", handleSetBrightness);
  server.on("/setColor", handleSetColor);
  server.begin();
  Serial.println("Web Server Started");
}

// ====================== 循环函数 ======================
void loop() {
  server.handleClient();

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    updateConnectedNum();
    lastUpdate = millis();
  }

  delay(10);
}