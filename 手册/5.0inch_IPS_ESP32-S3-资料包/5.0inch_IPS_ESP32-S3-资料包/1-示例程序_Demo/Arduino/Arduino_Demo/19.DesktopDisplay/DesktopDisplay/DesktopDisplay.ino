/*******************************************************************************
 * ESP32 WIFI + RGB LCD (800x480)
 * 功能：天气时钟，接收天气与时间信息，显示数据
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

// ====================== 网络配置（请修改为你的WiFi名称和密码） ======================
const char* WIFI_SSID = "ESP";    // WiFi名称
const char* WIFI_PASSWORD = "12345678"; // WiFi密码
const uint32_t CONNECT_TIMEOUT = 10000; // 连接超时时间：10秒（10000毫秒）
const uint32_t PROGRESS_UPDATE_INTERVAL = 100; // 进度条更新间隔：100毫秒

// ====================== 天气与时间配置 ======================
// NTP服务器（阿里云）
static const char ntpServerName[] = "ntp6.aliyun.com";
const int timeZone = 8;     // 东八区
// 天气相关配置
String cityCode = "101280601";  // 默认城市代码（可根据连接WIFI的IP自动获取）
unsigned long weatherUpdateTime = 0; // 天气更新时间戳
const uint32_t WEATHER_UPDATE_INTERVAL = 300000; // 天气更新间隔：5分钟（300000毫秒）
// 天气数据存储
String cityName = "";
String temp = "";
String humidity = "";
String weather = "";
String aqiText = "";
String windDir = "";
String windSpeed = "";
String todayWeather = "";
String tempMin = "";
String tempMax = "";

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


// ====================== 进度条配置 ======================
#define PROGRESS_BAR_X 50          // 进度条起始X坐标
#define PROGRESS_BAR_Y 200         // 进度条起始Y坐标
#define PROGRESS_BAR_WIDTH 700     // 进度条宽度
#define PROGRESS_BAR_HEIGHT 30     // 进度条高度
#define PROGRESS_BAR_RADIUS 5      // 进度条圆角半径
#define PROGRESS_BAR_BG_COLOR RGB565_NAVY // 进度条背景颜色
#define PROGRESS_BAR_FILL_COLOR RGB565_BLUE // 进度条填充颜色

// ====================== NTP相关配置 ======================
WiFiUDP Udp;
unsigned int localPort = 8000;
const int NTP_PACKET_SIZE = 48; // NTP数据包大小
byte packetBuffer[NTP_PACKET_SIZE]; // NTP数据包缓冲区

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
//联网相关函数
void drawConnectingScreen(float progress); // 绘制联网中页面和进度条
void drawFailedScreen(); // 绘制连接失败页面
bool connectWiFi(); // WiFi连接函数
// 天气与时间相关函数
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
void getCityCode();
void getCityWeather();
void parseWeatherData(String *cityDZ, String *dataSK, String *dataFC);
void displayWeatherAndTime();
String getWeekString();
String getDateString();
// ====================== Setup函数 ======================
void setup(void)
{
    // 初始化串口
    Serial.begin(115200);
    delay(100);

    // 初始化显示屏
    if (!gfx->begin())
    {
        Serial.println("gfx->begin() failed!");
        while (1) delay(1000); // 初始化失败则死循环
    }

    // 开启背光
#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif

    // 清屏（黑色）
    gfx->fillScreen(RGB565_BLACK);
    gfx->setUTF8Print(true);
    // 开始连接WiFi并显示进度
    bool connectSuccess = connectWiFi();

    // 若连接失败，显示失败页面
    if (!connectSuccess)
    {
        drawFailedScreen();
    }
    else
    {
        // 连接成功后的操作
        gfx->fillScreen(RGB565_BLACK);
        gfx->setCursor(300, 150);
        gfx->setFont(u8g2_font_unifont_t_gb2312);
        gfx->setTextColor(RGB565_GREEN);
        gfx->setTextSize(3);
        gfx->println("连接成功!");
        gfx->setCursor(150, 230);
        gfx->setFont(u8g2_font_unifont_t_gb2312);
        gfx->setTextColor(RGB565_GREEN);
        gfx->setTextSize(3);
        gfx->println("即将进入天气时钟页面");
        Serial.println("Starting UDP...");
        Udp.begin(localPort);
        Serial.println("Waiting for NTP sync...");
        setSyncProvider(getNtpTime);
        setSyncInterval(300); // 每5分钟同步一次时间

        // 获取城市代码
        getCityCode();
        // 获取初始天气数据
        getCityWeather();
        //清屏
        gfx->fillScreen(RGB565_BLACK);
        // 显示初始的天气和时间
        displayWeatherAndTime();
    }
}

// ====================== Loop函数 ======================
void loop()
{
    // 如果WiFi已连接
    if (WiFi.status() == WL_CONNECTED)
    {
        // 实时刷新时间
        static time_t prevDisplay = 0;
        if (now() != prevDisplay)
        {
            prevDisplay = now();
            displayWeatherAndTime();
        }

        // 每5分钟更新一次天气
        if (millis() - weatherUpdateTime > WEATHER_UPDATE_INTERVAL)
        {
            gfx->fillScreen(RGB565_BLACK);//清屏
            weatherUpdateTime = millis();
            getCityWeather();
        }
            
}
}
// ====================== WiFi连接函数 ======================
bool connectWiFi()
{
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA); // 设置为STA模式
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // 开始连接

    uint32_t startTime = millis(); // 记录开始连接的时间
    float progress = 0.0; // 进度（0.0 ~ 1.0）

    // 循环等待连接完成或超时
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < CONNECT_TIMEOUT)
    {
        // 计算进度（已用时间 / 超时时间）
        progress = (float)(millis() - startTime) / CONNECT_TIMEOUT;
        // 绘制联网中页面和进度条
        drawConnectingScreen(progress);
        // 控制进度条更新频率
        delay(PROGRESS_UPDATE_INTERVAL);
        Serial.print(".");
    }

    Serial.println();

    // 判断连接结果
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi connected successfully!");
        return true;
    }
    else
    {
        Serial.println("WiFi connection failed!");
        return false;
    }
}

// ====================== 绘制联网中页面和进度条 ======================
void drawConnectingScreen(float progress)
{

    // 显示标题：Connecting to WiFi...
    gfx->setCursor(300, 150);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextColor(RGB565_WHITE);
    gfx->setTextSize(3);
    gfx->print("连接中... ");

    // 绘制进度条背景（圆角矩形）
    gfx->fillRoundRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT,
                       PROGRESS_BAR_RADIUS, PROGRESS_BAR_BG_COLOR);

    // 计算进度条填充宽度（限制最大为进度条总宽度）
    int fillWidth = (int)(progress * PROGRESS_BAR_WIDTH);
    fillWidth = constrain(fillWidth, 0, PROGRESS_BAR_WIDTH);

    // 绘制进度条填充部分（蓝色圆角矩形）
    if (fillWidth > 0)
    {
        gfx->fillRoundRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, fillWidth, PROGRESS_BAR_HEIGHT,
                           PROGRESS_BAR_RADIUS, PROGRESS_BAR_FILL_COLOR);
    }

    // 显示进度百分比
    gfx->setCursor(PROGRESS_BAR_X + PROGRESS_BAR_WIDTH / 2 - 30, PROGRESS_BAR_Y + PROGRESS_BAR_HEIGHT + 10);
}

// ====================== 绘制连接失败页面 ======================
void drawFailedScreen()
{
    // 清屏（黑色背景）
    gfx->fillScreen(RGB565_BLACK);

    // 显示连接失败标题（红色）
    gfx->setCursor(50, 100);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextColor(RGB565_RED);
    gfx->setTextSize(4);
    gfx->println("连接失败!");

    // 显示提示信息（红色）
    gfx->setCursor(50, 180);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextColor(RGB565_RED);
    gfx->setTextSize(2);
    gfx->println("请重新启动并检查网络信息是否填写正确");


    // 串口打印提示
    Serial.println("Connection Failed! Please restart and check WiFi info.");
}

// ====================== NTP时间获取函数 ======================
time_t getNtpTime()
{
    IPAddress ntpServerIP; // NTP服务器IP地址

    // 丢弃之前的数据包
    while (Udp.parsePacket() > 0) ;
    Serial.println("Transmit NTP Request");

    // 获取NTP服务器IP
    WiFi.hostByName(ntpServerName, ntpServerIP);
    sendNTPpacket(ntpServerIP);

    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500)
    {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE)
        {
            Serial.println("Receive NTP Response");
            Udp.read(packetBuffer, NTP_PACKET_SIZE); // 读取数据包

            // 解析NTP时间
            unsigned long secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];

            // 转换为Unix时间（减去1900到1970的秒数，加上时区）
            return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
        }
    }

    Serial.println("No NTP Response :-(");
    return 0; // 无法获取时间时返回0
}
// 发送NTP数据包
void sendNTPpacket(IPAddress &address)
{
    // 清空缓冲区
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // 设置NTP请求头
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0; // Stratum
    packetBuffer[2] = 6; // Polling Interval
    packetBuffer[3] = 0xEC; // Peer Clock Precision
    // 填充标识字段
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // 发送数据包
    Udp.beginPacket(address, 123); // NTP端口为123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}

// ====================== 城市代码获取函数 ======================
void getCityCode()
{
    String URL = "http://wgeo.weather.com.cn/ip/?_=" + String(now());
    HTTPClient httpClient;

    // 配置请求
    httpClient.begin(URL);
    // 设置请求头（模拟浏览器，避免被拦截）
    httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
    httpClient.addHeader("Referer", "http://www.weather.com.cn/");

    // 发送GET请求
    int httpCode = httpClient.GET();
    Serial.print("Send GET request to URL: ");
    Serial.println(URL);

    if (httpCode == HTTP_CODE_OK)
    {
        String str = httpClient.getString();
        int aa = str.indexOf("id=");
        if (aa > -1)
        {
            // 提取城市代码（9位）
            cityCode = str.substring(aa + 4, aa + 4 + 9);
            Serial.print("City Code: ");
            Serial.println(cityCode);
        }
        else
        {
            Serial.println("Failed to get city code, use default.");
        }
    }
    else
    {
        Serial.print("Get city code error: ");
        Serial.println(httpCode);
        Serial.println("Use default city code.");
    }

    httpClient.end(); // 关闭连接
}

// ====================== 天气数据获取函数 ======================
void getCityWeather()
{
    String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_=" + String(now());
    HTTPClient httpClient;
    WiFiClient wifiClient;

    // 配置请求
    httpClient.begin(wifiClient, URL);
    // 设置请求头（模拟浏览器）
    httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
    httpClient.addHeader("Referer", "http://www.weather.com.cn/");

    // 发送GET请求
    int httpCode = httpClient.GET();
    Serial.println("Getting weather data...");
    Serial.println(URL);

    if (httpCode == HTTP_CODE_OK)
    {
        String str = httpClient.getString();

        // 提取天气信息JSON片段
        // 1. 城市基础信息
        int indexStart = str.indexOf("weatherinfo\":");
        int indexEnd = str.indexOf("};var alarmDZ");
        String jsonCityDZ = str.substring(indexStart + 13, indexEnd);

        // 2. 实时天气数据
        indexStart = str.indexOf("dataSK =");
        indexEnd = str.indexOf(";var dataZS");
        String jsonDataSK = str.substring(indexStart + 8, indexEnd);

        // 3. 今日天气预报
        indexStart = str.indexOf("\"f\":[");
        indexEnd = str.indexOf(",{\"fa");
        String jsonFC = str.substring(indexStart + 5, indexEnd);

        // 解析天气数据
        parseWeatherData(&jsonCityDZ, &jsonDataSK, &jsonFC);
        Serial.println("Weather data updated.");
    }
    else
    {
        Serial.print("Get weather error: ");
        Serial.println(httpCode);
    }

    httpClient.end(); // 关闭连接
}

// ====================== 天气数据解析函数 ======================
void parseWeatherData(String *cityDZ, String *dataSK, String *dataFC)
{
    DynamicJsonDocument doc(2048); // 增大缓冲区，避免解析失败

    // 解析实时天气数据
    deserializeJson(doc, *dataSK);
    JsonObject sk = doc.as<JsonObject>();

    // 存储实时数据
    cityName = sk["cityname"].as<String>();
    temp = sk["temp"].as<String>();
    humidity = sk["SD"].as<String>();
    weather = sk["weather"].as<String>();
    windDir = sk["WD"].as<String>();
    windSpeed = sk["WS"].as<String>();

    // 解析空气质量
    int aqi = sk["aqi"].as<int>();
    if (aqi > 200)
    {
        aqiText = "重度污染";
    }
    else if (aqi > 150)
    {
        aqiText = "中度污染";
    }
    else if (aqi > 100)
    {
        aqiText = "轻度污染";
    }
    else if (aqi > 50)
    {
        aqiText = "良";
    }
    else
    {
        aqiText = "优";
    }

    // 解析今日天气
    deserializeJson(doc, *cityDZ);
    JsonObject dz = doc.as<JsonObject>();
    todayWeather = dz["weather"].as<String>();

    // 解析今日最高/最低温度
    deserializeJson(doc, *dataFC);
    JsonObject fc = doc.as<JsonObject>();
    tempMin = fc["fd"].as<String>();
    tempMax = fc["fc"].as<String>();
}

// ====================== 天气与时间显示函数 ======================
void displayWeatherAndTime()
{
    // ===== 第一部分：时间与日期 =====
    gfx->setCursor(50, 50);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextColor(RGB565_WHITE,RGB565_BLACK);
    gfx->setTextSize(3);
    gfx->print("当前时间：");

    // 显示时间（时:分:秒，补零）
    gfx->setCursor(50, 110);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextSize(4);
    gfx->setTextColor(RGB565_CYAN,RGB565_BLACK);
    gfx->fillRect(50, 60, 400, 50, RGB565_BLACK);
    if (hour() < 10) gfx->print("0");
    gfx->print(hour());
    gfx->print(":");
    if (minute() < 10) gfx->print("0");
    gfx->print(minute());
    gfx->print(":");
    if (second() < 10) gfx->print("0");
    gfx->print(second());

    // 显示日期和星期
    gfx->setCursor(50, 170);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextSize(3);
    gfx->setTextColor(RGB565_YELLOW,RGB565_BLACK);
    gfx->print("日期：");
    gfx->print(getDateString());
    gfx->print("  ");
    gfx->print(getWeekString());

    // ===== 第二部分：天气信息 =====
    gfx->setCursor(50, 230);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextSize(3);
    gfx->setTextColor(RGB565_GREEN,RGB565_BLACK);
    gfx->print("城市：");
    gfx->print(cityName);

    // 实时天气数据（分多行显示，整齐布局）
    gfx->setCursor(50, 300);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextSize(2);
    gfx->setTextColor(RGB565_WHITE,RGB565_BLACK);
    gfx->print("实时天气：");
    gfx->print(weather);
    gfx->print("  空气质量：");
    gfx->print(aqiText);
    

    gfx->setCursor(50, 340);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextSize(2);
    gfx->setTextColor(RGB565_WHITE,RGB565_BLACK);
    gfx->print("湿度：");
    gfx->print(humidity);
    gfx->print("  温度：");
    gfx->print(temp);
    gfx->print("℃");

    gfx->setCursor(50, 380);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextSize(2);
    gfx->setTextColor(RGB565_WHITE,RGB565_BLACK);
    gfx->print("风向：");
    gfx->print(windDir);
    gfx->print(windSpeed);

    gfx->setCursor(50, 420);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextSize(2);
    gfx->setTextColor(RGB565_WHITE,RGB565_BLACK);
    gfx->print("今日天气：");
    gfx->print(todayWeather);
    gfx->print("  温度：");
    gfx->print(tempMin);
    gfx->print("℃ ~ ");
    gfx->print(tempMax);
    gfx->print("℃");
}

// 获取星期字符串（中文）
String getWeekString()
{
    String wk[7] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
    return wk[weekday() - 1];
}

// 获取日期字符串（年-月-日，中文）
String getDateString()
{
    String s = String(year()) + "年" + month() + "月" + day() + "日";
    return s;
}