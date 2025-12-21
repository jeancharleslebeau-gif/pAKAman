	#include "ghost.h"
	#include "assets/assets.h"
	#include "core/graphics.h"
	#include "core/sprite.h"
	#include "maze.h"
	#include "game/config.h"
	#include "game/game.h"
	#include <cstdlib>
	#include <queue>
	#include <algorithm>   // Pour utiliser reverse


	extern float g_camera_y;

	// Helpers direction
	inline int dirX(Ghost::Dir d) {
		switch (d) {
			case Ghost::Dir::Left:  return -1;
			case Ghost::Dir::Right: return +1;
			default: return 0;
		}
	}
	inline int dirY(Ghost::Dir d) {
		switch (d) {
			case Ghost::Dir::Up:    return -1;
			case Ghost::Dir::Down:  return +1;
			default: return 0;
		}
	}

	// Helpers Pacman
	inline int pacDirX(Pacman::Dir d) {
		switch (d) {
			case Pacman::Dir::Left:  return -1;
			case Pacman::Dir::Right: return +1;
			default: return 0;
		}
	}
	inline int pacDirY(Pacman::Dir d) {
		switch (d) {
			case Pacman::Dir::Up:    return -1;
			case Pacman::Dir::Down:  return +1;
			default: return 0;
		}
	}

	// ------------------ IA Chase/Scatter/Frightened ---------------------

	Ghost::Dir chooseDirectChase(GameState& g, int row, int col) {
		int pacRow = g.pacman.y / TILE_SIZE;
		int pacCol = g.pacman.x / TILE_SIZE;
		int dRow = pacRow - row;
		int dCol = pacCol - col;

		if (abs(dCol) > abs(dRow))
			return (dCol > 0) ? Ghost::Dir::Right : Ghost::Dir::Left;
		else
			return (dRow > 0) ? Ghost::Dir::Down : Ghost::Dir::Up;
	}

	Ghost::Dir chooseInterceptor(GameState& g, int row, int col) {
		int pacRow = g.pacman.y / TILE_SIZE;
		int pacCol = g.pacman.x / TILE_SIZE;

		int targetRow = pacRow + 4 * pacDirY(g.pacman.dir);
		int targetCol = pacCol + 4 * pacDirX(g.pacman.dir);

		int dRow = targetRow - row;
		int dCol = targetCol - col;

		if (abs(dCol) > abs(dRow))
			return (dCol > 0) ? Ghost::Dir::Right : Ghost::Dir::Left;
		else
			return (dRow > 0) ? Ghost::Dir::Down : Ghost::Dir::Up;
	}

	Ghost::Dir chooseIndirect(GameState& g, int row, int col) {
		int pacRow = g.pacman.y / TILE_SIZE;
		int pacCol = g.pacman.x / TILE_SIZE;
		int dRow = pacRow - row;
		int dCol = pacCol - col;

		if (abs(dRow) + abs(dCol) < 4) {
			if (abs(dCol) > abs(dRow))
				return (dCol > 0) ? Ghost::Dir::Left : Ghost::Dir::Right;
			else
				return (dRow > 0) ? Ghost::Dir::Up : Ghost::Dir::Down;
		} else {
			if (abs(dCol) > abs(dRow))
				return (dCol > 0) ? Ghost::Dir::Right : Ghost::Dir::Left;
			else
				return (dRow > 0) ? Ghost::Dir::Down : Ghost::Dir::Up;
		}
	}

	Ghost::Dir chooseRandom(GameState& g, int row, int col) {
		Ghost::Dir dirs[4] = {Ghost::Dir::Left, Ghost::Dir::Right, Ghost::Dir::Up, Ghost::Dir::Down};
		for (int tries = 0; tries < 10; tries++) {
			Ghost::Dir d = dirs[rand() % 4];
			int nr = row + dirY(d);
			int nc = col + dirX(d);
			TileType t = g.maze.tiles[nr][nc];
			if (t != TileType::Wall && 
				t != TileType::GhostDoorClosed &&
				t != TileType::GhostDoorOpening	)
				return d;
		}
		return Ghost::Dir::Left;
	}

	// ------------------ BFS pathfinding pour les yeux -------------------

	struct Node {
		int row, col;
	};

	static bool find_shortest_path_to_house(const Maze& maze,
											int sr, int sc,
											int tr, int tc,
											std::vector<Ghost::Dir>& path)
	{
		static const int dr[4] = {-1, +1, 0, 0};
		static const int dc[4] = {0, 0, -1, +1};
		static const Ghost::Dir dirs[4] = {
			Ghost::Dir::Up, Ghost::Dir::Down,
			Ghost::Dir::Left, Ghost::Dir::Right
		};

		bool visited[MAZE_HEIGHT][MAZE_WIDTH] = {};
		Ghost::Dir parent_dir[MAZE_HEIGHT][MAZE_WIDTH];
		Node parent[MAZE_HEIGHT][MAZE_WIDTH];

		std::queue<Node> q;
		q.push({sr, sc});
		visited[sr][sc] = true;

		while (!q.empty()) {
			Node n = q.front(); q.pop();

			if (n.row == tr && n.col == tc) {
				// reconstruction path
				int r = tr, c = tc;
				path.clear();
				while (!(r == sr && c == sc)) {
					path.push_back(parent_dir[r][c]);
					Node p = parent[r][c];
					r = p.row;
					c = p.col;
				}
				std::reverse(path.begin(), path.end());
				return true;
			}

			for (int i = 0; i < 4; i++) {
				int nr = n.row + dr[i];
				int nc = n.col + dc[i];

				if (nr < 0 || nr >= MAZE_HEIGHT || nc < 0 || nc >= MAZE_WIDTH)
					continue;

				TileType t = maze.tiles[nr][nc];

				// Les yeux peuvent traverser GhostDoor, GhostHouse, Tunnel, TunnelEntry
				if (t == TileType::Wall)
					continue;

				if (!visited[nr][nc]) {
					visited[nr][nc] = true;
					parent[nr][nc] = n;
					parent_dir[nr][nc] = dirs[i];
					q.push({nr, nc});
				}
			}
		}
		return false;
	}

	// ------------------ Collision solide pour les fantômes ----------------

	static bool is_blocking_for_ghost(const GameState& g, int r, int c, bool is_eyes, Ghost::HouseState houseState)
	{
		TileType t = g.maze.tiles[r][c];

		// 1. Les yeux ne sont bloqués que par les murs
		if (is_eyes)
			return (t == TileType::Wall);

		// 2. Fantômes dans la maison : murs seulement
		if (houseState == Ghost::HouseState::Inside)
			return (t == TileType::Wall);

		// 3. Fantômes en train de sortir : seule GhostDoorOpen est traversable
		if (houseState == Ghost::HouseState::Leaving)
		{
			if (t == TileType::Wall)
				return true;

			if (t == TileType::GhostDoorClosed ||
				t == TileType::GhostDoorOpening)
				return true;

			// ✅ GhostDoorOpen est traversable
			return false;
		}

		// 4. Fantômes dehors : porte et maison bloquantes
		if (houseState == Ghost::HouseState::Outside)
		{
			if (t == TileType::Wall ||
				t == TileType::GhostHouse ||
				t == TileType::GhostDoorClosed ||
				t == TileType::GhostDoorOpening ||
				t == TileType::GhostDoorOpen)
				return true;

			return false;
		}

		return false;
	}


	static bool checkCollision(const GameState& g, int nextX, int nextY, Ghost::Dir dir, bool is_eyes, Ghost::HouseState houseState)
	{
		switch (dir) {
			case Ghost::Dir::Right: {
				int right = nextX + GHOST_SIZE - 1;
				int rowTop = nextY / TILE_SIZE;
				int rowBottom = (nextY + GHOST_SIZE - 1) / TILE_SIZE;
				int colRight = right / TILE_SIZE;
				return is_blocking_for_ghost(g, rowTop, colRight, is_eyes, houseState)
					|| is_blocking_for_ghost(g, rowBottom, colRight, is_eyes, houseState);
			}

			case Ghost::Dir::Left: {
				int left = nextX;
				int rowTop = nextY / TILE_SIZE;
				int rowBottom = (nextY + GHOST_SIZE - 1) / TILE_SIZE;
				int colLeft = left / TILE_SIZE;
				return is_blocking_for_ghost(g, rowTop, colLeft, is_eyes, houseState)
					|| is_blocking_for_ghost(g, rowBottom, colLeft, is_eyes, houseState);
			}

			case Ghost::Dir::Down: {
				int bottom = nextY + GHOST_SIZE - 1;
				int colLeft = nextX / TILE_SIZE;
				int colRight = (nextX + GHOST_SIZE - 1) / TILE_SIZE;
				int rowBottom = bottom / TILE_SIZE;
				return is_blocking_for_ghost(g, rowBottom, colLeft, is_eyes, houseState)
					|| is_blocking_for_ghost(g, rowBottom, colRight, is_eyes, houseState);
			}

			case Ghost::Dir::Up: {
				int top = nextY;
				int colLeft = nextX / TILE_SIZE;
				int colRight = (nextX + GHOST_SIZE - 1) / TILE_SIZE;
				int rowTop = top / TILE_SIZE;
				return is_blocking_for_ghost(g, rowTop, colLeft, is_eyes, houseState)
					|| is_blocking_for_ghost(g, rowTop, colRight, is_eyes, houseState);
			}

			default:
				return false;
		}
	}


	// ------------------ Mode helpers -----------------

	void Ghost::reverse_direction()
	{
		switch (dir) {
			case Dir::Left:  dir = Dir::Right; break;
			case Dir::Right: dir = Dir::Left;  break;
			case Dir::Up:    dir = Dir::Down;  break;
			case Dir::Down:  dir = Dir::Up;    break;
			default: break;
		}
	}

	void Ghost::on_start_frightened()
	{
		if (mode == Mode::Chase || mode == Mode::Scatter) {
			mode = Mode::Frightened;
			reverse_direction();
		}
	}

	void Ghost::on_end_frightened()
	{
		if (mode == Mode::Frightened) {
			mode = Mode::Chase; // ou Scatter selon g.global_mode, mais plus simple ici
		}
	}

	// ------------------ Tunnel wrap (T/E) -----------------

	static bool try_tunnel_wrap(GameState& g, Ghost& ghost, int& nextX, int& nextY)
	{
		int row = ghost.y / TILE_SIZE;
		int col = ghost.x / TILE_SIZE;

		int dr = dirY(ghost.dir);
		int dc = dirX(ghost.dir);

		int curr_row = row;
		int curr_col = col;
		int next_row = row + dr;
		int next_col = col + dc;

		if (curr_row < 0 || curr_row >= MAZE_HEIGHT ||
			curr_col < 0 || curr_col >= MAZE_WIDTH ||
			next_row < 0 || next_row >= MAZE_HEIGHT ||
			next_col < 0 || next_col >= MAZE_WIDTH)
			return false;

		TileType curr_t = g.maze.tiles[curr_row][curr_col];
		TileType next_t = g.maze.tiles[next_row][next_col];

		// ✅ Nouvelle logique : entrée = E → T
		if (curr_t == TileType::TunnelEntry && next_t == TileType::Tunnel)
		{
			int t0r = -1, t0c = -1;
			int t1r = -1, t1c = -1;
			int countT = 0;

			for (int r = 0; r < MAZE_HEIGHT; r++)
			for (int c = 0; c < MAZE_WIDTH; c++)
			{
				if (g.maze.tiles[r][c] == TileType::Tunnel)
				{
					if (countT == 0) { t0r = r; t0c = c; }
					else if (countT == 1) { t1r = r; t1c = c; }
					countT++;
				}
			}

			if (countT != 2)
				return false;

			int dest_r, dest_c;

			if (next_row == t0r && next_col == t0c)
			{
				dest_r = t1r;
				dest_c = t1c;
			}
			else
			{
				dest_r = t0r;
				dest_c = t0c;
			}

			ghost.x = dest_c * TILE_SIZE;
			ghost.y = dest_r * TILE_SIZE;

			// ✅ Chercher E_right
			const int dr2[4] = {-1, +1, 0, 0};
			const int dc2[4] = {0, 0, -1, +1};
			const Ghost::Dir d2[4] = {
				Ghost::Dir::Up, Ghost::Dir::Down,
				Ghost::Dir::Left, Ghost::Dir::Right
			};

			for (int i = 0; i < 4; i++)
			{
				int nr = dest_r + dr2[i];
				int nc = dest_c + dc2[i];

				if (nr < 0 || nr >= MAZE_HEIGHT || nc < 0 || nc >= MAZE_WIDTH)
					continue;

				if (g.maze.tiles[nr][nc] == TileType::TunnelEntry)
				{
					ghost.dir = d2[i];
					break;
				}
			}

			nextX = ghost.x + dirX(ghost.dir) * GHOST_SPEED;
			nextY = ghost.y + dirY(ghost.dir) * GHOST_SPEED;

			return true;
		}

		return false;
	}


	// ------------------ Update -----------------

	void Ghost::update(GameState& g) {

		// ------------------------------------------------------------
		// Coordonnées de base
		// ------------------------------------------------------------
		int col = x / TILE_SIZE;
		int row = y / TILE_SIZE;

		int centerX = x + GHOST_SIZE/2;
		int centerY = y + GHOST_SIZE/2;
		int colCenter = col*TILE_SIZE + TILE_SIZE/2;
		int rowCenter = row*TILE_SIZE + TILE_SIZE/2;
		bool isCentered = (abs(centerX-colCenter)<=GHOST_SPEED) &&
						  (abs(centerY-rowCenter)<=GHOST_SPEED);

		bool is_eyes = (mode == Mode::Eaten);

		// ------------------------------------------------------------
		// 1) Gestion de la maison des fantômes (Inside / Leaving / Outside)
		// ------------------------------------------------------------
		switch (houseState)
		{
			// --------------------------------------------------------
			// Fantôme à l'intérieur de la maison : mouvement lent + rebonds
			// --------------------------------------------------------
			case HouseState::Inside:
			{
				int speed = 1;

				int tryX = x + dirX(dir) * speed;
				int tryY = y + dirY(dir) * speed;

				int rowTop    = tryY / TILE_SIZE;
				int rowBottom = (tryY + GHOST_SIZE - 1) / TILE_SIZE;
				int colLeft   = tryX / TILE_SIZE;
				int colRight  = (tryX + GHOST_SIZE - 1) / TILE_SIZE;

				auto isHouse = [&](int r, int c) {
					TileType t = g.maze.tiles[r][c];
					return (t == TileType::GhostHouse);
				};

				bool blocked =
					!isHouse(rowTop, colLeft)   ||
					!isHouse(rowTop, colRight)  ||
					!isHouse(rowBottom, colLeft)||
					!isHouse(rowBottom, colRight);

				if (blocked) {
					// Choisir une nouvelle direction interne
					Ghost::Dir dirs[4] = {Dir::Left, Dir::Right, Dir::Up, Dir::Down};
					for (int i = 0; i < 4; i++) {
						Dir d = dirs[rand() % 4];
						int nx = x + dirX(d) * speed;
						int ny = y + dirY(d) * speed;

						int rT = ny / TILE_SIZE;
						int rB = (ny + GHOST_SIZE - 1) / TILE_SIZE;
						int cL = nx / TILE_SIZE;
						int cR = (nx + GHOST_SIZE - 1) / TILE_SIZE;

						if (isHouse(rT, cL) && isHouse(rT, cR) &&
							isHouse(rB, cL) && isHouse(rB, cR))
						{
							dir = d;
							break;
						}
					}
				}
				else {
					x = tryX;
					y = tryY;
				}

				// Sortie autorisée si la porte est ouverte ET si c'est leur tour
				if (g.ghostDoorState == GameState::DoorState::Open &&
					g.elapsed_ticks >= releaseTime_ticks)
				{
					houseState = HouseState::Leaving;
				}

				return;
			}

			// --------------------------------------------------------
			// Fantôme en train de sortir : alignement horizontal → montée
			// --------------------------------------------------------
			case HouseState::Leaving:
			{
				int speed = GHOST_SPEED;

				// Recalculer row/col à partir de la position actuelle
				row = y / TILE_SIZE;
				col = x / TILE_SIZE;

				// DEBUG : où est le fantôme, sur quelle tile ?
				printf("[LEAVING] GHOST row=%d col=%d TILE=%d\n",
					   row, col, (int)g.maze.tiles[row][col]);

				// DEBUG : quelle est la tile sous la porte ?
				printf("[LEAVING] TILE BELOW DOOR row=%d col=%d type=%d\n",
					   g.maze.ghost_door_row + 1,
					   g.maze.ghost_door_col,
					   (int)g.maze.tiles[g.maze.ghost_door_row + 1][g.maze.ghost_door_col]);

				// ------------------------------------------------------------
				// 1) Alignement horizontal/vertical selon la position de la porte
				// ------------------------------------------------------------
				if (col < g.maze.ghost_door_col)
					dir = Dir::Right;
				else if (col > g.maze.ghost_door_col)
					dir = Dir::Left;
				else if (row < g.maze.ghost_door_row)
					dir = Dir::Down;
				else if (row > g.maze.ghost_door_row)
					dir = Dir::Up;
				// sinon : déjà sur la porte, on garde dir tel quel

				int nextX = x + dirX(dir) * speed;
				int nextY = y + dirY(dir) * speed;

				int testRow = nextY / TILE_SIZE;
				int testCol = nextX / TILE_SIZE;

				// DEBUG : quelle tile veut-il atteindre ?
				printf("[LEAVING] NEXT row=%d col=%d type=%d dir=%d\n",
					   testRow, testCol, (int)g.maze.tiles[testRow][testCol], (int)dir);

				bool blocked = checkCollision(g, nextX, nextY, dir, false, HouseState::Leaving);

				// DEBUG : collision ou pas ?
				printf("[LEAVING] checkCollision=%d\n", blocked ? 1 : 0);

				// ------------------------------------------------------------
				// 2) Collision avec bounding box (porte ouverte traversable)
				// ------------------------------------------------------------
				if (!blocked) {
					x = nextX;
					y = nextY;

					// Recalculer après déplacement
					row = y / TILE_SIZE;
					col = x / TILE_SIZE;

					printf("[LEAVING] MOVE OK -> row=%d col=%d TILE=%d\n",
						   row, col, (int)g.maze.tiles[row][col]);
				} else {
					printf("[LEAVING] MOVE BLOCKED\n");
				}

				// ------------------------------------------------------------
				// 3) Détermination générique de la case "dehors"
				// ------------------------------------------------------------
				int out_row = g.maze.ghost_door_row;
				int out_col = g.maze.ghost_door_col;

				if (g.maze.ghost_door_row < g.maze.ghost_center_row)
					out_row = g.maze.ghost_door_row - 1;   // porte au-dessus
				else if (g.maze.ghost_door_row > g.maze.ghost_center_row)
					out_row = g.maze.ghost_door_row + 1;   // porte en-dessous
				else if (g.maze.ghost_door_col < g.maze.ghost_center_col)
					out_col = g.maze.ghost_door_col - 1;   // porte à gauche
				else if (g.maze.ghost_door_col > g.maze.ghost_center_col)
					out_col = g.maze.ghost_door_col + 1;   // porte à droite

				// DEBUG : info sur la case de sortie
				printf("[LEAVING] OUT row=%d col=%d\n", out_row, out_col);

				// ------------------------------------------------------------
				// 4) Sortie effective : le fantôme devient Outside
				//    uniquement lorsqu'il a FRANCHI la porte.
				// ------------------------------------------------------------
				if (row == out_row && col == out_col)
				{
					printf("[LEAVING] EXIT HOUSE -> OUTSIDE\n");
					houseState = HouseState::Outside;

					mode = (g.global_mode == GlobalGhostMode::Scatter)
							? Mode::Scatter
							: Mode::Chase;
				}

				return;
			}


			// --------------------------------------------------------
			// Fantôme dehors : IA normale
			// --------------------------------------------------------
			case HouseState::Outside:
				break;
		}

		// ------------------------------------------------------------
		// 2) IA normale (Scatter / Chase / Frightened / Eaten)
		// ------------------------------------------------------------

		if (mode == Mode::Frightened) {
			if (isCentered) {
				dir = chooseRandom(g, row, col);
			}
		}
		else if (mode == Mode::Chase || mode == Mode::Scatter) {
			if (isCentered) {
				Dir newDir = dir;
				switch (id) {
					case 0: newDir = chooseDirectChase(g, row, col);   break;
					case 1: newDir = chooseInterceptor(g, row, col);   break;
					case 2: newDir = chooseIndirect(g, row, col);      break;
					case 3: newDir = chooseRandom(g, row, col);        break;
				}
				int nr = row + dirY(newDir);
				int nc = col + dirX(newDir);

				if (!is_blocking_for_ghost(g, nr, nc, false, houseState)) {
					dir = newDir;
				}
			}
		}
		else if (mode == Mode::Eaten) {
			if (isCentered) {
				if (g.maze.ghost_center_row >= 0 && g.maze.ghost_center_col >= 0) {
					if (path.empty()) {
						find_shortest_path_to_house(
							g.maze,
							row, col,
							g.maze.ghost_center_row,
							g.maze.ghost_center_col,
							path
						);
					}
					if (!path.empty()) {
						dir = path.front();
						path.erase(path.begin());
					}
				}
			}
		}

		// ------------------------------------------------------------
		// 3) Vitesse selon mode et tunnel
		// ------------------------------------------------------------
		int speed = GHOST_SPEED;
		TileType curr_t = g.maze.tiles[row][col];
		bool in_tunnel = (curr_t == TileType::Tunnel || curr_t == TileType::TunnelEntry);

		if (!is_eyes && in_tunnel) {
			speed = GHOST_SPEED / 2;
			if (speed <= 0) speed = 1;
		}
		else if (mode == Mode::Frightened) {
			speed = GHOST_SPEED / 2;
			if (speed <= 0) speed = 1;
		}

		int nextX = x + dirX(dir)*speed;
		int nextY = y + dirY(dir)*speed;

		// ------------------------------------------------------------
		// 4) Tunnel wrap
		// ------------------------------------------------------------
		int tmpX = nextX;
		int tmpY = nextY;
		if (try_tunnel_wrap(g, *this, tmpX, tmpY)) {
			x = tmpX;
			y = tmpY;
		}
		else if (!checkCollision(g, nextX, nextY, dir, is_eyes, houseState)) {
			x = nextX;
			y = nextY;
		}

		animTick++;
	}

	// ------------------ Draw -----------------

	void Ghost::draw() const {
		const uint16_t* anim[2];

		bool is_frightened = (mode == Mode::Frightened);
		bool is_eyes = (mode == Mode::Eaten);

		if (is_eyes) {
			// Sprite yeux à prévoir ; pour l'instant, on réutilise ghost_blue_0
			anim[0] = ghost_blue_0;
			anim[1] = ghost_blue_1;
		} else if (is_frightened) {
			anim[0] = ghost_blue_0;
			anim[1] = ghost_blue_1;
		} else {
			switch (id) {
				case 0: anim[0] = ghost_red_0;    anim[1] = ghost_red_1;    break;
				case 1: anim[0] = ghost_blue_0;   anim[1] = ghost_blue_1;   break;
				case 2: anim[0] = ghost_pink_0;   anim[1] = ghost_pink_1;   break;
				case 3: anim[0] = ghost_orange_0; anim[1] = ghost_orange_1; break;
				default: anim[0] = ghost_red_0;   anim[1] = ghost_red_1;    break;
			}
		}

		int frame = (animTick / 8) % 2;
		int screen_x = x;
		int screen_y = y - (int)g_camera_y;

		gfx_drawSprite(screen_x, screen_y, anim[frame], GHOST_SIZE, GHOST_SIZE);

		if (!is_eyes) {
			// Yeux dynamiques comme avant
			uint16_t eyeColor = COLOR_LIGHTBLUE;
			int eye1x, eye1y, eye2x, eye2y;

			switch (dir) {
				case Dir::Left:
					eye1x = screen_x + 1; eye1y = screen_y + 5;
					eye2x = screen_x + 7; eye2y = screen_y + 5;
					break;
				case Dir::Right:
					eye1x = screen_x + 3; eye1y = screen_y + 5;
					eye2x = screen_x + 9; eye2y = screen_y + 5;
					break;
				case Dir::Down:
					eye1x = screen_x + 2; eye1y = screen_y + 6;
					eye2x = screen_x + 8; eye2y = screen_y + 6;
					break;
				case Dir::Up:
					eye1x = screen_x + 2; eye1y = screen_y + 3;
					eye2x = screen_x + 8; eye2y = screen_y + 3;
					break;
				case Dir::None:
				default:
					eye1x = screen_x + 3; eye1y = screen_y + 5;
					eye2x = screen_x + 9; eye2y = screen_y + 5;
					break;
			}

			for (int dy = 0; dy < 2; ++dy) {
				for (int dx = 0; dx < 2; ++dx) {
					lcd_putpixel(eye1x + dx, eye1y + dy, eyeColor);
					lcd_putpixel(eye2x + dx, eye2y + dy, eyeColor);
				}
			}
		}
	}
