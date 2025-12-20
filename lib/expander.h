#pragma once
#ifndef __EXPANDER_H_
#define __EXPANDER_H_

#include "common.h"

//! Initialise 16 b expander
int expander_init();

//! write 8b low expander (high byte will be never written, keep all bits to 1 for correct key read)
void expander_write(uint8_t u8_data);

//! return 16b expander inputs (keys, cf EXPANDER_KEY_xxx)
uint16_t expander_read();

//! simple test routine for expander
void test_expander();

// --- PWM ---
void lcd_init_pwm();
void lcd_update_pwm(uint8_t u8_duty);

// --- ADC ---
// initialize 3 channels adc for battery and Joystick
int adc_init();

// return Battery voltage in mV
int adc_read_vbatt();

// return battery voltage as % : 0 ~ 100
int adc_read_vbatt_percent();

// return joystick X coordinate as mV (0 .. 3300)
int adc_read_joyx();

// return joystick Y coordinate as mV (0 .. 3300)
int adc_read_joyy();

// --- Expander control ---
void expander_lcd_reset(uint8_t state);

#if (BOARD_VERSION < 4) // hard connected on V1.4+
void expander_lcd_cs(uint8_t state);
#else
void expander_lcd_rd(uint8_t state);
#endif

void expander_audio_amplifier_reset(uint8_t state);

// --- Audio amplifier ---
void audio_amp_write(uint8_t u8_reg, uint8_t u8_data);
uint8_t audio_amp_read(uint8_t u8_reg);

// --- Power control ---
void expander_power_off();

// --- Audio control ---
// 0 (mute) to 255 (max)
void audio_set_volume(uint8_t u8_volume);

// toggle on/off vibrator
void audio_set_vibrator(uint8_t u8_on);

#endif // __EXPANDER_H_
