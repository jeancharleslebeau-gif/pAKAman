#include "title_screen.h"
#include "assets/title_image.h"
#include "core/graphics.h"
#include "core/input.h"
#include "game/config.h"
#include "lib/graphics_basic.h"
#include "lib/expander.h"          	// ✅ pour EXPANDER_KEY_A
#include "freertos/FreeRTOS.h" 		// ✅ pour pdMS_TO_TICKS
#include "freertos/task.h"     		// ✅ pour vTaskDelay

void title_screen_show() {
    /* gfx_clear(COLOR_BLACK); */
    lcd_draw_bitmap(title_image_pixels, SCREEN_W ,SCREEN_H , 0, 0);
    gfx_text(50, 190, "Appuyez sur A pour lancer", COLOR_WHITE); 
	/* graphics_basic gfx;
	gfx.setColor( COLOR_WHITE);
	gfx.fillRect(0, 0, SCREEN_W, SCREEN_H); */
    gfx_flush();
	lcd_refresh();
}
