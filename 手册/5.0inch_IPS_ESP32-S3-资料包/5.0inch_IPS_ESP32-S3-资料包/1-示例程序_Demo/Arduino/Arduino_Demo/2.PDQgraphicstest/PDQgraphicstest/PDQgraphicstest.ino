/*******************************************************************************
 * RGB LCD (800x480)
 * 功能：测试RGB图形显示
 * 硬件：ESP32-S3 + 800x480 RGB LCD
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <random> // 用于生成随机颜色

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
#define PAGE_DISPLAY_TIME 3000    // 每个页面显示3秒
#define COLOR_RED     RGB565_RED
#define COLOR_GREEN   RGB565_GREEN
#define COLOR_BLUE    RGB565_BLUE
#define COLOR_BLACK   RGB565_BLACK
#define COLOR_WHITE   RGB565_WHITE
#define COLOR_YELLOW  RGB565_YELLOW
#define COLOR_CYAN    RGB565_CYAN
#define COLOR_MAGENTA RGB565_MAGENTA
#define COLOR_GRAY    RGB565_GRAY

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
void testScreenFill(); // 测试红绿蓝黑白屏填充
void drawPage1_Pixel_Text(); // 页面1：基础像素+文字
void drawPage2_Lines(); // 页面2：线条
void drawPage3_Rectangles(); // 页面3：矩形
void drawPage4_Circles_Arcs(); // 页面4：圆形/圆弧
void drawPage5_Triangles(); // 页面5：三角形
void drawPage6_RoundRects(); // 页面6：圆角矩形
uint16_t randomColor(); // 生成随机RGB565颜色

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

}

// ====================== Loop函数 ======================
void loop()
{
    // 第一步：测试红绿蓝黑白屏填充
    testScreenFill();

    // 第二步：依次绘制六大类图形页面
    drawPage1_Pixel_Text();
    delay(PAGE_DISPLAY_TIME);

    drawPage2_Lines();
    delay(PAGE_DISPLAY_TIME);

    drawPage3_Rectangles();
    delay(PAGE_DISPLAY_TIME);

    drawPage4_Circles_Arcs();
    delay(PAGE_DISPLAY_TIME);

    drawPage5_Triangles();
    delay(PAGE_DISPLAY_TIME);

    drawPage6_RoundRects();
    delay(PAGE_DISPLAY_TIME);
}

// ====================== 屏幕填充测试（红绿蓝黑白） ======================
void testScreenFill()
{
    // 红色屏
    gfx->fillScreen(COLOR_RED);
    delay(1000);
    // 绿色屏
    gfx->fillScreen(COLOR_GREEN);
    delay(1000);
    // 蓝色屏
    gfx->fillScreen(COLOR_BLUE);
    delay(1000);
    // 黑色屏
    gfx->fillScreen(COLOR_BLACK);
    delay(1000);
    // 白色屏
    gfx->fillScreen(COLOR_WHITE);
    delay(1000);
    // 回到黑色背景开始绘制图形
    gfx->fillScreen(COLOR_BLACK);
}

// ====================== 页面1：基础像素+文字 ======================
void drawPage1_Pixel_Text()
{
    gfx->fillScreen(COLOR_BLACK);
    // 1. 绘制像素矩阵（左上角100x100的像素网格，间隔5px）
    int pixel_grid_size = 100;
    int pixel_step = 5;
    for (int y = 20; y < 20 + pixel_grid_size; y += pixel_step)
    {
        for (int x = 20; x < 20 + pixel_grid_size; x += pixel_step)
        {
            gfx->drawPixel(x, y, randomColor());
        }
    }

    // 2. 绘制文字（不同大小、颜色的英文）
    // 标题
    gfx->setCursor(150, 20);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->println("Pixel & Text Test");

    // 不同大小的文字
    gfx->setCursor(150, 60);
    gfx->setTextColor(COLOR_RED);
    gfx->setTextSize(1);
    gfx->println("Text Size 1: Arduino GFX");

    gfx->setCursor(150, 80);
    gfx->setTextColor(COLOR_GREEN);
    gfx->setTextSize(2);
    gfx->println("Text Size 2: Hello World");

    gfx->setCursor(150, 120);
    gfx->setTextColor(COLOR_BLUE);
    gfx->setTextSize(3);
    gfx->println("Text Size 3: RGB LCD 800x480");

    // 带背景色的文字
    gfx->setCursor(150, 180);
    gfx->setTextColor(COLOR_YELLOW, COLOR_BLACK);
    gfx->setTextSize(2);
    gfx->println("Text with Background");

    // 数字和符号
    gfx->setCursor(150, 220);
    gfx->setTextColor(COLOR_CYAN);
    gfx->setTextSize(2);
    gfx->println("Numbers: 0123456789");
    gfx->setCursor(150, 250);
    gfx->println("Symbols: !@#$%^&*()");
}

// ====================== 页面2：线条 ======================
void drawPage2_Lines()
{
    gfx->fillScreen(COLOR_BLACK);
    // 标题
    gfx->setCursor(20, 20);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->println("Lines Test");

    // 1. 水平线（间隔30px，不同颜色）
    int line_y = 60;
    for (int i = 0; i < 5; i++)
    {
        gfx->drawFastHLine(20, line_y, LCD_WIDTH - 40, randomColor());
        line_y += 30;
    }

    // 2. 垂直线（间隔50px，不同颜色）
    int line_x = 50;
    for (int i = 0; i < 16; i++)
    {
        gfx->drawFastVLine(line_x, line_y, LCD_HEIGHT - line_y - 20, randomColor());
        line_x += 50;
    }

    // 3. 斜线（从左上角到右下角，分区绘制）
    int slash_step = 50;
    for (int x = 50; x < LCD_WIDTH - 50; x += slash_step)
    {
        gfx->drawLine(x, line_y, x + 50, line_y + 50, randomColor());
        gfx->drawLine(x + 50, line_y, x, line_y + 50, randomColor());
    }
}

// ====================== 页面3：矩形 ======================
void drawPage3_Rectangles()
{
    gfx->fillScreen(COLOR_BLACK);
    // 标题
    gfx->setCursor(20, 20);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->println("Rectangles Test");

    // 网格排列：3行4列的矩形（空心+实心交替）
    int rect_x = 20;
    int rect_y = 60;
    int rect_width = 180;
    int rect_height = 120;
    int gap_x = 20;
    int gap_y = 20;

    for (int row = 0; row < 3; row++)
    {
        rect_x = 20;
        for (int col = 0; col < 4; col++)
        {
            if (col % 2 == 0)
            {
                // 实心矩形
                gfx->fillRect(rect_x, rect_y, rect_width, rect_height, randomColor());
            }
            else
            {
                // 空心矩形
                gfx->drawRect(rect_x, rect_y, rect_width, rect_height, randomColor());
            }
            rect_x += rect_width + gap_x;
        }
        rect_y += rect_height + gap_y;
    }
}

// ====================== 页面4：圆形/圆弧 ======================
void drawPage4_Circles_Arcs()
{
    gfx->fillScreen(COLOR_BLACK);
    // 标题
    gfx->setCursor(20, 20);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->println("Circles & Arcs Test");

    // 1. 左侧：实心/空心圆形（网格排列）
    int circle_x = 50;
    int circle_y = 80;
    int circle_radius = 30;
    int circle_gap = 80;

    for (int row = 0; row < 3; row++)
    {
        circle_x = 50;
        for (int col = 0; col < 4; col++)
        {
            if (col % 2 == 0)
            {
                gfx->fillCircle(circle_x, circle_y, circle_radius, randomColor());
            }
            else
            {
                gfx->drawCircle(circle_x, circle_y, circle_radius, randomColor());
            }
            circle_x += circle_gap;
        }
        circle_y += circle_gap;
    }

    // 2. 右侧：实心/空心圆弧
    int arc_x = 500;
    int arc_y = 100;
    int arc_r = 60;
    // 实心圆弧
    gfx->fillArc(arc_x, arc_y, arc_r, arc_r - 20, 0, 180, COLOR_RED);
    // 空心圆弧
    gfx->drawArc(arc_x, arc_y + 150, arc_r, arc_r - 20, 0, 270, COLOR_BLUE);
    // 扇形（实心圆弧，内半径0）
    gfx->fillArc(arc_x, arc_y + 300, arc_r, 0, 45, 135, COLOR_GREEN);
}

// ====================== 页面5：三角形 ======================
void drawPage5_Triangles()
{
    gfx->fillScreen(COLOR_BLACK);
    // 标题
    gfx->setCursor(20, 20);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->println("Triangles Test");

    // 网格排列：4行8列的三角形（实心+空心交替）
    int tri_x = 10;
    int tri_y = 80;
    int tri_size = 80;
    int gap_x = 100;
    int gap_y = 100;

    for (int row = 0; row < 4; row++)
    {
        tri_x = 10;
        for (int col = 0; col < 8; col++)
        {
            // 三角形三个顶点坐标
            int x1 = tri_x + tri_size / 2;
            int y1 = tri_y;
            int x2 = tri_x;
            int y2 = tri_y + tri_size;
            int x3 = tri_x + tri_size;
            int y3 = tri_y + tri_size;

            if (col % 2 == 0)
            {
                gfx->fillTriangle(x1, y1, x2, y2, x3, y3, randomColor());
            }
            else
            {
                gfx->drawTriangle(x1, y1, x2, y2, x3, y3, randomColor());
            }
            tri_x += gap_x;
        }
        tri_y += gap_y;
    }
}

// ====================== 页面6：圆角矩形 ======================
void drawPage6_RoundRects()
{
    gfx->fillScreen(COLOR_BLACK);
    // 标题
    gfx->setCursor(20, 20);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->println("Round Rectangles Test");

    // 网格排列：3行4列的圆角矩形（实心+空心交替）
    int rrect_x = 20;
    int rrect_y = 60;
    int rrect_width = 180;
    int rrect_height = 120;
    int rrect_radius = 20;
    int gap_x = 20;
    int gap_y = 20;

    for (int row = 0; row < 3; row++)
    {
        rrect_x = 20;
        for (int col = 0; col < 4; col++)
        {
            if (col % 2 == 0)
            {
                // 实心圆角矩形
                gfx->fillRoundRect(rrect_x, rrect_y, rrect_width, rrect_height, rrect_radius, randomColor());
            }
            else
            {
                // 空心圆角矩形
                gfx->drawRoundRect(rrect_x, rrect_y, rrect_width, rrect_height, rrect_radius, randomColor());
            }
            rrect_x += rrect_width + gap_x;
        }
        rrect_y += rrect_height + gap_y;
    }
}

// ====================== 辅助函数：生成随机RGB565颜色 ======================
uint16_t randomColor()
{
    // RGB565格式：高5位红，中间6位绿，低5位蓝
    uint8_t r = random(0, 32); // 0-31
    uint8_t g = random(0, 64); // 0-63
    uint8_t b = random(0, 32); // 0-31
    return (r << 11) | (g << 5) | b;
}