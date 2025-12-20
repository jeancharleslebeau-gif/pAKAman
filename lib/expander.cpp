#include "driver/i2c_master.h"
#include "common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "TAS2505_rehs.h"
#include "expander.h"

static void delay(uint32_t u32_ms)
{
    vTaskDelay(u32_ms / portTICK_PERIOD_MS);
}

#define I2C_PORT_NUM_0 ((i2c_port_num_t)0)

i2c_master_bus_config_t i2c_mst_config;
i2c_master_bus_handle_t bus_handle;

i2c_device_config_t dev_cfg0;
i2c_master_dev_handle_t dev_handle0;

i2c_device_config_t dev_cfg1;
i2c_master_dev_handle_t dev_handle1;

i2c_device_config_t dev_cfg_audio;
i2c_master_dev_handle_t dev_handle_audio;

static uint8_t u8_expander_out_data = 0;

static void i2c_init_configs()
{
    // Bus config
    i2c_mst_config = {};
    i2c_mst_config.i2c_port = I2C_NUM_0;
    i2c_mst_config.sda_io_num = EXPANDER_I2C_SDA;
    i2c_mst_config.scl_io_num = EXPANDER_I2C_SCL;
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.intr_priority = 0;
    i2c_mst_config.trans_queue_depth = 0;
    i2c_mst_config.flags.enable_internal_pullup = true;
#if defined(I2C_MASTER_BUS_FLAG_ALLOW_PD)
    i2c_mst_config.flags.allow_pd = false;
#endif

    // Device 0
    dev_cfg0 = {};
    dev_cfg0.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg0.device_address = EXPANDER_I2C_ADDRESS0;
    dev_cfg0.scl_speed_hz = 400000;
    dev_cfg0.scl_wait_us = 0;
    dev_cfg0.flags = {};

    // Device 1
    dev_cfg1 = {};
    dev_cfg1.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg1.device_address = EXPANDER_I2C_ADDRESS1;
    dev_cfg1.scl_speed_hz = 400000;
    dev_cfg1.scl_wait_us = 0;
    dev_cfg1.flags = {};

    // Audio device
    dev_cfg_audio = {};
    dev_cfg_audio.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg_audio.device_address = AUDIO_AMP_I2C_ADDRESS;
    dev_cfg_audio.scl_speed_hz = 400000;
    dev_cfg_audio.scl_wait_us = 0;
    dev_cfg_audio.flags = {};
}

void expander_write(uint8_t u8_data)
{
    u8_expander_out_data = (u8_data | EXPANDER_KEY); // Key inputs must stay HIGH
    esp_err_t ret = i2c_master_transmit(dev_handle0, &u8_expander_out_data, sizeof(u8_expander_out_data), -1);
    if (ret) printf("I2C write return %d\n", ret);
}

void expander_lcd_reset(uint8_t state)
{
    if (state) expander_write(u8_expander_out_data | EXPANDER_LCD_nRESET);
    else       expander_write(u8_expander_out_data & ~EXPANDER_LCD_nRESET);
}

void expander_audio_amplifier_reset(uint8_t state)
{
    if (state) expander_write(u8_expander_out_data | EXPANDER_AMP_nRESET);
    else       expander_write(u8_expander_out_data & ~EXPANDER_AMP_nRESET);
}

#if (BOARD_VERSION < 4) // hard connected on V1.4+
void expander_lcd_cs(uint8_t state)
{
    if (state) expander_write(u8_expander_out_data | EXPANDER_LCD_nCS);
    else       expander_write(u8_expander_out_data & ~EXPANDER_LCD_nCS);
}
#else
void expander_lcd_rd(uint8_t state)
{
    if (state) expander_write(u8_expander_out_data | EXPANDER_LCD_nRD);
    else       expander_write(u8_expander_out_data & ~EXPANDER_LCD_nRD);
}
#endif

uint16_t expander_read()
{
    uint8_t u8_d1 = 0x55;
    esp_err_t ret1 = i2c_master_receive(dev_handle1, &u8_d1, sizeof(u8_d1), -1);
    uint8_t u8_d0 = 0x55;
    esp_err_t ret0 = i2c_master_receive(dev_handle0, &u8_d0, sizeof(u8_d0), -1);
    uint16_t u16_data = u8_d0 + 256 * (uint16_t)u8_d1;
    u16_data ^= EXPANDER_KEY_RUN; // Run key active high => active low
    u16_data ^= EXPANDER_KEY;     // all key active low => active high
    if (ret0 || ret1) {
        printf("I2C read return %d %d\n", ret0, ret1);
        return 0;
    }
    return u16_data;
}

void expander_power_off()
{
    while (expander_read() & EXPANDER_KEY_RUN) { /* wait user release button */ }

    expander_write(u8_expander_out_data &= ~EXPANDER_AMP_nRESET);
    expander_write(u8_expander_out_data |= EXPANDER_KEY_RUN);
    delay(250);
    expander_write(u8_expander_out_data &= ~EXPANDER_OUT_ENA_3V3);
}

// --- Contrôle ampli audio ---
// Écrit une valeur dans un registre de l’ampli TAS2505 via I2C
void audio_amp_write(uint8_t u8_reg, uint8_t u8_data)
{
    uint8_t u8_datareg[2] = { u8_reg, u8_data };
    esp_err_t ret = i2c_master_transmit(dev_handle_audio, u8_datareg, sizeof(u8_datareg), -1);
    if (ret) {
        printf("I2C Audio write return %d\n", ret);
    }
}

// Lit une valeur depuis un registre de l’ampli TAS2505
uint8_t audio_amp_read(uint8_t u8_reg)
{
    uint8_t u8_data = 0;
    esp_err_t ret = i2c_master_transmit_receive(dev_handle_audio,
                                                &u8_reg, sizeof(u8_reg),
                                                &u8_data, sizeof(u8_data), -1);
    if (ret) {
        printf("ERR : I2C Audio read return %d\n", ret);
        return 0;
    }
    printf("I2C Audio read reg 0x%02X return 0x%02X\n", u8_reg, u8_data);
    return u8_data;
}

// Réglage du volume global (0 = max, 255 = min/mute)
void audio_set_volume(uint8_t u8_volume)
{
    // Conversion volume → registre TAS2505
    u8_volume = 127 - u8_volume / 2; // 0 = volume max, 116 = volume min
    audio_amp_write(AUDIO_AMP_REG_PAGE, 1); // switch page 1

    if (u8_volume >= 116) { // mute
        audio_amp_write(AUDIO_AMP_P1_SPK_VOL, 0x7F);
        audio_amp_write(AUDIO_AMP_P1_HP_SPK_VOL, 0x7F);
    } else {
        audio_amp_write(AUDIO_AMP_P1_SPK_VOL, u8_volume);
        audio_amp_write(AUDIO_AMP_P1_HP_SPK_VOL, u8_volume);
    }
}

// Active/désactive le vibreur via GPIO de l’ampli
void audio_set_vibrator(uint8_t u8_on)
{
    audio_amp_write(AUDIO_AMP_REG_PAGE, 0); // Page 0
    if (u8_on) {
        audio_amp_write(AUDIO_AMP_P0_GPIO_CTRL, 0b00001101); // HIGH
    } else {
        audio_amp_write(AUDIO_AMP_P0_GPIO_CTRL, 0b00001100); // LOW
    }
}

// Routine de test expander : affiche les touches pressées
void test_expander()
{
    for (int i = 0; ; i++) {
        uint16_t u16_exp = expander_read();
        printf("%4d EXP = 0x%04X\n", i, u16_exp);

        if (u16_exp & EXPANDER_KEY_RUN) {
            printf("Set to power down\n");
            expander_write(0);
        }

        if (u16_exp & EXPANDER_KEY_RUN)   printf("KEY RUN\n");
        if (u16_exp & EXPANDER_KEY_MENU)  printf("KEY MENU\n");
        if (u16_exp & EXPANDER_KEY_L1)    printf("KEY L1\n");
        if (u16_exp & EXPANDER_KEY_R1)    printf("KEY R1\n");
        if (u16_exp & EXPANDER_KEY_UP)    printf("KEY UP\n");
        if (u16_exp & EXPANDER_KEY_DOWN)  printf("KEY DOWN\n");
        if (u16_exp & EXPANDER_KEY_LEFT)  printf("KEY LEFT\n");
        if (u16_exp & EXPANDER_KEY_RIGHT) printf("KEY RIGHT\n");
        if (u16_exp & EXPANDER_KEY_A)     printf("KEY A\n");
        if (u16_exp & EXPANDER_KEY_B)     printf("KEY B\n");
        if (u16_exp & EXPANDER_KEY_C)     printf("KEY C\n");
        if (u16_exp & EXPANDER_KEY_D)     printf("KEY D\n");

        delay(100);
    }
}


// ---------------- PWM ----------------
#include "driver/ledc.h"

#define LEDC_TIMER     LEDC_TIMER_0
#define LEDC_MODE      LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (PWM_LCD_GPIO)
#define LEDC_CHANNEL   LEDC_CHANNEL_0
#define LEDC_DUTY_RES  LEDC_TIMER_8_BIT
#define LEDC_DUTY      (0)
#define LEDC_FREQUENCY (PWM_LCD_FREQUENCY)

void lcd_init_pwm()
{
    // Timer config (zero-init then set fields to avoid designated-order issues)
    ledc_timer_config_t ledc_timer{};
    ledc_timer.speed_mode = LEDC_MODE;
    ledc_timer.duty_resolution = LEDC_DUTY_RES;
    ledc_timer.timer_num = LEDC_TIMER;
    ledc_timer.freq_hz = LEDC_FREQUENCY;
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
#if defined(SOC_LEDC_SUPPORT_DECONFIG)
    ledc_timer.deconfigure = false;
#endif
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Channel config (assign fields in declaration order)
    ledc_channel_config_t ledc_channel{};
    // Try to respect common order: speed_mode, channel, gpio_num, intr_type, timer_sel, duty, hpoint, flags, sleep_mode
    ledc_channel.speed_mode = LEDC_MODE;
    ledc_channel.channel = LEDC_CHANNEL;
    ledc_channel.gpio_num = LEDC_OUTPUT_IO;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.timer_sel = LEDC_TIMER;
    ledc_channel.duty = 0;
    ledc_channel.hpoint = 0;
    ledc_channel.flags = {};
#if defined(LEDC_SLEEP_ENABLE) || defined(LEDC_SLEEP_DISABLE)
    // Some targets expose sleep_mode; set disable if available
    ledc_channel.sleep_mode = LEDC_SLEEP_DISABLE;
#endif
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void lcd_update_pwm(uint8_t u8_duty)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, u8_duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

// ---------------- ADC ----------------
#include "esp_adc/adc_oneshot.h"

adc_oneshot_unit_handle_t adc1_handle;

const adc_oneshot_chan_cfg_t config = {
    .atten = ADC_ATTEN_DB_12,     // plage complète 0..3.3V
    .bitwidth = ADC_BITWIDTH_12,  // résolution 12 bits (0..4095)
};

int adc_init()
{
    adc_oneshot_unit_init_cfg_t init_config1{};
    init_config1.unit_id = ADC_UNIT_1;
#if defined(ADC_CLK_SRC_DEFAULT)
    init_config1.clk_src = ADC_CLK_SRC_DEFAULT;
#endif
#if defined(ADC_ULP_MODE_DISABLE)
    init_config1.ulp_mode = ADC_ULP_MODE_DISABLE;
#endif

    esp_err_t ret = adc_oneshot_new_unit(&init_config1, &adc1_handle);
    printf("adc_oneshot_new_unit return %d\n", ret);

    // Configure les 3 canaux (batterie + joystick X/Y)
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, (adc_channel_t)ADC1_CHANNEL_BATTERY, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, (adc_channel_t)ADC1_CHANNEL_JOYX, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, (adc_channel_t)ADC1_CHANNEL_JOYY, &config));

    return 0;
}

int adc_read_vbatt()
{
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, (adc_channel_t)ADC1_CHANNEL_BATTERY, &adc_raw));
    // Conversion brute en mV (0..3300)
    int voltage = (adc_raw * 3300) / 4095;
    return 2 * voltage; // batterie divisée par 2 → on multiplie pour retrouver la tension réelle
}

int adc_read_joyx()
{
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, (adc_channel_t)ADC1_CHANNEL_JOYX, &adc_raw));
    // Conversion brute en mV (0..3300)
    int voltage = (adc_raw * JOYX_MAX) / 4095; // JOYX_MAX = 3300
    return voltage;
}

int adc_read_joyy()
{
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, (adc_channel_t)ADC1_CHANNEL_JOYY, &adc_raw));
    int voltage = (adc_raw * JOYX_MAX) / 4095;
    return voltage;
}


int expander_init()
{
    i2c_init_configs();

    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    printf("Return %d\n", ret);

    esp_err_t ret0 = i2c_master_bus_add_device(bus_handle, &dev_cfg0, &dev_handle0);
    printf("Return %d\n", ret0);

    esp_err_t ret1 = i2c_master_bus_add_device(bus_handle, &dev_cfg1, &dev_handle1);
    printf("Return %d\n", ret1);

    esp_err_t ret2 = i2c_master_bus_add_device(bus_handle, &dev_cfg_audio, &dev_handle_audio);
    printf("Return %d\n", ret2);

#if (BOARD_VERSION < 4)
    expander_write(EXPANDER_OUT_ENA_3V3 | EXPANDER_AMP_nRESET | EXPANDER_LCD_nRESET | EXPANDER_LCD_nCS);
#else
    expander_write(EXPANDER_OUT_ENA_3V3 | EXPANDER_AMP_nRESET | EXPANDER_LCD_nRESET | EXPANDER_LCD_nRD);
#endif
    expander_write(EXPANDER_OUT_ENA_3V3);
    delay(100);
    expander_write(EXPANDER_OUT_ENA_3V3 | EXPANDER_LCD_nRESET);
    delay(100);
    printf("Done\n");

    if (ret | ret0 | ret1) return -1;
    return 0;
}
