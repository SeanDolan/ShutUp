#pragma once

#include <Arduino.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>

#include "shutup_settings.h"

namespace shutup {

class CabDisplay {
public:
  bool beginBlackout() {
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);
    pinMode(LCD_POWER_ON, OUTPUT);
    digitalWrite(LCD_POWER_ON, HIGH);
    if (!initPanel()) {
      return false;
    }
    fillScreen(kBlack);
    digitalWrite(LCD_BL, HIGH);
    return true;
  }

  void showConfigMode() {
    fillScreen(kBlack);
    drawCenteredText("SHUTUP", 48, 3, kWhite);
    drawCenteredText("CONFIG MODE", 88, 2, kWhite);
  }

  void showSplash() {
    fillScreen(kBlack);
    drawCenteredText("SHUTUP", 52, 3, kWhite);
    drawCenteredText("STARTING", 94, 2, kWhite);
  }

  void showNormal(const DoorState states[kDoorCount], const SettingsStore &settings, bool linkFresh, uint8_t heartbeatPercent,
                  uint16_t averageHeartbeatMs) {
    fillScreen(kBlack);
    drawText(8, 8, "SHUTUP", 2, kWhite);
    drawText(194, 8, linkFresh ? "CONNECTED" : "NO LINK", 1, linkFresh ? kGreen : kRed);
    drawText(194, 24, "HEALTH", 1, kWhite);
    drawNumber(240, 24, heartbeatPercent, 1, heartbeatPercent >= 80 ? kGreen : kOrange);
    drawText(258, 24, "%", 1, heartbeatPercent >= 80 ? kGreen : kOrange);
    drawText(194, 38, "AVG", 1, kWhite);
    drawNumber(222, 38, averageHeartbeatMs, 1, kWhite);
    drawText(258, 38, "MS", 1, kWhite);

    for (uint8_t i = 0; i < kDoorCount; ++i) {
      const DoorOverlayConfig &overlay = settings.doorOverlay(i);
      const bool configured = overlay.width > 0 && overlay.height > 0;
      const int x = configured ? overlay.x : 18 + (i % 3) * 98;
      const int y = configured ? overlay.y : 62 + (i / 3) * 50;
      const int width = configured ? overlay.width : 78;
      const int height = configured ? overlay.height : 32;
      const bool enabled = states[i] != DoorState::Disabled;
      const bool doorOpen = states[i] == DoorState::Open;
      const uint16_t color = !enabled ? kDim : rgb565(doorOpen ? overlay.openColor : overlay.closedColor);
      fillRect(x, y, width, height, color);
      if (!configured || width >= 42) {
        drawText(x + 4, y + 4, !enabled ? "OFF" : (doorOpen ? "OPEN" : "OK"), 1, kBlack);
      }
    }
  }

private:
  static constexpr int kWidth = 320;
  static constexpr int kHeight = 170;
  static constexpr uint16_t kBlack = 0x0000;
  static constexpr uint16_t kWhite = 0xFFFF;
  static constexpr uint16_t kRed = 0xF800;
  static constexpr uint16_t kGreen = 0x07E0;
  static constexpr uint16_t kBlue = 0x001F;
  static constexpr uint16_t kOrange = 0xFD20;
  static constexpr uint16_t kDim = 0x39E7;

  static uint16_t rgb565(uint32_t color) {
    const uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
    const uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    const uint8_t b = static_cast<uint8_t>(color & 0xFF);
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  }

  struct LcdCommand {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
  };

  bool initPanel() {
    if (panel_) {
      return true;
    }

    pinMode(LCD_RD, OUTPUT);
    digitalWrite(LCD_RD, HIGH);

    esp_lcd_i80_bus_config_t busConfig = {
        .dc_gpio_num = LCD_DC,
        .wr_gpio_num = LCD_WR,
        .clk_src = LCD_CLK_SRC_PLL160M,
        .data_gpio_nums = {LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7},
        .bus_width = 8,
        .max_transfer_bytes = kWidth * 40 * sizeof(uint16_t),
        .psram_trans_align = 0,
        .sram_trans_align = 0,
    };
    if (esp_lcd_new_i80_bus(&busConfig, &bus_) != ESP_OK) {
      return false;
    }

    esp_lcd_panel_io_i80_config_t ioConfig = {
        .cs_gpio_num = LCD_CS,
        .pclk_hz = 16000000,
        .trans_queue_depth = 10,
        .on_color_trans_done = nullptr,
        .user_ctx = nullptr,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
    };
    if (esp_lcd_new_panel_io_i80(bus_, &ioConfig, &io_) != ESP_OK) {
      return false;
    }

    esp_lcd_panel_dev_config_t panelConfig = {
        .reset_gpio_num = LCD_RES,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
        .vendor_config = nullptr,
    };
    if (esp_lcd_new_panel_st7789(io_, &panelConfig, &panel_) != ESP_OK) {
      return false;
    }
    esp_lcd_panel_reset(panel_);
    esp_lcd_panel_init(panel_);
    esp_lcd_panel_invert_color(panel_, true);
    esp_lcd_panel_swap_xy(panel_, true);
    esp_lcd_panel_mirror(panel_, false, true);
    esp_lcd_panel_set_gap(panel_, 0, 35);

    static constexpr LcdCommand initCommands[] = {
        {0x11, {0}, 0x80},
        {0x3A, {0x05}, 1},
        {0xB2, {0x0B, 0x0B, 0x00, 0x33, 0x33}, 5},
        {0xB7, {0x75}, 1},
        {0xBB, {0x28}, 1},
        {0xC0, {0x2C}, 1},
        {0xC2, {0x01}, 1},
        {0xC3, {0x1F}, 1},
        {0xC6, {0x13}, 1},
        {0xD0, {0xA7}, 1},
        {0xD0, {0xA4, 0xA1}, 2},
        {0xD6, {0xA1}, 1},
        {0xE0, {0xF0, 0x05, 0x0A, 0x06, 0x06, 0x03, 0x2B, 0x32, 0x43, 0x36, 0x11, 0x10, 0x2B, 0x32}, 14},
        {0xE1, {0xF0, 0x08, 0x0C, 0x0B, 0x09, 0x24, 0x2B, 0x22, 0x43, 0x38, 0x15, 0x16, 0x2F, 0x37}, 14},
    };
    for (const auto &command : initCommands) {
      esp_lcd_panel_io_tx_param(io_, command.cmd, command.data, command.len & 0x7F);
      if ((command.len & 0x80) != 0) {
        delay(120);
      }
    }
    esp_lcd_panel_disp_on_off(panel_, true);
    return true;
  }

  void fillScreen(uint16_t color) {
    fillRect(0, 0, kWidth, kHeight, color);
  }

  void fillRect(int x, int y, int w, int h, uint16_t color) {
    if (!panel_ || w <= 0 || h <= 0) {
      return;
    }
    x = constrain(x, 0, kWidth);
    y = constrain(y, 0, kHeight);
    w = min(w, kWidth - x);
    h = min(h, kHeight - y);
    for (int i = 0; i < w; ++i) {
      line_[i] = color;
    }
    for (int row = 0; row < h; ++row) {
      esp_lcd_panel_draw_bitmap(panel_, x, y + row, x + w, y + row + 1, line_);
    }
  }

  void drawCenteredText(const char *text, int y, int scale, uint16_t color) {
    const int width = textWidth(text, scale);
    drawText((kWidth - width) / 2, y, text, scale, color);
  }

  void drawNumber(int x, int y, uint16_t value, int scale, uint16_t color) {
    char text[8];
    snprintf(text, sizeof(text), "%u", value);
    drawText(x, y, text, scale, color);
  }

  void drawText(int x, int y, const char *text, int scale, uint16_t color) {
    int cursor = x;
    for (const char *p = text; *p; ++p) {
      drawChar(cursor, y, *p, scale, color);
      cursor += 6 * scale;
    }
  }

  int textWidth(const char *text, int scale) const {
    return strlen(text) * 6 * scale;
  }

  void drawChar(int x, int y, char c, int scale, uint16_t color) {
    if (c >= 'a' && c <= 'z') {
      c = static_cast<char>(c - 'a' + 'A');
    }
    const uint8_t *glyph = glyphFor(c);
    for (uint8_t col = 0; col < 5; ++col) {
      uint8_t bits = glyph[col];
      for (uint8_t row = 0; row < 7; ++row) {
        if ((bits & (1U << row)) != 0) {
          fillRect(x + col * scale, y + row * scale, scale, scale, color);
        }
      }
    }
  }

  const uint8_t *glyphFor(char c) const {
    static constexpr uint8_t blank[5] = {0, 0, 0, 0, 0};
    static constexpr uint8_t glyphs[][5] = {
        {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
        {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
        {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
        {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E},
    };
    static constexpr uint8_t letters[][5] = {
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
        {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
        {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
        {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
        {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
        {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
        {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
        {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
        {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
        {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
        {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
    };
    static constexpr uint8_t percent[5] = {0x23, 0x13, 0x08, 0x64, 0x62};
    static constexpr uint8_t dash[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
    if (c >= '0' && c <= '9') {
      return glyphs[c - '0'];
    }
    if (c >= 'A' && c <= 'Z') {
      return letters[c - 'A'];
    }
    if (c == '%') {
      return percent;
    }
    if (c == '-') {
      return dash;
    }
    return blank;
  }

  esp_lcd_i80_bus_handle_t bus_{nullptr};
  esp_lcd_panel_io_handle_t io_{nullptr};
  esp_lcd_panel_handle_t panel_{nullptr};
  uint16_t line_[kWidth]{};
};

}  // namespace shutup
