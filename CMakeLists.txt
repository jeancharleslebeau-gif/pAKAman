set(ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)

file(GLOB_RECURSE GAME_ASSETS  	
    ${ASSETS_DIR}/*
    ${ASSETS_DIR}/tiles/*
    ${ASSETS_DIR}/sounds/*
)

idf_component_register(SRCS

	"app_main.cpp"  

        # Lib / matériel
        "lib/LCD.cpp"
        "lib/expander.cpp"          
        "lib/graphics_basic.cpp"  
        "lib/sdcard.cpp"
	"lib/audio_sfx.cpp" 
	"lib/audio_player.cpp"


        # Core
        "core/audio.cpp"
        "core/input.cpp"
        "core/graphics.cpp"
        "core/sprite_atlas.cpp"
        "core/persist.cpp"      
	"core/sprite.cpp"


	 # UI
    	"ui/title_screen.cpp"
    	"ui/highscores.cpp"
    	"ui/menu.cpp"


	# Game
    	"game/config.cpp"
    	"game/game.cpp"
    	"game/maze.cpp"
    	"game/pacman.cpp"
    	"game/ghost.cpp"
    	"game/level.cpp"
    

    ${GAME_ASSETS}

    INCLUDE_DIRS .
)