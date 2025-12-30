#include "ghost.h"
#include "game/game.h"
#include "maze.h"
#include "config.h"
#include "assets/assets.h"
#include "core/graphics.h"
#include "core/sprite.h"
#include <queue>
#include <algorithm>
#include <random>
#include <cmath>

extern float g_camera_y;
extern int debug;

static std::mt19937 ghost_rng(123456);

// ------------------------------------------------------------
// Helpers direction
// ------------------------------------------------------------
int Ghost::dirX(Dir d) {
    switch (d) {
        case Dir::Left:  return -1;
        case Dir::Right: return +1;
        default: return 0;
    }
}
int Ghost::dirY(Dir d) {
    switch (d) {
        case Dir::Up:    return -1;
        case Dir::Down:  return +1;
        default: return 0;
    }
}

void Ghost::reverse_direction()
{
    switch (dir)
    {
        case Dir::Left:  dir = Dir::Right; break;
        case Dir::Right: dir = Dir::Left;  break;
        case Dir::Up:    dir = Dir::Down;  break;
        case Dir::Down:  dir = Dir::Up;    break;
        default: break;
    }
}


bool Ghost::isCentered() const {
    return pixel_offset == 0;
}

static bool is_opposite(Ghost::Dir a, Ghost::Dir b) {
    return (a == Ghost::Dir::Left  && b == Ghost::Dir::Right) ||
           (a == Ghost::Dir::Right && b == Ghost::Dir::Left)  ||
           (a == Ghost::Dir::Up    && b == Ghost::Dir::Down)  ||
           (a == Ghost::Dir::Down  && b == Ghost::Dir::Up);
}

// ------------------------------------------------------------
// BFS pour les yeux (Eaten)
// ------------------------------------------------------------
static bool bfs_to_target(const Maze& maze,
                          int sr, int sc,
                          int tr, int tc,
                          std::vector<std::pair<int,int>>& path)
{
    struct Node { int r, c; };
    static const int dr[4] = {-1, +1, 0, 0};
    static const int dc[4] = {0, 0, -1, +1};

    bool visited[MAZE_HEIGHT][MAZE_WIDTH] = {};
    Node parent[MAZE_HEIGHT][MAZE_WIDTH];

    std::queue<Node> q;
    q.push({sr, sc});
    visited[sr][sc] = true;

    while (!q.empty()) {
        Node n = q.front(); q.pop();

        if (n.r == tr && n.c == tc) {
            path.clear();
            int r = tr, c = tc;
            while (!(r == sr && c == sc)) {
                path.push_back({r, c});
                Node p = parent[r][c];
                r = p.r;
                c = p.c;
            }
            std::reverse(path.begin(), path.end());
            return true;
        }

        for (int i = 0; i < 4; i++) {
            int nr = n.r + dr[i];
            int nc = n.c + dc[i];

            if (nr < 0 || nr >= MAZE_HEIGHT ||
                nc < 0 || nc >= MAZE_WIDTH)
                continue;

			if (!is_walkable_for_eyes(maze.tiles[nr][nc]))
				continue;

            if (!visited[nr][nc]) {
                visited[nr][nc] = true;
                parent[nr][nc] = n;
                q.push({nr, nc});
            }
        }
    }
    return false;
}

// ------------------------------------------------------------
// Constructeur
// ------------------------------------------------------------
Ghost::Ghost(int id_, int start_c, int start_r)
{
    id = id_;
    tile_r = start_r;
    tile_c = start_c;
    pixel_offset = 0;

    x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
    y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;

    dir = Dir::Left;
    next_dir = Dir::Left;

    mode = Mode::Scatter;
    previous_mode = Mode::Scatter;

    houseState = HouseState::Inside;
    eaten_timer = 0;

    speed_normal     = GHOST_SPEED;
    speed_frightened = GHOST_SPEED_FRIGHTENED;
    speed_tunnel     = GHOST_SPEED_TUNNEL;
    speed_eyes       = GHOST_SPEED_EYES;
}

// ------------------------------------------------------------
// Reset après mort de Pac-Man
// ------------------------------------------------------------
void Ghost::reset_to_start()
{
    tile_r = start_row;
    tile_c = start_col;
    pixel_offset = 0;

    dir = Dir::Left;
    next_dir = Dir::Left;

    mode = Mode::Scatter;
    previous_mode = Mode::Scatter;

    houseState = HouseState::Inside;
    eaten_timer = 0;

    path.clear();
}

// ------------------------------------------------------------
// Frightened
// ------------------------------------------------------------
void Ghost::on_start_frightened()
{
    if (mode == Mode::Eaten)
        return;

    previous_mode = mode;
    mode = Mode::Frightened;

    // Demi-tour instantané
    switch (dir) {
        case Dir::Left:  dir = Dir::Right; break;
        case Dir::Right: dir = Dir::Left;  break;
        case Dir::Up:    dir = Dir::Down;  break;
        case Dir::Down:  dir = Dir::Up;    break;
        default: break;
    }
}

void Ghost::on_end_frightened()
{
    if (mode == Mode::Frightened)
        mode = previous_mode;
}

// ------------------------------------------------------------
// Directions valides
// ------------------------------------------------------------
std::vector<Ghost::Dir> Ghost::getValidDirections(const GameState& g,
                                                  int row, int col,
                                                  bool is_eyes) const
{
    std::vector<Dir> dirs;
    dirs.reserve(4);

    for (Dir d : {Dir::Up, Dir::Left, Dir::Down, Dir::Right})
    {
        int nr = row + dirY(d);
        int nc = col + dirX(d);

        if (nr < 0 || nr >= MAZE_HEIGHT ||
            nc < 0 || nc >= MAZE_WIDTH)
            continue;

        TileType t = g.maze.tiles[nr][nc];

        if (is_eyes) {
            if (t == TileType::Wall)
                continue;
        }
        else {
            if (t == TileType::Wall ||
                t == TileType::GhostDoorClosed ||
                t == TileType::GhostDoorOpening)
                continue;

            if (houseState == HouseState::Outside &&
                (t == TileType::GhostHouse || t == TileType::GhostDoorOpen))
                continue;
        }

        dirs.push_back(d);
    }

    return dirs;
}

bool is_walkable_for_eyes(TileType t)
{
    switch (t)
    {
        case TileType::Wall:
            return false;

        // Les yeux peuvent traverser la maison et la porte
        case TileType::GhostHouse:
        case TileType::GhostDoorClosed:
        case TileType::GhostDoorOpening:
        case TileType::GhostDoorOpen:
            return true;

        // Les yeux peuvent traverser toutes les cases normales
        case TileType::Empty:
        case TileType::Pellet:
        case TileType::PowerPellet:
        case TileType::Tunnel:
        case TileType::TunnelEntry:
            return true;

        default:
            return true;
    }
}


// ------------------------------------------------------------
// Choix direction vers cible
// ------------------------------------------------------------
Ghost::Dir Ghost::chooseDirectionTowardsTarget(const GameState& g,
                                               int row, int col,
                                               int tr, int tc,
                                               bool is_eyes) const
{
    auto valid = getValidDirections(g, row, col, is_eyes);
    if (valid.empty())
        return dir;

    std::vector<Dir> filtered;
    for (Dir d : valid)
        if (!is_opposite(d, dir))
            filtered.push_back(d);

    if (!filtered.empty())
        valid = filtered;

    Dir best = valid.front();
    int bestDist = 999999;

    for (Dir d : valid)
    {
        int nr = row + dirY(d);
        int nc = col + dirX(d);
        int dist = std::abs(nr - tr) + std::abs(nc - tc);

        if (dist < bestDist) {
            best = d;
            bestDist = dist;
        }
    }

    return best;
}

Ghost::Dir Ghost::chooseDirectionInsideHouse(const GameState& g, int row, int col)
{
    std::vector<Dir> valid;

    auto try_add = [&](Dir d)
    {
        int nr = row + dirY(d);
        int nc = col + dirX(d);

        if (g.maze.tiles[nr][nc] == TileType::GhostHouse)
            valid.push_back(d);
    };

    try_add(Dir::Up);
    try_add(Dir::Down);
    try_add(Dir::Left);
    try_add(Dir::Right);

    if (valid.empty())
        return dir; // fail-safe

    // Choix simple : direction aléatoire parmi les valides
    return valid[rand() % valid.size()];
}


// ------------------------------------------------------------
// Random frightened
// ------------------------------------------------------------
Ghost::Dir Ghost::chooseRandomDir(const GameState& g,
                                  int row, int col,
                                  bool is_eyes) const
{
    auto valid = getValidDirections(g, row, col, is_eyes);
    if (valid.empty())
        return dir;

    std::vector<Dir> filtered;
    for (Dir d : valid)
        if (!is_opposite(d, dir))
            filtered.push_back(d);

    if (!filtered.empty())
        valid = filtered;

    std::uniform_int_distribution<int> dist(0, valid.size() - 1);
    return valid[dist(ghost_rng)];
}

// ------------------------------------------------------------
// Scatter / Chase targets
// ------------------------------------------------------------
void Ghost::getScatterTarget(const GameState& g, int& tr, int& tc) const
{
    switch (id)
    {
        case 0: tr = 0;              tc = MAZE_WIDTH - 1; break;
        case 1: tr = 0;              tc = 0;              break;
        case 2: tr = MAZE_HEIGHT-1;  tc = MAZE_WIDTH - 1; break;
        case 3: tr = MAZE_HEIGHT-1;  tc = 0;              break;
        default: tr = 0;             tc = MAZE_WIDTH - 1; break;
    }
}

void Ghost::getChaseTarget(const GameState& g, int& tr, int& tc) const
{
    int pr = g.pacman.tileRow();
    int pc = g.pacman.tileCol();

    switch (id)
    {
        case 0: tr = pr; tc = pc; break;

        case 1:
            tr = pr + 4 * Ghost::dirY((Ghost::Dir)g.pacman.dir);
            tc = pc + 4 * Ghost::dirX((Ghost::Dir)g.pacman.dir);
            break;

        case 2:
        {
            const Ghost* blinky = nullptr;
            for (const auto& gh : g.ghosts)
                if (gh.id == 0) blinky = &gh;

            int aheadR = pr + 2 * Ghost::dirY((Ghost::Dir)g.pacman.dir);
            int aheadC = pc + 2 * Ghost::dirX((Ghost::Dir)g.pacman.dir);

            if (blinky) {
                int br = blinky->tile_r;
                int bc = blinky->tile_c;
                int vr = aheadR - br;
                int vc = aheadC - bc;
                tr = br + 2 * vr;
                tc = bc + 2 * vc;
            } else {
                tr = aheadR;
                tc = aheadC;
            }
            break;
        }

        case 3:
        {
            int dist = std::abs(tile_r - pr) + std::abs(tile_c - pc);
            if (dist >= 8) {
                tr = pr;
                tc = pc;
            } else {
                getScatterTarget(g, tr, tc);
            }
            break;
        }

        default:
            tr = pr;
            tc = pc;
            break;
    }
}

// ------------------------------------------------------------
// Update principal
// ------------------------------------------------------------
void Ghost::update(GameState& g)
{
    bool is_eyes = (mode == Mode::Eaten);
    int row = tile_r;
    int col = tile_c;
	
	// ------------------------------------------------------------
	// FAIL-SAFE : si le fantôme est sur une case normale du labyrinthe,
	// il doit être considéré comme Outside.
	// ------------------------------------------------------------
	TileType t = g.maze.tiles[row][col];

	if (t != TileType::GhostHouse &&
		t != TileType::GhostDoorClosed &&
		t != TileType::GhostDoorOpening &&
		t != TileType::GhostDoorOpen)
	{
		// Si le fantôme est physiquement dehors, son état doit refléter cela
		if (houseState != HouseState::Outside)
			houseState = HouseState::Outside;
	}


    int speed = speed_normal;
    if (mode == Mode::Frightened) speed = speed_frightened;
    if (mode == Mode::Eaten)      speed = speed_eyes;

    if (g.maze.tiles[row][col] == TileType::Tunnel)
	{
        speed = speed_tunnel;
	}	

	// ---------------------------
	// 1) Mode Eaten → Returning
	// ---------------------------
	if (mode == Mode::Eaten)
	{
		const int speed = GHOST_SPEED_EYES;

		int tr = g.maze.ghost_center_row;
		int tc = g.maze.ghost_center_col;

		if (tile_r == tr && tile_c == tc)
		{
			mode = previous_mode;
			houseState = HouseState::Inside;
			eaten_timer = 30;
			releaseTime_ticks = g.elapsed_ticks + g.ghostReleaseInterval_ticks;
			pixel_offset = 0;
			dir = Dir::Up;

			x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE)/2;
			y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE)/2;
			return;
		}

		pixel_offset += speed;

		if (pixel_offset >= TILE_SIZE)
		{
			pixel_offset = 0;

			prev_tile_r = tile_r;
			prev_tile_c = tile_c;

			if (path.empty())
				bfs_to_target(g.maze, tile_r, tile_c, tr, tc, path);

			if (!path.empty())
			{
				auto [nr, nc] = path.front();
				path.erase(path.begin());

				if (nr < tile_r)      dir = Dir::Up;
				else if (nr > tile_r) dir = Dir::Down;
				else if (nc < tile_c) dir = Dir::Left;
				else if (nc > tile_c) dir = Dir::Right;

				tile_r = nr;
				tile_c = nc;
			}

			if (try_portal_wrap(g, g.portalH, *this) ||	try_portal_wrap(g, g.portalV, *this))
			{
				x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE)/2;
				y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE)/2;
				return;
			}
		}

		x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE)/2 + dirX(dir) * pixel_offset;
		y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE)/2 + dirY(dir) * pixel_offset;

		return;
	}


	
	// ---------------------------
	// 2) Inside : déplacement libre dans la maison
	// ---------------------------
	if (houseState == HouseState::Inside)
	{
		bool door_open   = (g.ghostDoorState == GameState::DoorState::Open);
		bool can_release = (g.elapsed_ticks >= releaseTime_ticks);

		// --- CAS A & B : déplacement libre dans la maison ---
		// - Porte fermée
		// - OU porte ouverte mais pas encore mon tour
		if (!door_open || (door_open && !can_release))
		{
			if (isCentered())
			{
				dir = chooseDirectionInsideHouse(g, row, col);
			}

			pixel_offset += speed;
			if (pixel_offset >= TILE_SIZE)
			{
				int nr = row + dirY(dir);
				int nc = col + dirX(dir);

				if (g.maze.tiles[nr][nc] == TileType::GhostHouse)
				{
					tile_r = nr;
					tile_c = nc;
				}

				pixel_offset = 0;
			}

			// Toujours tenir x/y synchro, même s'ils "semblent" figés
			x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2 + dirX(dir) * pixel_offset;
			y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2 + dirY(dir) * pixel_offset;

			return;
		}

		// --- CAS C : porte ouverte ET c'est mon tour → diriger vers la porte ---
		// 1) si déjà sous la porte ouverte et centré → passer en Leaving
		int up_r = tile_r - 1;
		int up_c = tile_c;
		if (isCentered() &&
			up_r >= 0 && up_r < MAZE_HEIGHT &&
			g.maze.tiles[up_r][up_c] == TileType::GhostDoorOpen)
		{
			houseState   = HouseState::Leaving;
			dir          = Dir::Up;
			pixel_offset = 0;

			x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
			y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
			return;
		}

		// 2) sinon → BFS vers la case juste sous la porte
		if (isCentered())
		{
			int target_r = g.maze.ghost_door_row + 1;
			int target_c = g.maze.ghost_door_col;

			if (path.empty())
				bfs_to_target(g.maze, row, col, target_r, target_c, path);

			if (!path.empty())
			{
				auto [nr, nc] = path.front();
				path.erase(path.begin());

				if (nr < row)      dir = Dir::Up;
				else if (nr > row) dir = Dir::Down;
				else if (nc < col) dir = Dir::Left;
				else if (nc > col) dir = Dir::Right;
			}
		}

		pixel_offset += speed;
		if (pixel_offset >= TILE_SIZE)
		{
			tile_r += dirY(dir);
			tile_c += dirX(dir);
			pixel_offset = 0;
		}

		x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2 + dirX(dir) * pixel_offset;
		y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2 + dirY(dir) * pixel_offset;

		return;
	}


	// ---------------------------
	// 3) Leaving → Outside
	// ---------------------------
	if (houseState == HouseState::Leaving)
	{
		// Si la porte se referme → bloqué
		if (g.ghostDoorState != GameState::DoorState::Open)
		{
			pixel_offset = 0;

			// Mise à jour visuelle même figé
			x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
			y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
			return;
		}

		// Avancer vers le haut
		pixel_offset += speed;
		if (pixel_offset >= TILE_SIZE)
		{
			tile_r += dirY(dir);
			tile_c += dirX(dir);
			pixel_offset = 0;
		}

		// Mise à jour visuelle obligatoire
		x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2 + dirX(dir) * pixel_offset;
		y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2 + dirY(dir) * pixel_offset;

		// Dès qu'on quitte la porte → Outside
		TileType t = g.maze.tiles[tile_r][tile_c];
		if (t != TileType::GhostDoorOpen)
		{
			houseState = HouseState::Outside;

			if (mode != Mode::Eaten && mode != Mode::Frightened)
			{
				mode = (g.global_mode == GlobalGhostMode::Scatter)
					   ? Mode::Scatter
					   : Mode::Chase;
			}
		}

		return;
	}


    // ---------------------------
    // 4) Choix direction (Scatter / Chase / Frightened)
    // ---------------------------
    if (isCentered())
    {
        if (mode == Mode::Frightened)
        {
            dir = chooseRandomDir(g, row, col, is_eyes);
        }
        else if (mode == Mode::Scatter)
        {
            int tr, tc;
            getScatterTarget(g, tr, tc);
            dir = chooseDirectionTowardsTarget(g, row, col, tr, tc, is_eyes);
        }
        else if (mode == Mode::Chase)
        {
            int tr, tc;
            getChaseTarget(g, tr, tc);
            dir = chooseDirectionTowardsTarget(g, row, col, tr, tc, is_eyes);
        }
    }

	// ---------------------------
	// 5) Mouvement case-based
	// ---------------------------
	pixel_offset += speed;

	if (pixel_offset >= TILE_SIZE)
	{
		
		// Mémoriser la tuile précédente
		prev_tile_r = tile_r; 
		prev_tile_c = tile_c;
		
		// Nouvelle tuile
		tile_r += dirY(dir);
		tile_c += dirX(dir);
		pixel_offset = 0;

		int nr, nc;
		if (try_portal_wrap(g, g.portalH, *this) || try_portal_wrap(g, g.portalH, *this))
		{

			x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE)/2;
			y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE)/2;
			return;
		}
	}

	// --- Position normale (pas de wrap) ---
	x = tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2 + dirX(dir) * pixel_offset;
	y = tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2 + dirY(dir) * pixel_offset;
}	


// ------------------------------------------------------------
// Draw
// ------------------------------------------------------------
void Ghost::draw() const
{
    const uint16_t* body_anim[2] = { nullptr, nullptr };
    const uint16_t* eyes = nullptr;

    bool is_frightened = (mode == Mode::Frightened);
    bool is_eyes       = (mode == Mode::Eaten);

    // --- 1) Sélection du corps ---
    if (is_eyes)
    {
        // Corps invisible → on ne dessine que les yeux
        body_anim[0] = nullptr;
        body_anim[1] = nullptr;
    }
    else if (is_frightened)
    {
        body_anim[0] = ghost_scared_0;
        body_anim[1] = ghost_scared_1;
    }
    else
    {
        switch (id)
        {
            case 0: body_anim[0] = ghost_red_0;    body_anim[1] = ghost_red_1;    break;
            case 1: body_anim[0] = ghost_blue_0;   body_anim[1] = ghost_blue_1;   break;
            case 2: body_anim[0] = ghost_pink_0;   body_anim[1] = ghost_pink_1;   break;
            case 3: body_anim[0] = ghost_orange_0; body_anim[1] = ghost_orange_1; break;
        }
    }

    // --- 2) Sélection des yeux ---
    switch (dir)
    {
        case Dir::Left:  eyes = ghost_eyes_left;  break;
        case Dir::Right: eyes = ghost_eyes_right; break;
        case Dir::Up:    eyes = ghost_eyes_up;    break;
        case Dir::Down:  eyes = ghost_eyes_down;  break;
        default:         eyes = ghost_eyes_left;  break;
    }

    // Offsets EXACTS que tu utilisais avant
    static const EyeOffset eyeOffsets[4] = {
        {2, 4}, // Left
        {4, 4}, // Right
        {3, 2}, // Up
        {3, 6}  // Down
    };

    int frame = (animTick / 8) % 2;
    int sx = x;
    int sy = y - (int)g_camera_y;

    // --- 3) Dessin du corps ---
    if (body_anim[0])
        gfx_drawSprite(sx, sy, body_anim[frame], GHOST_SIZE, GHOST_SIZE);

    // --- 4) Dessin des yeux (sauf frightened & eaten) ---
	if (!is_frightened && !is_eyes)
	{
		int idx = -1;
		switch (dir)
		{
			case Dir::Left:  idx = 0; break;
			case Dir::Right: idx = 1; break;
			case Dir::Up:    idx = 2; break;
			case Dir::Down:  idx = 3; break;
			default:         idx = 0; break; // fallback gauche
		}

		const EyeOffset& off = eyeOffsets[idx];
		gfx_drawSprite(sx + off.dx, sy + off.dy, eyes, 10, 5);
	}
	else if (is_eyes)
	{
		// Mode Eaten → yeux seuls, centrés
		gfx_drawSprite(sx + 3, sy + 4, eyes, 10, 5);
	}

}

