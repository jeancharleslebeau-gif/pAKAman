		#include "common.h"
		#include <esp_lcd_panel_io.h>
		#include "esp_lcd_io_i80.h"
		#include "LCD.h"
		#include "expander.h"
		#include <string.h> // memcpy
		#include "core/graphics.h"

		// définition réelle
		uint16_t framebuffer[320 * 240];

		static void delay(uint32_t u32_ms)
		{
			vTaskDelay(u32_ms / portTICK_PERIOD_MS);
		}

		IRAM_ATTR uint32_t millis()
		{
			return xTaskGetTickCount() * 1000 / portTICK_PERIOD_MS;
		}

		volatile uint32_t u32_start_refresh = 0;
		volatile uint32_t u32_delta_refresh = 0;
		volatile uint32_t u32_refresh_ctr = 0;
		volatile uint32_t u32_draw_count = 0;

		IRAM_ATTR bool color_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
		{

			//  esp_lcd_panel_io_rx_param(lcd_panel_h io, ILI9342C_GET_SCANLINE, void *param, size_t param_size);

			//  Serial.printf( "color_trans_done_cb!\n");
			//  LCD_FAST_test();

			// bug !!  u32_delta_refresh = millis()-u32_start_refresh;
			u32_delta_refresh = 0;
			u32_refresh_ctr = u32_refresh_ctr + 1;
			u32_start_refresh = 0;
			return false;
		}

		/**
		 * @brief LCD clock source
		 * @note User should select the clock source based on the real requirement:
		 * @verbatim embed:rst:leading-asterisk
		 * +---------------------+-------------------------+----------------------------+
		 * | LCD clock source    | Features                | Power Management           |
		 * +=====================+=========================+============================+
		 * | LCD_CLK_SRC_PLL160M | High resolution         | ESP_PM_APB_FREQ_MAX lock   |
		 * +---------------------+-------------------------+----------------------------+
		 * | LCD_CLK_SRC_PLL240M | High resolution         | ESP_PM_APB_FREQ_MAX lock   |
		 * +---------------------+-------------------------+----------------------------+
		 * | LCD_CLK_SRC_APLL    | Configurable resolution | ESP_PM_NO_LIGHT_SLEEP lock |
		 * +---------------------+-------------------------+----------------------------+
		 * | LCD_CLK_SRC_XTAL    | Medium resolution       | No PM lock                 |
		 * +---------------------+-------------------------+----------------------------+
		 * @endverbatim
		 */
		esp_lcd_i80_bus_handle_t i80_bus = NULL;
		esp_lcd_i80_bus_config_t bus_config = {
			.dc_gpio_num = LCD_PIN_DnC,
			.wr_gpio_num = LCD_PIN_nWR,
			.clk_src = LCD_CLK_SRC_PLL160M, // 7.885 ms
											//    .clk_src = LCD_CLK_SRC_PLL240M, // 7.885 ms
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
			.bus_width = 8,
			.max_transfer_bytes = 320 * 240 * sizeof(uint16_t), // transfer 100 lines of pixels (assume pixel is RGB565) at most in one transaction
																//    .max_transfer_bytes = 320 * 100 * sizeof(uint16_t), // transfer 100 lines of pixels (assume pixel is RGB565) at most in one transaction
			.psram_trans_align = 64,
			.sram_trans_align = 64,
		};

		esp_lcd_panel_io_i80_config_t panel_config = {
			.cs_gpio_num = -1,
			// 320x240*2*50 Hz = 8MHz Min : target < 20 ms
			//  .pclk_hz = 80000000, // 1 ms 1000 fps, FAIL colors
			//  .pclk_hz = 40000000, //  5 ms / 200 fps, FAIL colors
			//  .pclk_hz = 32000000, //  5 ms / 200 fps, FAIL colors
			//  .pclk_hz = 24000000, //  7 ms / 140 fps, FAIL colors
			.pclk_hz = 20000000, //  8 ms / 125 fps, OK ILI 2.6 TFT + ST 2.8
								 //  .pclk_hz = 16000000, // 10 ms / 100 fps, OK ( 70 fps screen )
								 //  .pclk_hz = 8000000,  // 20 ms / 50 fps, OK
								 //  .pclk_hz = 4000000,  // 39 ms / 25 fps, OK
								 //  .pclk_hz = 2000000,  // 77 ms / 12 fps, OK

			//  .trans_queue_depth = 320*240,
			.trans_queue_depth = 320 * 16,
			.on_color_trans_done = color_trans_done_cb,
			.user_ctx = 0,
			.lcd_cmd_bits = 8,
			.lcd_param_bits = 8,
			.dc_levels = {
				.dc_idle_level = 1,
				.dc_cmd_level = 0,
				.dc_dummy_level = 1,
				.dc_data_level = 1},
			.flags = {.cs_active_high = 0, .reverse_color_bits = 0, .swap_color_bytes = 1, .pclk_active_neg = 0, .pclk_idle_low = 0}};
		esp_lcd_panel_io_handle_t lcd_panel_h;
		#include "hal/lcd_types.h"
		#include "esp_lcd_panel_st7789.h"

		#include "esp_lcd_io_i80.h"
		/*
		esp_lcd_panel_dev_config_t st7789_config = {
			.reset_gpio_num = -1,
			.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
			.bits_per_pixel = 16,
		};
		esp_lcd_panel_handle_t st7789_h;
		*/
		void LCD_i80_bus_init()
		{
			printf("esp_lcd_new_i80_bus\n");
			esp_err_t ret;
			ret = esp_lcd_new_i80_bus(&bus_config, &i80_bus);
			printf("esp_lcd_new_i80_bus return %d\n", ret);
			if (ret)
				for (;;)
					delay(1000);

			printf("esp_lcd_new_panel_io_i80\n");
			//    ESP_ERROR_CHECK( ret=esp_lcd_new_panel_io_i80(i80_bus, &panel_config, &lcd_panel_h) );
			ret = esp_lcd_new_panel_io_i80(i80_bus, &panel_config, &lcd_panel_h);
			printf("esp_lcd_new_panel_io_i80 return %d\n", ret);
			if (ret)
				for (;;)
					delay(1000);
		}

		void LCD_i80_bus_uninit()
		{
			esp_lcd_panel_io_del(lcd_panel_h);
			esp_lcd_del_i80_bus(i80_bus);
		}

		// hw reset
		void ILI9342C_hard_reset()
		{
			expander_lcd_reset(0);
			delay(10);
			expander_lcd_reset(1);
			delay(100);
		#if (BOARD_VERSION < 4) // CD# hard wired on v1.4+
			expander_lcd_cs(0);
		#endif
			delay(100);
		}
		/*
		void ILI9342C_write_cmd( uint8_t cmd, const uint8_t *pu8_data, int len )
		{
			esp_err_t ret = esp_lcd_panel_io_tx_param( lcd_panel_h, cmd, pu8_data, len );
			if (ret)
			{
				printf("ILI9342C_write_cmd(0x%02X) FAIL\n", cmd );
				delay(1000);
			}
		}
		*/

		void lcd_draw_text(uint16_t x, uint16_t y, const char *pc)
		{
			lcd_draw_str(x, y, pc); // wrapper autour de lcd_draw_str
		}

		#include "esp_lcd_panel_ops.h"

		uint32_t u32_reg_config[400];
		void save_pin_config()
		{
			for (uint32_t i = 0; i < 400; i++)
				u32_reg_config[i] = ((uint32_t *)DR_REG_GPIO_BASE)[i];
			/*
			  for ( int i = 0 ; i < sizeof(u8_i80_bus_pins); i++ )
			  {
				  uint32_t ulPin = u8_i80_bus_pins[i];
				  EPortType port = g_APinDescription[ulPin].ulPort;
				  uint32_t pin = g_APinDescription[ulPin].ulPin;
				  uint32_t pinMask = (1ul << pin);
			  }
			  */
		}
		void restaure_pin_config()
		{
			for (uint32_t i = 0; i < 400; i++)
			{
				uint32_t u32_new = ((uint32_t *)DR_REG_GPIO_BASE)[i];
				if (u32_new != u32_reg_config[i])
				{
					//      printf( "0x%04X : old 0x%08X new 0x%08X\n", i, u32_reg_config[i], u32_new );
					((uint32_t *)DR_REG_GPIO_BASE)[i] = u32_reg_config[i];
				}
			}

			/*
			  ((uint32_t*)DR_REG_GPIO_BASE)[0x08] = u32_reg_config[0x08] ;
			  ((uint32_t*)DR_REG_GPIO_BASE)[0x0B] = u32_reg_config[0x0B] ;
			  ((uint32_t*)DR_REG_GPIO_BASE)[0x0F] = u32_reg_config[0x0F] ;
			  ((uint32_t*)DR_REG_GPIO_BASE)[0x10] = u32_reg_config[0x10] ;
			*/
		}

		#include "hal/gpio_hal.h"
		#include "driver/gpio.h"
		#define LOW 0
		#define HIGH 1

		#define INPUT 1
		#define OUTPUT 0

		void pinMode(gpio_num_t pin, uint8_t dir)
		{
			gpio_config_t io_conf{};
			io_conf.pin_bit_mask = 1ULL << pin;
			io_conf.mode = (dir == OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
			io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
			io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
			io_conf.intr_type = GPIO_INTR_DISABLE;

			gpio_config(&io_conf);
		}

		void digitalWrite(gpio_num_t pin, uint8_t u8_level)
		{
			gpio_ll_set_level(&GPIO, pin, u8_level);
		}

		// only for GPIO from 0 to 31
		/*void digitalWriteFast( gpio_num_t pin, uint8_t u8_level )
		{
			if (u8_level)
				GPIO.out_w1ts = 1 << pin;
			else
				GPIO.out_w1tc = 1 << pin;
		}
		*/

		int digitalRead(gpio_num_t pin)
		{
			return gpio_get_level(pin);
		}

		#include "esp_timer.h"
		void delayMicroseconds(uint32_t u32_us)
		{
			//    u32_us = 10;
			uint64_t start_us = esp_timer_get_time();
			while ((esp_timer_get_time() - start_us) < u32_us)
				;
		}

		const gpio_num_t u8_data_bus[8] = {LCD_PIN_DB0, LCD_PIN_DB1, LCD_PIN_DB2, LCD_PIN_DB3, LCD_PIN_DB4, LCD_PIN_DB5, LCD_PIN_DB6, LCD_PIN_DB7};

		void lcd_set_bus_dir(uint8_t u8_mode)
		{
			for (int i = 0; i < 8; i++)
				pinMode(u8_data_bus[i], u8_mode);
		}
		void lcd_write_bus_data(uint8_t u8_data)
		{
			digitalWrite(LCD_PIN_nWR, HIGH);
			for (int i = 0; i < 8; i++)
			{
				digitalWrite(u8_data_bus[i], u8_data & 1);
				u8_data >>= 1;
			}
			digitalWrite(LCD_PIN_nWR, LOW);
			digitalWrite(LCD_PIN_nWR, HIGH);
		}

		uint8_t lcd_read_bus_data()
		{
			uint8_t u8_data = 0;
		#if (BOARD_VERSION < 4) // RD# en I2C expander
			digitalWrite(LCD_PIN_nRD, HIGH);
			digitalWrite(LCD_PIN_nRD, LOW);
		#else
			expander_lcd_rd(1);
			expander_lcd_rd(0);
		#endif
			lcd_set_bus_dir(INPUT);
			delayMicroseconds(10);
			//  digitalWrite( LCD_PIN_nRD, HIGH);

			for (int i = 0; i < 8; i++)
			{
				u8_data <<= 1;
				u8_data |= digitalRead(u8_data_bus[7 - i]) ? 1 : 0;
			}
		#if (BOARD_VERSION < 4) // RD# en I2C expander
			digitalWrite(LCD_PIN_nRD, HIGH);
		#else
			expander_lcd_rd(1);
		#endif
			delayMicroseconds(10);
			return u8_data;
		}

		void ILI9342C_write_cmd_soft(uint8_t cmd, const uint8_t *pu8_data, int len)
		{
			digitalWrite(LCD_PIN_DnC, LOW);
			lcd_write_bus_data(cmd);
			digitalWrite(LCD_PIN_DnC, HIGH);
			while (len--)
				lcd_write_bus_data(*pu8_data++);
		}

		void lcd_init_io()
		{
		#if (BOARD_VERSION < 4) // RD# en I2C expander
			pinMode(LCD_PIN_nRD, OUTPUT);
		#else
			pinMode(LCD_FMARK, INPUT);
		#endif

			pinMode(LCD_PIN_nWR, OUTPUT);
			pinMode(LCD_PIN_DnC, OUTPUT);
			pinMode(LCD_PIN_nRST, OUTPUT);

		#if (BOARD_VERSION < 4) // RD# en I2C expander
			digitalWrite(LCD_PIN_nRD, HIGH);
		#else
			expander_lcd_rd(1);
		#endif
			digitalWrite(LCD_PIN_nWR, HIGH);
			digitalWrite(LCD_PIN_DnC, HIGH);
			digitalWrite(LCD_PIN_nRST, HIGH);

			lcd_set_bus_dir(OUTPUT);
		}

		void LCD_FAST_test(const uint16_t *u16_pframe_buffer)
		{
			u32_start_refresh = millis(); // start date
										  //    ESP_ERROR_CHECK( esp_lcd_panel_io_tx_color ( lcd_panel_h, ST7789V_CMD_RAMWR, u16_pframe_buffer, 320*240*2 ) );
										  // plantage ?
			esp_lcd_panel_io_tx_color(lcd_panel_h, ST7789V_CMD_RAMWR, u16_pframe_buffer, 320 * 240 * 2);
		}

		void ILI9342C_write_cmd(uint8_t cmd, const uint8_t *pu8_data, int len)
		{
			esp_lcd_panel_io_tx_param(lcd_panel_h, cmd, pu8_data, len);
		}

		uint32_t LCD_last_refresh_delay()
		{
			return u32_delta_refresh;
		}

		// get Scanline
		uint8_t lcd_read_scanline()
		{
			uint8_t u8_ret = 0;

			lcd_set_bus_dir(OUTPUT);
			ILI9342C_write_cmd(ST7789V_CMD_GET_SCANLINE, 0, 0);
			lcd_set_bus_dir(INPUT);
			u8_ret = lcd_read_bus_data(); // trash dumy byte
			u8_ret = lcd_read_bus_data(); // trash MSB
			u8_ret = lcd_read_bus_data(); // b0..7
			return u8_ret;
		}

		#ifdef USE_PSRAM_VIDEO_BUFFER
		extern uint16_t *framebuffer; // dynamic in PSRAM (display)
		#else
		extern uint16_t framebuffer[320 * 240]; // static in SRAM (rendrer)
		#endif

		/*void LCD_SLOW_test(const uint16_t* u16_pframe_buffer)
		{
			digitalWrite( LCD_PIN_DnC, LOW);
			lcd_write_bus_data( ST7789V_CMD_RAMWR );
			digitalWrite( LCD_PIN_DnC, HIGH);
			for (int x = 0 ; x < LCD_V_RESOLUTION ; x++ )
			{
				for (int y = 0 ; y < LCD_H_RESOLUTION ; y++ )
				{
					uint16_t u16_pix = framebuffer[x + (239-y)*LCD_V_RESOLUTION];
					lcd_write_bus_data( u16_pix>>8 );
					lcd_write_bus_data( u16_pix&255 );
				}
			}
		}*/
		void LCD_SLOW_test(const uint16_t *u16_pframe_buffer)
		{
			digitalWrite(LCD_PIN_DnC, LOW);
			lcd_write_bus_data(ST7789V_CMD_RAMWR);
			digitalWrite(LCD_PIN_DnC, HIGH);
			for (int x = 0; x < 320 * 240; x++)
			{
				uint16_t u16_pix = framebuffer[x];
				lcd_write_bus_data(u16_pix >> 8);
				lcd_write_bus_data(u16_pix & 255);
			}
		}

		// without fix orientation
		// 565RGB to 565BGR
		void load_rgb(const uint16_t *u16_pframe_buffer)
		{
			for (int x = 0; x < 320 * 240; x++)
			{
				uint16_t u16_pix = u16_pframe_buffer[x];
				//        uint16_t u16_pix2 = u16_pix;
				//        u16_pix &= 0b0000011111100000;
				//        u16_pix |= u16_pix2>>(5+6);
				//        u16_pix |= u16_pix2<<(5+6);
				framebuffer[x] = u16_pix;
			}
		}

		const uint8_t gamebuinoLogo[] = {
			80,
			10,
			0b00111100,
			0b00111111,
			0b00111111,
			0b11110011,
			0b11110011,
			0b11110011,
			0b00110011,
			0b00111111,
			0b00111111,
			0b00011100,
			0b00111100,
			0b00111111,
			0b00111111,
			0b11110011,
			0b11110011,
			0b11110011,
			0b00110011,
			0b00111111,
			0b00111111,
			0b00100110,
			0b00110000,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00000011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00100110,
			0b00110000,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00000011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00101010,
			0b00110011,
			0b00111111,
			0b00110011,
			0b00110011,
			0b11110011,
			0b11000011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00011100,
			0b00110011,
			0b00111111,
			0b00110011,
			0b00110011,
			0b11110011,
			0b11000011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00000000,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00000011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00000000,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00000011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00110011,
			0b00000000,
			0b00111111,
			0b00110011,
			0b00110011,
			0b00110011,
			0b11110011,
			0b11110011,
			0b11110011,
			0b00110011,
			0b00111111,
			0b00000000,
			0b00111111,
			0b00110011,
			0b00110011,
			0b00110011,
			0b11110011,
			0b11110011,
			0b11110011,
			0b00110011,
			0b00111111,
			0b00000000,
		};

		void lcd_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint16_t u16_pix_color)
		{
			uint8_t w = *(bitmap++);
			uint8_t h = *(bitmap++);

			uint8_t byteWidth = (w + 7) / 8;
			uint8_t _x = x;
			uint8_t dw = 8 - (w % 8);
			for (uint8_t j = 0; j < h; j++)
			{
				x = _x;
				for (uint8_t i = 0; i < byteWidth;)
				{
					uint8_t b = *(bitmap++);
					i++;
					for (uint8_t k = 0; k < 8; k++)
					{
						if (i == byteWidth && k == dw)
						{
							x += (w % 8);
							break;
						}
						if (b & 0x80)
						{
							lcd_putpixel(x, y, u16_pix_color);
						}
						b <<= 1;
						x++;
					}
				}
				y++;
			}
		}

		void lcd_clear(uint16_t u16_pix_color)
		{
			for (int x = 0; x < 320 * 240; x++)
				framebuffer[x] = u16_pix_color;
		}

		/*
		// 565RGB to 565BGR with fix orientation
		void load_rgb( const uint16_t* u16_pframe_buffer )
		{
			for (int x = 0 ; x < LCD_V_RESOLUTION ; x++ )
			{
				for (int y = 0 ; y < LCD_H_RESOLUTION ; y++ )
				{
					uint16_t u16_pix = u16_pframe_buffer[x + (239-y)*LCD_V_RESOLUTION];
					uint16_t u16_pix2 = u16_pix;
					u16_pix &= 0b0000011111100000;
					u16_pix |= u16_pix2>>(5+6);
					u16_pix |= u16_pix2<<(5+6);
					framebuffer[x*240 + y] = u16_pix;
				}
			}
		}
		*/
		// ST7789V_MADCTRL_MV
		void st7789v_rotation_set(uint8_t rotation)
		{
			static uint8_t mad_cfg = 0;
			switch (rotation % 4)
			{
			case 0:
				mad_cfg = ST7789V_MADCTRL_MX | ST7789V_MADCTRL_MY | ST7789V_MADCTRL_RGB;
				// Column address (MX): Right to left
				// Page address (MY): Bottom to top
				// Page/ Column order (MV): normal
				// RGB/BGR order: RGB
				break;
			case 1:
				mad_cfg = ST7789V_MADCTRL_MV | ST7789V_MADCTRL_RGB;
				// Column address (MX): Left to right
				// Page address (MY): Top to bottom
				// Page/ Column order (MV): reverse
				// RGB/BGR order: RGB
				break;
			case 2:
				mad_cfg = ST7789V_MADCTRL_RGB;
				// Column address (MX): Left to right
				// Page address (MY): Top to bottom
				// Page/ Column order (MV): normal
				// RGB/BGR order: RGB
				break;
			case 3:
				mad_cfg = ST7789V_MADCTRL_MX | ST7789V_MADCTRL_MV | ST7789V_MADCTRL_RGB;
				// Column address (MX): Right to left
				// Page address (MY): Top to bottom
				// Page/ Column order (MV): reverse
				// RGB/BGR order: RGB
				break;
			default:
				break;
			}
			ILI9342C_write_cmd(ST7789V_CMD_MADCTL, &mad_cfg, 1);
		}

		void set_addr_window(uint16_t x_0, uint16_t y_0, uint16_t x_1, uint16_t y_1)
		{
			// copy coordinates
			static uint8_t x_coord[4];
			static uint8_t y_coord[4];
			x_coord[0] = x_0 >> 8;
			x_coord[1] = (uint8_t)x_0;
			x_coord[2] = x_1 >> 8;
			x_coord[3] = (uint8_t)x_1;
			y_coord[0] = y_0 >> 8;
			y_coord[1] = (uint8_t)y_0;
			y_coord[2] = y_1 >> 8;
			y_coord[3] = (uint8_t)y_1;
			ILI9342C_write_cmd(ST7789V_CMD_CASET, x_coord, 4);
			ILI9342C_write_cmd(ST7789V_CMD_RASET, y_coord, 4);
		}

		//! set hard fps to @u8_fps, from 40 to 100
		void lcd_set_fps(uint8_t u8_fps)
		{
			if (u8_fps > 100)
				u8_fps = 100; // bound max available  fps
			if (u8_fps < 40)
				u8_fps = 40; // bound min available fps
			uint32_t u32_div = 10000000;
			u32_div = u32_div / (336 * u8_fps);
			if (u32_div > 250)
				u32_div -= 250;
			else
				u32_div = 0;
			uint8_t u8_fr_cfg = u32_div / 16;
			if (u8_fr_cfg > 31) // 5 bits value
				u8_fr_cfg = 31;
			if (u8_fr_cfg < 1)
				u8_fr_cfg = 1;
			printf("Sef FPS to %d : %d\n", u8_fps, u8_fr_cfg);
			ILI9342C_write_cmd(0xC6, &u8_fr_cfg, 1);
		}

		void LCD_init()
		{
			lcd_init_io();
			esp_timer_early_init();
			//    uint8_t u8_scanline = 0;
			printf("LCD_i80_bus_init()\n");
			LCD_i80_bus_init();

			ILI9342C_hard_reset();
			//    delay(200);
			ILI9342C_write_cmd(ST7789V_CMD_RESET, 0, 0);
			//    delay(200);

			ILI9342C_write_cmd(ST7789V_CMD_SLPOUT, 0, 0);
			const uint8_t color_mode[] = {COLOR_MODE_65K | COLOR_MODE_16BIT};
			ILI9342C_write_cmd(ST7789V_CMD_COLMOD, color_mode, 1);
			//    delay(10);

			ILI9342C_write_cmd(ST7789V_CMD_INVON, 0, 0);
			//  ILI9342C_write_cmd( ST7789V_CMD_INVOFF, 0, 0);
			//    delay(10);
			ILI9342C_write_cmd(ST7789V_CMD_NORON, 0, 0);
			//    delay(10);

			st7789v_rotation_set(3);

			set_addr_window(0, 0, 319, 239);
			/*
				const uint8_t mad_cfg[] = { ST7789V_MADCTRL_MV | ST7789V_MADCTRL_RGB };
				ILI9342C_write_cmd( ST7789V_CMD_MADCTL, mad_cfg, 1);
				delay(10);

				const uint8_t vscsad[] = { 0x01, 0x40 };
				ILI9342C_write_cmd( 0x37, vscsad, 2);
				delay(10);
					// 0x37,0x00,0x50
			*/

			/*
		//le diviseur ne marche pas sur le list frame sync
		//regarder si diviseur par 8 a un effet sur l'affichage
			const uint8_t fr_adv_cfg[] = { 0x0 + 3, 0x1E, 0x1E }; // refresh div/8, base 40 FPS => 5 fps
			ILI9342C_write_cmd( 0xB3, fr_adv_cfg, 3);
			delay(10);
			*/

			ILI9342C_write_cmd(ST7789V_CMD_DISPON, 0, 0);
			//    delay(500);

			const uint8_t te_cfg[] = {0x00}; // Tearing effecct output mode : V-Blanking information only
			ILI9342C_write_cmd(0x35, te_cfg, 1);

			// RDID
			/*
				// ST7789V : 0x85 0x85 0x52 ( non conforme doc )
				lcd_set_bus_dir( OUTPUT );
				digitalWrite( LCD_PIN_DnC, LOW);
				lcd_write_bus_data( ST7789V_CMD_RDID );
				digitalWrite( LCD_PIN_DnC, HIGH);
				lcd_set_bus_dir( INPUT );
				printf( "LCD RDId : " );
				for ( int i = 0 ; i < 4 ; i++ )
					printf( " 0x%02X", lcd_read_bus_data() );
				printf( "\n" );
			*/

			/*
			#ifdef USE_VSYCNC
				printf( "Loop scanline\n" );
				  // get Scanline
				for ( int i = 0 ; i < 100 ; i++ )
				{
					u8_scanline = lcd_read_scanline();
					#if (BOARD_VERSION < 4) // RD# en I2C expander
					printf( "LCD SCAN LINE : %d\n", u8_scanline );
					#else
					printf( "LCD SCAN LINE : %d / %d\n", u8_scanline, digitalRead( LCD_FMARK ) );
					#endif
				}
			#endif
			*/

			if (0)
			{
				lcd_set_bus_dir(OUTPUT);
				// write data
				printf("clear screen\n");
				for (int x = 0; x < 320 * 240; x++)
					framebuffer[x] = 0;
				LCD_SLOW_test(framebuffer);
			}

			if (0)
			{
				printf("GPIO speed test\n");
				for (int i = 0; i < 8; i++)
				{
					uint64_t start_us = esp_timer_get_time();
					for (int lp = 0; lp < 1000000; lp++)
					{
						gpio_ll_set_level(&GPIO, u8_data_bus[i], 1);
						gpio_ll_set_level(&GPIO, u8_data_bus[i], 0);
					}
					uint64_t end_us = esp_timer_get_time();
					printf("GPIO%d : %0.3f ms\n", i, (end_us - start_us) / 1000.0);
				}
			}
			// load picture from flash
			//    printf( "Load rgb from flash\n" );
			//    load_rgb( _4_data );

			// 565 B G R
			if (0)
			{
				printf("write data\n");
				uint64_t start_draw_us = esp_timer_get_time();
				LCD_SLOW_test(framebuffer);
				uint64_t end_draw_us = esp_timer_get_time();
				printf("draw time %0.3fms\n", (end_draw_us - start_draw_us) / 1000.0);
				//    for (;;)
			}

			if (0)
				for (int i = 0; i < 256; i++)
				{
					uint8_t mad_cfg = i;
					ILI9342C_write_cmd(ST7789V_CMD_MADCTL, &mad_cfg, 1);
					printf("MAD = 0x%02X\n", mad_cfg);
					LCD_SLOW_test(framebuffer);
					delay(500);
				}

			if (0)
			{
				printf("LCD_FAST_test()\n");
				uint64_t start_draw_us = esp_timer_get_time();
				u32_refresh_ctr = 0;
				LCD_FAST_test(framebuffer);
				while (u32_refresh_ctr == 0)
					;
				uint64_t end_draw_us = esp_timer_get_time();
				printf("draw time %0.3fms\n", (end_draw_us - start_draw_us) / 1000.0);
				//        delay(2000);
			}
			/*
				load_rgb( _5_data );
				u32_refresh_ctr = 0;
				LCD_FAST_test(framebuffer);
				while( u32_refresh_ctr == 0 );
				delay(2000);

				load_rgb( _3_data );
				u32_refresh_ctr = 0;
				LCD_FAST_test(framebuffer);
				while( u32_refresh_ctr == 0 );
				delay(2000);
			*/
			//    load_rgb( outrun_cpc_320X240_data );
			u32_refresh_ctr = 1;

			if (0)
			{
				lcd_clear(color_black);
				const gamebuino_color_e color_list[] = {color_white, color_gray, color_darkgray, color_black, color_purple, color_pink, color_red, color_orange, color_brown, color_beige, color_yellow, color_lightgreen, color_green, color_darkblue, color_blue, color_lightblue};
				for (size_t i = 0; i < count_of(color_list); i++)
				{
					uint16_t u16_color = color_list[i];
					lcd_drawBitmap((320 - 80) / 2, 15 * i, gamebuinoLogo, u16_color);
					lcd_move_cursor(0, 15 * i);
					lcd_printf("%2d", i);
				}
				lcd_refresh();
				delay(1000);
			}

			lcd_set_fps(100); // max for intro scrool
			for (int16_t y = -10; y < (240 - 10) / 2; y += 4)
			{
				//        lcd_clear(color_blue);
				lcd_clear(color_black);
				lcd_drawBitmap((320 - 80) / 2, y, gamebuinoLogo, color_orange);
				lcd_refresh();
				while (!lcd_refresh_completed())
					; // for simple buffer : wait refresh completed before erase screen buffer
			}
			//    delay(500);
			lcd_set_fps(50); // default fps

			//    delay(2000);
			//    printf( "LCD_i80_bus_uninit()\n" );
			//    LCD_i80_bus_uninit();
			//    delay(2000);
		}

		#include "assets/font8x8_basic.h"
		
		extern esp_lcd_panel_io_i80_config_t panel_config;

		bool lcd_is_rgb565() {
			return panel_config.flags.swap_color_bytes;
		}

		void lcd_putpixel(uint16_t x, uint16_t y, uint16_t u16_color)
		{
			uint32_t u32_offset = x + y * 320;
			if (u32_offset < 320 * 240)
			{
				framebuffer[u32_offset] = u16_color;
				// swap des octets manuellement
				// uint16_t swapped = (u16_color >> 8) | (u16_color << 8);
				// framebuffer[u32_offset] = swapped;
			}
		}

		void lcd_draw_char(uint16_t x, uint16_t y, char c)
		{
			for (uint16_t dy = 0; dy < 8; dy++)
			{
				uint8_t u8_line = font8x8_basic[(uint8_t)c][dy];

				uint16_t color = current_text_color;
		#ifdef LCD_COLOR_ORDER_RGB
				color = (color >> 8) | (color << 8);
		#endif

				for (uint16_t dx = 0; dx < 8; dx++)
				{
					if (u8_line & 1)
					{
						lcd_putpixel(x + dx, y + dy, color);
					}
					u8_line >>= 1;
				}
			}
		}

		void lcd_draw_char_bg(uint16_t x, uint16_t y, char c, uint16_t bgColor)
		{
			for (uint16_t dy = 0; dy < 8; dy++)
			{
				uint8_t u8_line = font8x8_basic[(uint8_t)c][dy];

				uint16_t color = current_text_color;
				uint16_t bg    = bgColor;
		#ifdef LCD_COLOR_ORDER_RGB
				color = (color >> 8) | (color << 8);
				bg    = (bg >> 8) | (bg << 8);
		#endif

				for (uint16_t dx = 0; dx < 8; dx++)
				{
					if (u8_line & 1)
					{
						lcd_putpixel(x + dx, y + dy, color); // texte
					}
					else
					{
						lcd_putpixel(x + dx, y + dy, bg);    // fond
					}
					u8_line >>= 1;
				}
			}
		}

		void lcd_draw_str(uint16_t x, uint16_t y, const char *pc)
		{
			while (*pc)
			{
				lcd_draw_char(x, y, *pc++);
				x += 8;
			}
		}

		void lcd_draw_str_bg(uint16_t x, uint16_t y, const char *pc, uint16_t bgColor)
		{
			while (*pc)
			{
				lcd_draw_char_bg(x, y, *pc++, bgColor);
				x += 8;
			}
		}

		uint8_t lcd_refresh_completed()
		{
			return u32_refresh_ctr;
		}

		void lcd_refresh()
		{
			while (u32_refresh_ctr == 0)
				; // wait last update completed (DMA)
			u32_refresh_ctr = 0; // reset complete dma flag

		#ifdef USE_VSYCNC
			// Sync to Scanline
			uint64_t start_us = esp_timer_get_time();
			while (digitalRead(LCD_FMARK)) {
				if ((esp_timer_get_time() - start_us) > 20000) {
					printf("ERROR : Loop scanline 1 timeout\n");
					break;
				}
			}

			start_us = esp_timer_get_time();
			while (digitalRead(LCD_FMARK) == 0) {
				if ((esp_timer_get_time() - start_us) > 20000) {
					printf("ERROR : Loop scanline 0 timeout\n");
					break;
				}
			}
		#endif

			u32_draw_count = u32_draw_count + 1;
			LCD_FAST_test(framebuffer); // start DMA update
		}

		uint16_t cursor_x = 0;
		uint16_t cursor_y = 0;

		void lcd_move_cursor(uint16_t x, uint16_t y)
		{
			if (x < 320)
				cursor_x = x;
			if (y < 240)
				cursor_y = y;
		}

		void lcd_printf(const char *pc_format, ...)
		{
			va_list ap;
			static char pc_message_buffer[256];
			va_start(ap, pc_format);												// init stack ptr from pc_format parameter
			vsnprintf(pc_message_buffer, sizeof(pc_message_buffer), pc_format, ap); // fromat the string
			va_end(ap);																// release ptr

			//	return UART_print_str(pc_message_buffer);
			char *pc = pc_message_buffer;
			while (*pc)
			{
				switch (*pc)
				{
				case '\r':
					cursor_x = 0;
					break;
				case '\n':
					cursor_x = 0;
					cursor_y += 8;
					break;
				default:
					lcd_draw_char(cursor_x, cursor_y, *pc);
					cursor_x += 8;
					break;
				}
				if (cursor_x >= 320)
				{
					cursor_x = 0;
					cursor_y += 8;
				}
				if (cursor_y >= 240)
				{
					memcpy(framebuffer, &framebuffer[320 * 8], (320 * 240 - 320 * 8) * 2);
					memset(&framebuffer[320 * (240 - 8)], 0, 320 * 8 * 2);
					cursor_y -= 8;
				}
				pc++;
			}
		}
