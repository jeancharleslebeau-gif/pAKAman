#pragma once
#include <stdint.h>
#include "driver/gpio.h"

#define BOARD_VERSION 4 // CORE V1.4

//#define USE_WIFI
//#define USE_PSRAM_VIDEO_BUFFER  // alloc 320x240x16 video buffer in PSRAM ( instead or static alloc in DRAM )
#define DOUBLE_BUFFER_8B  // use 2nd buffer 320x240x8 for anarch in DRAM 

#define ZERO_BUFFER_PSRAM   // 1 SRAM video buffer render directly from SRAM ( no double buffer )
//#define ONCE_BUFFER_PSRAM   // 1 PSRAM + 1 SRAM video buffer : copy SRAM to PSRAM buffer
//#define DUAL_BUFFER_PSRAM // 2 PSRAM video buffer, swap video pointers

//#define USE_VSYCNC // sync to screen refresh : 70 / 35 .. fps or free FPS

// setting PWM properties
#define PWM_LCD_GPIO            45
#define PWM_LCD_FREQUENCY       50000 // must be > 20000 to avoid being audible
#define PWM_LCD_LED_CHANNEL     0
#define PWM_LCD_RESOLUTION      8

// PCB v1.3 #define CONFIG_EXAMPLE_PIN_CMD 36
#define CONFIG_EXAMPLE_PIN_CMD 38
#define CONFIG_EXAMPLE_PIN_CLK 2
// PCB v1.3 #define CONFIG_EXAMPLE_PIN_D0 35
#define CONFIG_EXAMPLE_PIN_D0 1
#define CONFIG_EXAMPLE_PIN_D1 15
#define CONFIG_EXAMPLE_PIN_D2 7
#define CONFIG_EXAMPLE_PIN_D3 16

//#define LCD_H_RESOLUTION  320
//#define LCD_V_RESOLUTION  240

#define LCD_H_RESOLUTION  240
#define LCD_V_RESOLUTION  320

#if (BOARD_VERSION < 4) // on I2C expander on V1.4+
    static const gpio_num_t LCD_PIN_nRD = GPIO_NUM_47;
#else
    static const gpio_num_t LCD_FMARK   = GPIO_NUM_47;
#endif

static const gpio_num_t LCD_PIN_nWR  = GPIO_NUM_21;
static const gpio_num_t LCD_PIN_DnC  = GPIO_NUM_48;
static const gpio_num_t LCD_PIN_nRST = GPIO_NUM_42; // connected to expander, mapped to CPU_MTMS => change this, use expander

// Data bus
static const gpio_num_t LCD_PIN_DB7 = GPIO_NUM_14;
static const gpio_num_t LCD_PIN_DB6 = GPIO_NUM_13;
static const gpio_num_t LCD_PIN_DB5 = GPIO_NUM_12;
static const gpio_num_t LCD_PIN_DB4 = GPIO_NUM_11;
static const gpio_num_t LCD_PIN_DB3 = GPIO_NUM_10;
static const gpio_num_t LCD_PIN_DB2 = GPIO_NUM_9;
static const gpio_num_t LCD_PIN_DB1 = GPIO_NUM_46;
static const gpio_num_t LCD_PIN_DB0 = GPIO_NUM_3;

// I2S audio
#define I2S_PIN_BCLK  GPIO_NUM_18
#define I2S_PIN_WS    GPIO_NUM_17
#define I2S_PIN_DOUT  GPIO_NUM_8

#define NEOPIXELS_COUNT 4
#define NEOPIXELS_GPIO  44

// PCB V1.3
//#define EXPANDER_I2C_SCL   ((gpio_num_t)38)  // GPIO 38
//#define EXPANDER_I2C_SDA   ((gpio_num_t)37)  // GPIO 37
// PCB V1.4
#define EXPANDER_I2C_SCL   ((gpio_num_t)43)  // GPIO 43, shared with MCU_TX
#define EXPANDER_I2C_SDA   ((gpio_num_t)0)   // GPIO 0, shared with MCU_BOOT

#define EXPANDER_I2C_ADDRESS0   0x38 // 7 bits address
#define EXPANDER_I2C_ADDRESS1   0x3F // 7 bits address
#define AUDIO_AMP_I2C_ADDRESS   0x18 // 7 bits address

// EXP0 : LOW byte
#define EXPANDER_OUT_ENA_3V3  0x0001
#define EXPANDER_KEY_RUN      0x0002
#define EXPANDER_KEY_MENU     0x0004
#define EXPANDER_AMP_nRESET   0x0008
#define EXPANDER_LCD_nRESET   0x0010
#if (BOARD_VERSION < 4) // hard connected on V1.4+
    #define EXPANDER_LCD_nCS      0x0020
#else
    #define EXPANDER_LCD_nRD      0x0020
#endif
#define EXPANDER_KEY_L1       0x0080
#define EXPANDER_KEY_R1       0x0040

// EXP1 : HIGH byte
#if (BOARD_VERSION < 4) // key map change on V1.4+  
    #define EXPANDER_KEY_UP       0x0100
    #define EXPANDER_KEY_DOWN     0x0200
    #define EXPANDER_KEY_LEFT     0x0400
    #define EXPANDER_KEY_RIGHT    0x0800
    #define EXPANDER_KEY_A        0x1000
    #define EXPANDER_KEY_B        0x2000
    #define EXPANDER_KEY_C        0x4000
    #define EXPANDER_KEY_D        0x8000
#else    
    #define EXPANDER_KEY_RIGHT    0x0100
    #define EXPANDER_KEY_UP       0x0200
    #define EXPANDER_KEY_DOWN     0x0400
    #define EXPANDER_KEY_LEFT     0x0800
    #define EXPANDER_KEY_A        0x8000
    #define EXPANDER_KEY_B        0x2000
    #define EXPANDER_KEY_C        0x4000
    #define EXPANDER_KEY_D        0x1000
#endif

#define EXPANDER_KEY     ( EXPANDER_KEY_RUN | EXPANDER_KEY_MENU | EXPANDER_KEY_L1 | EXPANDER_KEY_R1 | EXPANDER_KEY_UP | EXPANDER_KEY_DOWN | EXPANDER_KEY_LEFT | EXPANDER_KEY_RIGHT | EXPANDER_KEY_A | EXPANDER_KEY_B | EXPANDER_KEY_C | EXPANDER_KEY_D )

#define ADC1_CHANNEL_BATTERY  3 // ADC1, channel 3
#if (BOARD_VERSION < 4) // hard connected on V1.4+
    #define ADC1_CHANNEL_JOYX     4
    #define ADC1_CHANNEL_JOYY     5
#else
    #define ADC1_CHANNEL_JOYX     5
    #define ADC1_CHANNEL_JOYY     4
#endif

#define JOYX_MAX    3300            // 3.300 V max
#define JOYX_LOW    (1*JOYX_MAX/4)  // 25% max : low level as DPAD
#define JOYX_MID    (2*JOYX_MAX/4)  // 50% max
#define JOYX_HIGH   (3*JOYX_MAX/4)  // 75% max : high level as DPAD

#define count_of(A) (sizeof(A)/sizeof(A[0]))

#define USE_UART_BUTTONS