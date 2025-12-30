#include "pacman.h"
#include "config.h"
#include "maze.h"
#include "game/game.h"
#include "core/graphics.h"
#include "assets/assets.h"
#include "core/sprite.h"
#include "core/input.h"
#include "core/audio.h"


extern float g_camera_y;
extern int debug;
char debugText[64];

#define DBG(code) do { if (debug) { code; } } while(0)

// ------------------------------------------------------------
// Helpers internes
// ------------------------------------------------------------

static bool pac_can_move(const GameState& g, const Pacman& p, Pacman::Dir d)
{
    int nr = p.tile_r + Pacman::dirY(d);
    int nc = p.tile_c + Pacman::dirX(d);

    if (nr < 0 || nr >= MAZE_HEIGHT || nc < 0 || nc >= MAZE_WIDTH)
        return false;

    TileType t = g.maze.tiles[nr][nc];

    // Pac-Man ne traverse pas les murs ni la porte fermée
    if (t == TileType::Wall ||
        t == TileType::GhostDoorClosed ||
        t == TileType::GhostDoorOpening)
        return false;

    return true;
}

static bool is_opposite(Pacman::Dir a, Pacman::Dir b)
{
    return (a == Pacman::Dir::Left  && b == Pacman::Dir::Right) ||
           (a == Pacman::Dir::Right && b == Pacman::Dir::Left)  ||
           (a == Pacman::Dir::Up    && b == Pacman::Dir::Down)  ||
           (a == Pacman::Dir::Down  && b == Pacman::Dir::Up);
}


// ------------------------------------------------------------
// UPDATE
// ------------------------------------------------------------
void Pacman::update(GameState& g)
{
    // ------------------------------------------------------------
    // 1) INPUT : lecture + filtrage → next_dir
    // ------------------------------------------------------------
    Keys k;
    input_poll(k);

    Dir requestedDir = Dir::None;
    const int DEADZONE = 500;

    if (k.left  || (k.joxx < JOYX_MID - DEADZONE)) requestedDir = Dir::Left;
    if (k.right || (k.joxx > JOYX_MID + DEADZONE)) requestedDir = Dir::Right;
    if (k.up    || (k.joxy > JOYX_MID + DEADZONE)) requestedDir = Dir::Up;
    if (k.down  || (k.joxy < JOYX_MID - DEADZONE)) requestedDir = Dir::Down;

    // Mémorise l’intention du joueur (persistante pour les virages anticipés)
    next_dir = requestedDir;

    // ------------------------------------------------------------
    // 2) LOGIQUE CASE-BASED : décisions de direction
    // ------------------------------------------------------------
    int speed = PACMAN_SPEED;
    bool centered = (pixel_offset == 0);

	if (centered)
	{
		// 1) Demi-tour toujours autorisé (arcade)
		if (next_dir != dir && is_opposite(next_dir, dir))
		{
			dir = next_dir;
		}
		// 2) Virage normal si possible
		else if (next_dir != dir && pac_can_move(g, *this, next_dir))
		{
			dir = next_dir;
		}

		// 3) Direction actuelle impossible → stop
		if (!pac_can_move(g, *this, dir))
			dir = Dir::None;
	}


    // ------------------------------------------------------------
    // 3) AVANCEMENT AVEC SNAP OFFSET (vitesses non multiples de 16 OK)
    // ------------------------------------------------------------
    if (dir != Dir::None && speed > 0)
    {
        pixel_offset += speed;

        if (pixel_offset >= TILE_SIZE)
        {
			// Mémoriser la tuile précédente
			prev_tile_r = tile_r; 
			prev_tile_c = tile_c;
	
			// On entre dans une nouvelle tuile	
            tile_r += dirY(dir);
            tile_c += dirX(dir);
            pixel_offset = 0;

			// --------------------------------------------------------
			// 3b) TUNNEL WRAP EN TILES
			// --------------------------------------------------------
			if (try_portal_wrap(g, g.portalH, *this) ||
				try_portal_wrap(g, g.portalV, *this))
			{
				// try_portal_wrap a déjà mis à jour tile_r / tile_c / pixel_offset / dir
				x = tile_c * TILE_SIZE + PACMAN_OFFSET;
				y = tile_r * TILE_SIZE + PACMAN_OFFSET;
				return;
			}

            // --------------------------------------------------------
            // 4) COLLECTE (PELLETS / POWER PELLETS)
            // --------------------------------------------------------
            TileType& cell = g.maze.tiles[tile_r][tile_c];

            if (cell == TileType::Pellet)
            {
                cell = TileType::Empty;
                g.maze.pellet_count--;
                g.score += DOT_SCORE;
                audio_play_pacgomme();
            }
            else if (cell == TileType::PowerPellet)
            {
                cell = TileType::Empty;
                g.maze.power_pellet_count--;
                g.score += POWERDOT_SCORE;
                audio_play_power();
                game_trigger_frightened(g);
            }
        }
    }

    // ------------------------------------------------------------
    // 5) MISE À JOUR POSITION PIXEL (depuis tile + offset)
    // ------------------------------------------------------------
    x = tile_c * TILE_SIZE + PACMAN_OFFSET + dirX(dir) * pixel_offset;
    y = tile_r * TILE_SIZE + PACMAN_OFFSET + dirY(dir) * pixel_offset;

    // ------------------------------------------------------------
    // 6) ANIMATION
    // ------------------------------------------------------------
    animTick++;

    // ------------------------------------------------------------
    // 7) DEBUG : préparation de la chaîne (affichée dans draw())
    // ------------------------------------------------------------
    /* DBG({
        snprintf(debugText, sizeof(debugText),
                 "P r=%d c=%d off=%d dir=%d next=%d",
                 tile_r, tile_c, pixel_offset, (int)dir, (int)next_dir);
    }); */
}


// ------------------------------------------------------------
// DRAW
// ------------------------------------------------------------
void Pacman::draw(const GameState& g) const
{
    (void)g;

    const uint16_t* sprites[3];

    switch (dir)
    {
        case Dir::Right:
        case Dir::None:
            sprites[0] = pacman_right_0;
            sprites[1] = pacman_right_1;
            sprites[2] = pacman_right_2;
            break;

        case Dir::Left:
            sprites[0] = pacman_left_0;
            sprites[1] = pacman_left_1;
            sprites[2] = pacman_left_2;
            break;

        case Dir::Up:
            sprites[0] = pacman_up_0;
            sprites[1] = pacman_up_1;
            sprites[2] = pacman_up_2;
            break;

        case Dir::Down:
            sprites[0] = pacman_down_0;
            sprites[1] = pacman_down_1;
            sprites[2] = pacman_down_2;
            break;
    }

    int frame = (animTick / 4) % 3;
    const uint16_t* sprite = sprites[frame];

    int screen_x = x;
    int screen_y = y - (int)g_camera_y;

    gfx_drawSprite(screen_x, screen_y, sprite, PACMAN_SIZE, PACMAN_SIZE);
	
	/* DBG({
		gfx_text(0, 60, debugText, COLOR_GREEN);
	}); */
}
