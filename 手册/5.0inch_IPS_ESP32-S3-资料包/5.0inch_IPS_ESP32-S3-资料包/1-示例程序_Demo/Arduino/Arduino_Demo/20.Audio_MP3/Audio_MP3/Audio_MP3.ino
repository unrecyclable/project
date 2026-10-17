#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Audio.h>
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>

// ====================== I2S 音频引脚 ======================
#define I2S_BCLK     0
#define I2S_LRCLK    18
#define I2S_SDATA    17

// ====================== SD 卡 SPI 引脚 ======================
#define SD_CS        10
#define SD_SCLK      12
#define SD_MISO      13
#define SD_MOSI      11

// ====================== RGB LCD 配置 ======================
#define TFT_BL       2
#define LCD_WIDTH    800
#define LCD_HEIGHT   480

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

// ====================== 播放配置 ======================
#define MAX_MP3_FILES 50
char mp3Playlist[MAX_MP3_FILES][64];
int playlistCount = 0;
int currentSongIndex = 0;
bool isPlaying = false;

#define SPI_CLOCK_SPEED 10000000
#define VOLUME_LEVEL 10

// ====================== 全局对象 ======================
Audio audio;

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

// ====================== 显示初始化 ======================
void initDisplay() {
    gfx->begin();
    gfx->fillScreen(RGB565_BLACK);
    gfx->setUTF8Print(true);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    gfx->setFont(u8g2_font_unifont_t_gb2312);
    gfx->setTextSize(2);
}

// ====================== 显示：正在播放歌曲 ======================
void showPlayingScreen(const char* fileName) {
    gfx->fillScreen(RGB565_BLACK);
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(40, 100);
    gfx->print("🎵 正在播放：");
    gfx->setCursor(40, 160);
    gfx->print(fileName);
}

// ====================== 显示：无音频文件 ======================
void showNoAudioScreen() {
    gfx->fillScreen(RGB565_BLACK);
    gfx->setTextColor(RGB565_YELLOW);
    gfx->setCursor(40, 150);
    gfx->print("⚠️ SD卡无音频文件");
    gfx->setCursor(40, 210);
    gfx->print("请放入MP3文件后重启");
}

// ====================== 显示：初始化中 ======================
void showInitScreen() {
    gfx->fillScreen(RGB565_BLACK);
    gfx->setTextColor(RGB565_CYAN);
    gfx->setCursor(40, 180);
    gfx->print("🔧 系统初始化中...");
}

// ====================== 扫描MP3 ======================
void scanMp3Files() {
    Serial.println("扫描 MP3...");
    playlistCount = 0;
    File root = SD.open("/");
    if (!root) return;

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            const char* name = file.name();
            int len = strlen(name);
            if (len > 4 && (
                strcasecmp(name + len - 4, ".mp3") == 0
            )) {
                if (playlistCount < MAX_MP3_FILES) {
                    strncpy(mp3Playlist[playlistCount], name, 63);
                    Serial.println(name);
                    playlistCount++;
                }
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

// ====================== 播放指定索引 ======================
bool playSongByIndex(int index) {
    if (index < 0 || index >= playlistCount) {
        isPlaying = false;
        return false;
    }

    audio.stopSong();
    delay(100);

    const char* path = mp3Playlist[index];
    showPlayingScreen(path);  // <--- 这里自动刷新屏幕显示歌名
    Serial.printf("播放：%s\n", path);

    audio.connecttoFS(SD, path);
    delay(500);

    if (!audio.isRunning()) {
        audio.connecttoFS(SD, path);
        delay(500);
        if (!audio.isRunning()) return false;
    }

    isPlaying = true;
    currentSongIndex = index;
    return true;
}

// ====================== 下一首 ======================
void playNextSong() {
    currentSongIndex++;
    if (currentSongIndex >= playlistCount) currentSongIndex = 0;
    if (!playSongByIndex(currentSongIndex)) {
        delay(1000);
        playNextSong();
    }
}

// ====================== SETUP ======================
void setup() {
    Serial.begin(115200);
    initDisplay();
    showInitScreen();
    delay(1000);

    // SD卡初始化
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, SPI, SPI_CLOCK_SPEED)) {
        Serial.println("SD卡失败");
        gfx->fillScreen(RGB565_BLACK);
        gfx->setTextColor(RGB565_RED);
        gfx->setCursor(40, 150);
        gfx->print("⚠️ 未检测到SD卡");
        gfx->setCursor(40, 210);
        gfx->print("请插入SD卡后重启");
        while (1);
    }

    scanMp3Files();

    // 音频初始化
    audio.setPinout(I2S_BCLK, I2S_LRCLK, I2S_SDATA);
    audio.setVolume(VOLUME_LEVEL);

    if (playlistCount > 0) {
        playSongByIndex(0);
    } else {
        showNoAudioScreen();  // <--- 无文件显示
        while (1);
    }
}

// ====================== LOOP ======================
void loop() {
    audio.loop();

    static unsigned long lastFeed = millis();
    if (isPlaying && audio.isRunning()) {
        lastFeed = millis();
    }

    // 播放卡死检测
    if (isPlaying && !audio.isRunning() && (millis() - lastFeed > 3000)) {
        isPlaying = false;
        delay(500);
        playNextSong();
    }

    // 播放完成自动切歌
    if (isPlaying && !audio.isRunning()) {
        isPlaying = false;
        delay(500);
        playNextSong();
    }
}