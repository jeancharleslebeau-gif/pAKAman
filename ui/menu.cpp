#include "menu.h"
#include "core/graphics.h"
#include "core/input.h"

MenuSel menu_loop(){
    Keys k;
    int sel=0;
    for(;;){
        gfx_clear(COLOR_BLACK);
        gfx_text(80,80,"1. Jouer",COLOR_WHITE);
        gfx_text(80,100,"2. Editeur",COLOR_WHITE);
        gfx_text(80,120,"3. Scores",COLOR_WHITE);
        gfx_flush();
        input_poll(k);
        if(k.A){ if(sel==0) return MenuSel::Start; if(sel==1) return MenuSel::Editor; if(sel==2) return MenuSel::HighScores; }
    }
}
