// ============================================================================
//  LCD.cpp — Pilote ST7789 (bus i80 + DMA) pour écran 320×240 RGB565
// ============================================================================
//
//  Rôle :
//    - Initialiser le bus i80 et le contrôleur LCD ST7789
//    - Gérer le framebuffer 320×240 (RGB565)
//    - Fournir des primitives bas niveau : pixels, texte, scrolling
//    - Gérer un pipeline DMA propre : lcd_wait_for_dma / lcd_wait_for_vsync
//      / lcd_start_dma / lcd_refresh
//
//  Ce fichier ne fait aucun rendu haut niveau : il est utilisé par les
//  backends gfx_fb / gfx_direct et par la façade graphics.cpp.
// ============================================================================

#include "LCD.h"
#include "expander.h"
#include "core/graphics.h"

#include <string.h>     // memcpy, memset
#include <stdio.h>      // printf
#include <stdarg.h>     // va_list, vsnprintf

#include <esp_lcd_panel_io.h>
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_ops.h"

#include "hal/gpio_hal.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "hal/lcd_types.h"

// ============================================================================
//  Framebuffer
// ============================================================================

uint16_t framebuffer[320 * 240];


// ============================================================================
//  Helpers système
// ============================================================================

static void delay_ms(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

IRAM_ATTR uint32_t millis()
{
    return xTaskGetTickCount() * 1000 / portTICK_PERIOD_MS;
}


// ============================================================================
//  État du pipeline DMA
// ============================================================================

volatile uint32_t u32_start_refresh = 0;
volatile uint32_t u32_delta_refresh = 0;
volatile uint32_t u32_draw_count    = 0;
volatile uint32_t u32_refresh_ctr = 0;


// ============================================================================
//  Bus i80 + IO ST7789
// ============================================================================

// Clock source, bus config, panel config
esp_lcd_i80_bus_handle_t i80_bus = nullptr;
esp_lcd_panel_io_handle_t lcd_panel_h = nullptr;

// Bus configuration
static esp_lcd_i80_bus_config_t bus_config = {
    .dc_gpio_num = LCD_PIN_DnC,
    .wr_gpio_num = LCD_PIN_nWR,
    .clk_src     = LCD_CLK_SRC_PLL160M,
    .data_gpio_nums = {
        LCD_PIN_DB0,
        LCD_PIN_DB1,
        LCD_PIN_DB2,
        LCD_PIN_DB3,
        LCD_PIN_DB4,
        LCD_PIN_DB5,
        LCD_PIN_DB6,
        LCD_PIN_DB7,
    },
    .bus_width         = 8,
    .max_transfer_bytes= 320 * 240 * sizeof(uint16_t),
    .psram_trans_align = 64,
    .sram_trans_align  = 64,
};

// Panel IO config (DMA)
esp_lcd_panel_io_i80_config_t panel_config = {
    .cs_gpio_num = -1,
    .pclk_hz     = 20000000,         // ~8 ms / 125 fps, validé pour tes dalles
    .trans_queue_depth = 320 * 16,   // profondeur de queue DMA
    .on_color_trans_done = nullptr,  // sera assigné plus bas
    .user_ctx    = nullptr,
    .lcd_cmd_bits   = 8,
    .lcd_param_bits = 8,
    .dc_levels = {
        .dc_idle_level   = 1,
        .dc_cmd_level    = 0,
        .dc_dummy_level  = 1,
        .dc_data_level   = 1
    },
    .flags = {
        .cs_active_high   = 0,
        .reverse_color_bits = 0,
        .swap_color_bytes = 1,
        .pclk_active_neg  = 0,
        .pclk_idle_low    = 0
    }
};


// ============================================================================
//  Callback DMA — fin de transfert
// ============================================================================

IRAM_ATTR bool color_trans_done_cb(esp_lcd_panel_io_handle_t,
                                   esp_lcd_panel_io_event_data_t*,
                                   void*)
{
    uint32_t now = millis();
    if (u32_start_refresh != 0) {
        u32_delta_refresh = now - u32_start_refresh;
    } else {
        u32_delta_refresh = 0;
    }
    u32_refresh_ctr++;   // ← maintenant ça compile
    u32_start_refresh = 0;
    return false;
}


// ============================================================================
//  Gestion bus i80
// ============================================================================

static void LCD_i80_bus_init()
{
    printf("esp_lcd_new_i80_bus\n");
    esp_err_t ret = esp_lcd_new_i80_bus(&bus_config, &i80_bus);
    printf("esp_lcd_new_i80_bus return %d\n", ret);
    if (ret != ESP_OK) {
        for (;;) delay_ms(1000);
    }

    printf("esp_lcd_new_panel_io_i80\n");
    panel_config.on_color_trans_done = color_trans_done_cb;
    ret = esp_lcd_new_panel_io_i80(i80_bus, &panel_config, &lcd_panel_h);
    printf("esp_lcd_new_panel_io_i80 return %d\n", ret);
    if (ret != ESP_OK) {
        for (;;) delay_ms(1000);
    }
}

static void LCD_i80_bus_uninit()
{
    if (lcd_panel_h) {
        esp_lcd_panel_io_del(lcd_panel_h);
        lcd_panel_h = nullptr;
    }
    if (i80_bus) {
        esp_lcd_del_i80_bus(i80_bus);
        i80_bus = nullptr;
    }
}


// ============================================================================
//  IO GPIO haut niveau
// ============================================================================

#define LOW  0
#define HIGH 1

#define INPUT  1
#define OUTPUT 0

static void pinMode(gpio_num_t pin, uint8_t dir)
{
    gpio_config_t io_conf{};
    io_conf.pin_bit_mask   = 1ULL << pin;
    io_conf.mode           = (dir == OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    io_conf.pull_up_en     = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en   = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type      = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

static void digitalWrite(gpio_num_t pin, uint8_t level)
{
    gpio_ll_set_level(&GPIO, pin, level);
}

static int digitalRead(gpio_num_t pin)
{
    return gpio_get_level(pin);
}

static void delayMicroseconds(uint32_t us)
{
    uint64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < us) {
        // busy-wait court, ok pour micro-délai
    }
}


// ============================================================================
//  Bus parallèle "soft" (lecture/écriture registre ST7789, scanline…)
// ============================================================================

static const gpio_num_t u8_data_bus[8] = {
    LCD_PIN_DB0, LCD_PIN_DB1, LCD_PIN_DB2, LCD_PIN_DB3,
    LCD_PIN_DB4, LCD_PIN_DB5, LCD_PIN_DB6, LCD_PIN_DB7
};

static void lcd_set_bus_dir(uint8_t mode)
{
    for (int i = 0; i < 8; ++i)
        pinMode(u8_data_bus[i], mode);
}

static void lcd_write_bus_data(uint8_t data)
{
    digitalWrite(LCD_PIN_nWR, HIGH);
    for (int i = 0; i < 8; ++i) {
        digitalWrite(u8_data_bus[i], data & 1);
        data >>= 1;
    }
    digitalWrite(LCD_PIN_nWR, LOW);
    digitalWrite(LCD_PIN_nWR, HIGH);
}

static uint8_t lcd_read_bus_data()
{
    uint8_t data = 0;

#if (BOARD_VERSION < 4)
    digitalWrite(LCD_PIN_nRD, HIGH);
    digitalWrite(LCD_PIN_nRD, LOW);
#else
    expander_lcd_rd(1);
    expander_lcd_rd(0);
#endif

    lcd_set_bus_dir(INPUT);
    delayMicroseconds(10);

    for (int i = 0; i < 8; ++i) {
        data <<= 1;
        data |= digitalRead(u8_data_bus[7 - i]) ? 1 : 0;
    }

#if (BOARD_VERSION < 4)
    digitalWrite(LCD_PIN_nRD, HIGH);
#else
    expander_lcd_rd(1);
#endif

    delayMicroseconds(10);
    return data;
}

static void ILI9342C_write_cmd_soft(uint8_t cmd, const uint8_t* data, int len)
{
    digitalWrite(LCD_PIN_DnC, LOW);
    lcd_write_bus_data(cmd);
    digitalWrite(LCD_PIN_DnC, HIGH);
    while (len--) {
        lcd_write_bus_data(*data++);
    }
}

static void ILI9342C_write_cmd(uint8_t cmd, const uint8_t* data, int len)
{
    esp_lcd_panel_io_tx_param(lcd_panel_h, cmd, data, len);
}


// ============================================================================
//  Initialisation des IO LCD
// ============================================================================

static void lcd_init_io()
{
#if (BOARD_VERSION < 4)
    pinMode(LCD_PIN_nRD, OUTPUT);
#else
    pinMode(LCD_FMARK, INPUT);
#endif

    pinMode(LCD_PIN_nWR,  OUTPUT);
    pinMode(LCD_PIN_DnC,  OUTPUT);
    pinMode(LCD_PIN_nRST, OUTPUT);

#if (BOARD_VERSION < 4)
    digitalWrite(LCD_PIN_nRD, HIGH);
#else
    expander_lcd_rd(1);
#endif

    digitalWrite(LCD_PIN_nWR, HIGH);
    digitalWrite(LCD_PIN_DnC, HIGH);
    digitalWrite(LCD_PIN_nRST, HIGH);

    lcd_set_bus_dir(OUTPUT);
}


// ============================================================================
//  Reset matériel + rotation + fenêtre d’affichage
// ============================================================================

static void ILI9342C_hard_reset()
{
    expander_lcd_reset(0);
    delay_ms(10);
    expander_lcd_reset(1);
    delay_ms(100);
#if (BOARD_VERSION < 4)
    expander_lcd_cs(0);
#endif
    delay_ms(100);
}

static void st7789v_rotation_set(uint8_t rotation)
{
    uint8_t mad_cfg = 0;
    switch (rotation % 4) {
    case 0:
        mad_cfg = ST7789V_MADCTRL_MX | ST7789V_MADCTRL_MY | ST7789V_MADCTRL_RGB;
        break;
    case 1:
        mad_cfg = ST7789V_MADCTRL_MV | ST7789V_MADCTRL_RGB;
        break;
    case 2:
        mad_cfg = ST7789V_MADCTRL_RGB;
        break;
    case 3:
        mad_cfg = ST7789V_MADCTRL_MX | ST7789V_MADCTRL_MV | ST7789V_MADCTRL_RGB;
        break;
    default:
        break;
    }
    ILI9342C_write_cmd(ST7789V_CMD_MADCTL, &mad_cfg, 1);
}

static void set_addr_window(uint16_t x0, uint16_t y0,
                            uint16_t x1, uint16_t y1)
{
    uint8_t x_coord[4], y_coord[4];
    x_coord[0] = x0 >> 8;
    x_coord[1] = (uint8_t)x0;
    x_coord[2] = x1 >> 8;
    x_coord[3] = (uint8_t)x1;

    y_coord[0] = y0 >> 8;
    y_coord[1] = (uint8_t)y0;
    y_coord[2] = y1 >> 8;
    y_coord[3] = (uint8_t)y1;

    ILI9342C_write_cmd(ST7789V_CMD_CASET, x_coord, 4);
    ILI9342C_write_cmd(ST7789V_CMD_RASET, y_coord, 4);
}


// ============================================================================
//  Tests lents (mode "bit banging") — conservés pour debug éventuel
// ============================================================================

static void LCD_SLOW_test(const uint16_t* buffer)
{
    digitalWrite(LCD_PIN_DnC, LOW);
    lcd_write_bus_data(ST7789V_CMD_RAMWR);
    digitalWrite(LCD_PIN_DnC, HIGH);

    for (int i = 0; i < 320 * 240; ++i) {
        uint16_t pix = buffer[i];
        lcd_write_bus_data(pix >> 8);
        lcd_write_bus_data(pix & 0xFF);
    }
}


// ============================================================================
//  Transfert DMA rapide
// ============================================================================

void LCD_FAST_test(const uint16_t* buf)
{
    u32_start_refresh = millis();
    esp_lcd_panel_io_tx_color(lcd_panel_h, ST7789V_CMD_RAMWR, buf,
                              320 * 240 * sizeof(uint16_t));
}

uint32_t LCD_last_refresh_delay()
{
    return u32_delta_refresh;
}


// ============================================================================
//  Scanline / tearing
// ============================================================================

static uint8_t lcd_read_scanline()
{
    uint8_t ret = 0;

    lcd_set_bus_dir(OUTPUT);
    ILI9342C_write_cmd(ST7789V_CMD_GET_SCANLINE, nullptr, 0);
    lcd_set_bus_dir(INPUT);

    (void)lcd_read_bus_data(); // dummy
    (void)lcd_read_bus_data(); // MSB
    ret = lcd_read_bus_data(); // LSB

    return ret;
}


// ============================================================================
//  Pipeline DMA propre
// ============================================================================

void lcd_wait_for_dma()
{
    static uint32_t last_seen = 0;

    while (u32_refresh_ctr == last_seen) {
        taskYIELD();
    }

    last_seen = u32_refresh_ctr;
}


void lcd_wait_for_vsync()
{
#ifdef USE_VSYCNC
    uint64_t start_us = esp_timer_get_time();

    // Attendre front descendant
    while (digitalRead(LCD_FMARK)) {
        if ((esp_timer_get_time() - start_us) > 20000) {
            printf("ERROR : Loop scanline 1 timeout\n");
            return;
        }
    }

    // Attendre front montant
    start_us = esp_timer_get_time();
    while (!digitalRead(LCD_FMARK)) {
        if ((esp_timer_get_time() - start_us) > 20000) {
            printf("ERROR : Loop scanline 0 timeout\n");
            return;
        }
    }
#endif
}

void lcd_start_dma()
{
    u32_draw_count = u32_draw_count + 1;
    LCD_FAST_test(framebuffer);
}

uint8_t lcd_refresh_completed()
{
    return (uint8_t)u32_refresh_ctr;
}

void lcd_refresh()
{
    lcd_wait_for_dma();
    lcd_wait_for_vsync();
    lcd_start_dma();
}


// ============================================================================
//  Couleurs, palette, FPS
// ============================================================================

void lcd_clear(uint16_t color)
{
    for (int i = 0; i < 320 * 240; ++i)
        framebuffer[i] = color;
}

void lcd_set_fps(uint8_t fps)
{
    if (fps > 100) fps = 100;
    if (fps < 40)  fps = 40;

    uint32_t div = 10000000 / (336 * fps);
    if (div > 250) div -= 250;
    else           div = 0;

    uint8_t cfg = div / 16;
    if (cfg > 31) cfg = 31;
    if (cfg < 1)  cfg = 1;

    printf("Set FPS to %d : reg 0x%02X\n", fps, cfg);
    ILI9342C_write_cmd(0xC6, &cfg, 1);
}

bool lcd_is_rgb565()
{
    return panel_config.flags.swap_color_bytes;
}


// ============================================================================
//  Initialisation complète du LCD
// ============================================================================

void LCD_init()
{
    lcd_init_io();
    esp_timer_early_init();

    printf("LCD_i80_bus_init()\n");
    LCD_i80_bus_init();

    ILI9342C_hard_reset();

    ILI9342C_write_cmd(ST7789V_CMD_RESET, nullptr, 0);
    ILI9342C_write_cmd(ST7789V_CMD_SLPOUT, nullptr, 0);

    const uint8_t color_mode[] = { COLOR_MODE_65K | COLOR_MODE_16BIT };
    ILI9342C_write_cmd(ST7789V_CMD_COLMOD, color_mode, 1);

    ILI9342C_write_cmd(ST7789V_CMD_INVON,  nullptr, 0);
    ILI9342C_write_cmd(ST7789V_CMD_NORON,  nullptr, 0);

    st7789v_rotation_set(3);
    set_addr_window(0, 0, 319, 239);

    ILI9342C_write_cmd(ST7789V_CMD_DISPON, nullptr, 0);

    const uint8_t te_cfg[] = { 0x00 }; // V-blank only
    ILI9342C_write_cmd(0x35, te_cfg, 1);

    // Framebuffer initial : noir
    lcd_clear(color_black);

    // Prépare le pipeline DMA : on considère qu’un premier refresh est "fait"
    u32_refresh_ctr = 1;

    // Petite intro : logo Gamebuino qui scroll (facultatif)
    extern const uint8_t gamebuinoLogo[]; // défini plus bas
    lcd_set_fps(100);

    for (int16_t y = -10; y < (240 - 10) / 2; y += 4) {
        lcd_clear(color_black);
        // logo centré
        int16_t x = (320 - 80) / 2;
        // dessine le logo en bitmap 1-bit
        // (fonction définie plus bas)
        extern void lcd_drawBitmap(int16_t x, int16_t y,
                                   const uint8_t* bitmap,
                                   uint16_t color);
        lcd_drawBitmap(x, y, gamebuinoLogo, color_orange);
        lcd_refresh();
        while (!lcd_refresh_completed()) {
            taskYIELD();
        }
    }

    lcd_set_fps(40); // FPS par défaut pour le jeu
}


// ============================================================================
//  Texte (font8x8_basic)
// ============================================================================

#include "assets/font8x8_basic.h"

// Couleur de texte globale (définie dans graphics.cpp)
extern uint16_t current_text_color;

void lcd_putpixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint32_t offset = x + y * 320;
    if (offset < 320 * 240) {
        framebuffer[offset] = color;
    }
}

void lcd_draw_char(uint16_t x, uint16_t y, char c)
{
    for (uint16_t dy = 0; dy < 8; ++dy) {
        uint8_t line = font8x8_basic[(uint8_t)c][dy];
        uint16_t color = current_text_color;

#ifdef LCD_COLOR_ORDER_RGB
        color = (color >> 8) | (color << 8);
#endif

        for (uint16_t dx = 0; dx < 8; ++dx) {
            if (line & 1) {
                lcd_putpixel(x + dx, y + dy, color);
            }
            line >>= 1;
        }
    }
}

void lcd_draw_char_bg(uint16_t x, uint16_t y, char c, uint16_t bgColor)
{
    for (uint16_t dy = 0; dy < 8; ++dy) {
        uint8_t line = font8x8_basic[(uint8_t)c][dy];

        uint16_t fg = current_text_color;
        uint16_t bg = bgColor;

#ifdef LCD_COLOR_ORDER_RGB
        fg = (fg >> 8) | (fg << 8);
        bg = (bg >> 8) | (bg << 8);
#endif

        for (uint16_t dx = 0; dx < 8; ++dx) {
            if (line & 1) {
                lcd_putpixel(x + dx, y + dy, fg);
            } else {
                lcd_putpixel(x + dx, y + dy, bg);
            }
            line >>= 1;
        }
    }
}

void lcd_draw_str(uint16_t x, uint16_t y, const char* text)
{
    while (*text) {
        lcd_draw_char(x, y, *text++);
        x += 8;
    }
}

void lcd_draw_str_bg(uint16_t x, uint16_t y, const char* text, uint16_t bgColor)
{
    while (*text) {
        lcd_draw_char_bg(x, y, *text++, bgColor);
        x += 8;
    }
}

void lcd_draw_text(uint16_t x, uint16_t y, const char* text)
{
    lcd_draw_str(x, y, text);
}


// ============================================================================
//  Affichage d’un petit bitmap 1-bit (logo Gamebuino)
// ============================================================================

const uint8_t gamebuinoLogo[] = {
    80, 10,
    // [données originales conservées, pas modifiées]
    0b00111100, 0b00111111, 0b00111111, 0b11110011, 0b11110011,
    0b11110011, 0b00110011, 0b00111111, 0b00111111, 0b00011100,
    0b00111100, 0b00111111, 0b00111111, 0b11110011, 0b11110011,
    0b11110011, 0b00110011, 0b00111111, 0b00111111, 0b00100110,
    0b00110000, 0b00110011, 0b00110011, 0b00110011, 0b00000011,
    0b00110011, 0b00110011, 0b00110011, 0b00110011, 0b00100110,
    0b00110000, 0b00110011, 0b00110011, 0b00110011, 0b00000011,
    0b00110011, 0b00110011, 0b00110011, 0b00110011, 0b00101010,
    0b00110011, 0b00111111, 0b00110011, 0b00110011, 0b11110011,
    0b11000011, 0b00110011, 0b00110011, 0b00110011, 0b00011100,
    0b00110011, 0b00111111, 0b00110011, 0b00110011, 0b11110011,
    0b11000011, 0b00110011, 0b00110011, 0b00110011, 0b00000000,
    0b00110011, 0b00110011, 0b00110011, 0b00110011, 0b00000011,
    0b00110011, 0b00110011, 0b00110011, 0b00110011, 0b00000000,
    0b00110011, 0b00110011, 0b00110011, 0b00110011, 0b00000011,
    0b00110011, 0b00110011, 0b00110011, 0b00110011, 0b00000000,
    0b00111111, 0b00110011, 0b00110011, 0b00110011, 0b11110011,
    0b11110011, 0b11110011, 0b00110011, 0b00111111, 0b00000000,
    0b00111111, 0b00110011, 0b00110011, 0b00110011, 0b11110011,
    0b11110011, 0b11110011, 0b00110011, 0b00111111, 0b00000000,
};

void lcd_drawBitmap(int16_t x, int16_t y,
                    const uint8_t* bitmap,
                    uint16_t color)
{
    uint8_t w = *(bitmap++);
    uint8_t h = *(bitmap++);

    uint8_t byteWidth = (w + 7) / 8;
    uint8_t x0 = x;
    uint8_t dw = 8 - (w % 8);

    for (uint8_t j = 0; j < h; ++j) {
        x = x0;
        for (uint8_t i = 0; i < byteWidth; ) {
            uint8_t b = *(bitmap++);
            ++i;
            for (uint8_t k = 0; k < 8; ++k) {
                if (i == byteWidth && k == dw) {
                    x += (w % 8);
                    break;
                }
                if (b & 0x80) {
                    lcd_putpixel(x, y, color);
                }
                b <<= 1;
                ++x;
            }
        }
        ++y;
    }
}


// ============================================================================
//  Console texte (printf sur framebuffer)
// ============================================================================

static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;

void lcd_move_cursor(uint16_t x, uint16_t y)
{
    if (x < 320) cursor_x = x;
    if (y < 240) cursor_y = y;
}

void lcd_printf(const char* fmt, ...)
{
    va_list ap;
    static char buffer[256];

    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    char* p = buffer;
    while (*p) {
        switch (*p) {
        case '\r':
            cursor_x = 0;
            break;
        case '\n':
            cursor_x = 0;
            cursor_y += 8;
            break;
        default:
            lcd_draw_char(cursor_x, cursor_y, *p);
            cursor_x += 8;
            break;
        }

        if (cursor_x >= 320) {
            cursor_x = 0;
            cursor_y += 8;
        }

        if (cursor_y >= 240) {
            // scroll de 8 pixels : décale le framebuffer
            memmove(framebuffer,
                    &framebuffer[320 * 8],
                    (320 * 240 - 320 * 8) * sizeof(uint16_t));
            memset(&framebuffer[320 * (240 - 8)],
                   0,
                   320 * 8 * sizeof(uint16_t));
            cursor_y -= 8;
        }
        ++p;
    }
}
