#include "ghost.h"
#include "assets/assets.h"
#include "core/graphics.h"
#include "core/sprite.h"
#include "maze.h"
#include "game/config.h"
#include "game/game.h"
#include <cstdlib>
#include <queue>
#include <algorithm>   // pour std::reverse
#include <cmath>       // pour std::abs

extern float g_camera_y;

static bool is_blocking_for_ghost(const GameState& g,
                                  int r, int c,
                                  bool is_eyes,
                                  Ghost::HouseState houseState);


// ------------------------------------------------------------
// Helpers direction (Ghost & Pacman)
// ------------------------------------------------------------

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

inline bool is_opposite(Ghost::Dir a, Ghost::Dir b) {
    return (a == Ghost::Dir::Left  && b == Ghost::Dir::Right) ||
           (a == Ghost::Dir::Right && b == Ghost::Dir::Left)  ||
           (a == Ghost::Dir::Up    && b == Ghost::Dir::Down)  ||
           (a == Ghost::Dir::Down  && b == Ghost::Dir::Up);
}

// ------------------------------------------------------------
// BFS pathfinding pour les yeux (Mode::Eaten)
// ------------------------------------------------------------

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
            // Reconstruction du chemin
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

            // Les yeux peuvent traverser tout sauf les murs
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

// ------------------------------------------------------------
// Collision solide pour les fantômes (entre eux & avec le décor)
// ------------------------------------------------------------

bool Ghost::wouldCollideWithOtherGhost(const GameState& g, int row, int col) const
{
    for (const auto& other : g.ghosts)
    {
        if (&other == this)
            continue;

        // Les yeux peuvent traverser tout le monde
        if (other.mode == Mode::Eaten)
            continue;

        // Fantômes dans la maison ou en Leaving → traversables
        if (other.houseState != HouseState::Outside)
            continue;

        int orow = other.y / TILE_SIZE;
        int ocol = other.x / TILE_SIZE;

        if (orow == row && ocol == col)
            return true;
    }
    return false;
}

std::vector<Ghost::Dir> Ghost::getValidDirections(const GameState& g, int row, int col, bool is_eyes) const
{
    std::vector<Dir> dirs;
    dirs.reserve(4);

    for (Dir d : {Dir::Up, Dir::Left, Dir::Down, Dir::Right})
    {
        int nr = row + dirY(d);
        int nc = col + dirX(d);

        // 1. Vérifier les murs / portes / maison selon l'état
        if (is_blocking_for_ghost(g, nr, nc, is_eyes, houseState))
            continue;

        // 2. Vérifier la présence d'autres fantômes (hors yeux)
        if (!is_eyes && wouldCollideWithOtherGhost(g, nr, nc))
            continue;

        dirs.push_back(d);
    }

    return dirs;
}

bool Ghost::collidesWithOtherGhost(const GameState& g, int nextRow, int nextCol) const
{
    for (const auto& other : g.ghosts)
    {
        if (&other == this)
            continue;

        if (other.mode == Mode::Eaten)
            continue;

        if (other.houseState != HouseState::Outside)
            continue;

        int orow = other.y / TILE_SIZE;
        int ocol = other.x / TILE_SIZE;

        if (orow == nextRow && ocol == nextCol)
            return true;
    }
    return false;
}

// ------------------------------------------------------------
// Décor bloquant pour les fantômes (murs, portes, maison)
// ------------------------------------------------------------

static bool is_blocking_for_ghost(const GameState& g,
                                  int r, int c,
                                  bool is_eyes,
                                  Ghost::HouseState houseState)
{
    TileType t = g.maze.tiles[r][c];

    // 1) Les yeux : bloqués uniquement par les murs
    if (is_eyes)
        return (t == TileType::Wall);

    // 2) Fantômes Inside : seuls les murs bloquent
    if (houseState == Ghost::HouseState::Inside)
        return (t == TileType::Wall);

    // 3) Fantômes Leaving : tout bloque sauf GhostDoorOpen
    if (houseState == Ghost::HouseState::Leaving)
    {
        if (t == TileType::Wall)
            return true;

        if (t == TileType::GhostDoorClosed ||
            t == TileType::GhostDoorOpening)
            return true;

        // GhostDoorOpen traversable
        return false;
    }

    // 4) Fantômes Outside : porte et maison bloquantes
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

// ------------------------------------------------------------
// Collision bounding box + collisions entre fantômes
// ------------------------------------------------------------

static bool checkCollision(const GameState& g,
                           int nextX, int nextY,
                           Ghost::Dir dir,
                           bool is_eyes,
                           Ghost::HouseState houseState,
                           const Ghost* selfGhost)
{
    // 1) Collision fantôme ↔ fantôme (hors yeux, uniquement Outside)
    if (houseState == Ghost::HouseState::Outside && !is_eyes)
    {
        int nextRow = nextY / TILE_SIZE;
        int nextCol = nextX / TILE_SIZE;

        for (const auto& other : g.ghosts)
        {
            if (&other == selfGhost)
                continue;

            if (other.mode == Ghost::Mode::Eaten)
                continue;

            if (other.houseState != Ghost::HouseState::Outside)
                continue;

            int orow = other.y / TILE_SIZE;
            int ocol = other.x / TILE_SIZE;

            if (orow == nextRow && ocol == nextCol)
                return true;
        }
    }

    // 2) Collision avec le décor (murs, portes, maison)
    switch (dir)
    {
        case Ghost::Dir::Right:
        {
            int right     = nextX + GHOST_SIZE - 1;
            int rowTop    = nextY / TILE_SIZE;
            int rowBottom = (nextY + GHOST_SIZE - 1) / TILE_SIZE;
            int colRight  = right / TILE_SIZE;

            return is_blocking_for_ghost(g, rowTop,    colRight, is_eyes, houseState) ||
                   is_blocking_for_ghost(g, rowBottom, colRight, is_eyes, houseState);
        }
        case Ghost::Dir::Left:
        {
            int left      = nextX;
            int rowTop    = nextY / TILE_SIZE;
            int rowBottom = (nextY + GHOST_SIZE - 1) / TILE_SIZE;
            int colLeft   = left / TILE_SIZE;

            return is_blocking_for_ghost(g, rowTop,    colLeft, is_eyes, houseState) ||
                   is_blocking_for_ghost(g, rowBottom, colLeft, is_eyes, houseState);
        }
        case Ghost::Dir::Down:
        {
            int bottom    = nextY + GHOST_SIZE - 1;
            int colLeft   = nextX / TILE_SIZE;
            int colRight  = (nextX + GHOST_SIZE - 1) / TILE_SIZE;
            int rowBottom = bottom / TILE_SIZE;

            return is_blocking_for_ghost(g, rowBottom, colLeft,  is_eyes, houseState) ||
                   is_blocking_for_ghost(g, rowBottom, colRight, is_eyes, houseState);
        }
        case Ghost::Dir::Up:
        {
            int top       = nextY;
            int colLeft   = nextX / TILE_SIZE;
            int colRight  = (nextX + GHOST_SIZE - 1) / TILE_SIZE;
            int rowTop    = top / TILE_SIZE;

            return is_blocking_for_ghost(g, rowTop, colLeft,  is_eyes, houseState) ||
                   is_blocking_for_ghost(g, rowTop, colRight, is_eyes, houseState);
        }
        default:
            return false;
    }
}

// ------------------------------------------------------------
// Mode helpers (frightened / reverse direction)
// ------------------------------------------------------------

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

// Appelé quand un power pellet est mangé
void Ghost::on_start_frightened()
{
    if (mode == Mode::Chase || mode == Mode::Scatter) {
        mode = Mode::Frightened;
		frightened_timer = FRIGHTENED_DURATION_TICKS;
        reverse_direction();  // arcade: demi-tour instantané
    }
}

// Appelé quand le mode frightened se termine
void Ghost::on_end_frightened()
{
    if (mode == Mode::Frightened) {
        mode = Mode::Chase;
        reverse_direction();
    }
}


// ------------------------------------------------------------
// Tunnel wrap (T/E)
// ------------------------------------------------------------

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

    // Entrée de tunnel : E -> T
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

        // Retrouver l'entrée de tunnel de sortie pour orienter le fantôme
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

// ------------------------------------------------------------
// Cibles Scatter / Chase par fantôme (arcade-like)
// ------------------------------------------------------------

// Coin de Scatter pour chaque fantôme
void Ghost::getScatterTarget(const GameState& g, int& tr, int& tc) const
{
    // On utilise simplement les coins du maze
    switch (id)
    {
        case 0: // Blinky - haut droite
            tr = 0;
            tc = MAZE_WIDTH - 1;
            break;
        case 1: // Pinky - haut gauche
            tr = 0;
            tc = 0;
            break;
        case 2: // Inky - bas droite
            tr = MAZE_HEIGHT - 1;
            tc = MAZE_WIDTH - 1;
            break;
        case 3: // Clyde - bas gauche
            tr = MAZE_HEIGHT - 1;
            tc = 0;
            break;
        default:
            tr = 0;
            tc = MAZE_WIDTH - 1;
            break;
    }

    // Clamp de sécurité
    if (tr < 0) tr = 0;
    if (tr >= MAZE_HEIGHT) tr = MAZE_HEIGHT - 1;
    if (tc < 0) tc = 0;
    if (tc >= MAZE_WIDTH) tc = MAZE_WIDTH - 1;

    (void)g; // pas utilisé pour l'instant
}

// Cible Chase pour chaque fantôme (Blinky / Pinky / Inky / Clyde)
void Ghost::getChaseTarget(const GameState& g, int& tr, int& tc) const
{
    int pacRow = g.pacman.y / TILE_SIZE;
    int pacCol = g.pacman.x / TILE_SIZE;

    switch (id)
    {
        case 0: // Blinky : target = Pac-Man direct
        {
            tr = pacRow;
            tc = pacCol;
            break;
        }

        case 1: // Pinky : 4 cases devant Pac-Man
        {
            int offsetR = 4 * pacDirY(g.pacman.dir);
            int offsetC = 4 * pacDirX(g.pacman.dir);
            tr = pacRow + offsetR;
            tc = pacCol + offsetC;
            break;
        }

        case 2: // Inky : vecteur depuis Blinky vers tile devant Pac-Man (simplifié)
        {
            const Ghost* blinky = nullptr;
            for (const auto& gh : g.ghosts)
            {
                if (gh.id == 0) { blinky = &gh; break; }
            }

            int aheadR = pacRow + 2 * pacDirY(g.pacman.dir);
            int aheadC = pacCol + 2 * pacDirX(g.pacman.dir);

            if (blinky)
            {
                int br = blinky->y / TILE_SIZE;
                int bc = blinky->x / TILE_SIZE;

                int vr = aheadR - br;
                int vc = aheadC - bc;

                tr = br + 2 * vr;
                tc = bc + 2 * vc;
            }
            else
            {
                tr = aheadR;
                tc = aheadC;
            }
            break;
        }

        case 3: // Clyde : s'il est loin, il chase ; s'il est proche, il scatter
        {
            int gr = y / TILE_SIZE;
            int gc = x / TILE_SIZE;

            int dist = std::abs(gr - pacRow) + std::abs(gc - pacCol);

            if (dist >= 8)
            {
                tr = pacRow;
                tc = pacCol;
            }
            else
            {
                // Proche => cible de Scatter (son coin)
                getScatterTarget(g, tr, tc);
            }
            break;
        }

        default:
        {
            tr = pacRow;
            tc = pacCol;
            break;
        }
    }

    // Clamp de sécurité
    if (tr < 0) tr = 0;
    if (tr >= MAZE_HEIGHT) tr = MAZE_HEIGHT - 1;
    if (tc < 0) tc = 0;
    if (tc >= MAZE_WIDTH) tc = MAZE_WIDTH - 1;
}

// Choix de direction générique vers une cible (arcade-like)
Ghost::Dir Ghost::chooseDirectionTowardsTarget(const GameState& g,
                                               int row, int col,
                                               int targetRow, int targetCol,
                                               bool is_eyes) const
{
    // Liste des directions possibles (sans mur, sans autres fantômes)
    auto valid = getValidDirections(g, row, col, is_eyes);
    if (valid.empty())
        return dir; // aucune option, on garde la direction actuelle

    // On essaie d'éviter le demi-tour si possible (arcade)
    std::vector<Dir> filtered;
    filtered.reserve(valid.size());
    for (Dir d : valid)
    {
        if (!is_opposite(d, dir))
            filtered.push_back(d);
    }
    if (filtered.empty())
        filtered = valid; // si aucune sans demi-tour, on autorise

    // Choisir la direction qui minimise la distance à la cible
    Dir best = filtered.front();
    int bestDist = 999999;

    for (Dir d : filtered)
    {
        int nr = row + dirY(d);
        int nc = col + dirX(d);
        int dist = std::abs(nr - targetRow) + std::abs(nc - targetCol);

        if (dist < bestDist)
        {
            best = d;
            bestDist = dist;
        }
    }

    return best;
}

// Choix de direction aléatoire (Frightened) avec filtre intelligent
Ghost::Dir Ghost::chooseRandomDir(const GameState& g, int row, int col, bool is_eyes) const
{
    auto valid = getValidDirections(g, row, col, is_eyes);
    if (valid.empty())
        return dir;

    // On essaie de ne pas faire demi-tour si on a d'autres options
    std::vector<Dir> filtered;
    filtered.reserve(valid.size());
    for (Dir d : valid)
    {
        if (!is_opposite(d, dir))
            filtered.push_back(d);
    }
    if (!filtered.empty())
        valid = filtered;

    return valid[std::rand() % valid.size()];
}

// ------------------------------------------------------------
// Update (logique principale fantôme)
// ------------------------------------------------------------

void Ghost::update(GameState& g)
{
    // Coordonnées de base
    int col = x / TILE_SIZE;
    int row = y / TILE_SIZE;

    int centerX   = x + GHOST_SIZE / 2;
    int centerY   = y + GHOST_SIZE / 2;
    int colCenter = col * TILE_SIZE + TILE_SIZE / 2;
    int rowCenter = row * TILE_SIZE + TILE_SIZE / 2;
    bool isCentered =
        (std::abs(centerX - colCenter) <= GHOST_SPEED) &&
        (std::abs(centerY - rowCenter) <= GHOST_SPEED);

    bool is_eyes = (mode == Mode::Eaten);

    // --------------------------------------------------------
    // 1) Gestion de la maison (Inside / Leaving / Outside)
    // --------------------------------------------------------

    switch (houseState)
    {
        // ----------------- Inside : rebond dans la maison -----------------
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
                !isHouse(rowTop,    colLeft)  ||
                !isHouse(rowTop,    colRight) ||
                !isHouse(rowBottom, colLeft)  ||
                !isHouse(rowBottom, colRight);

            if (blocked)
            {
                // Choisir une nouvelle direction interne
                Ghost::Dir dirs[4] = {Dir::Left, Dir::Right, Dir::Up, Dir::Down};
                for (int i = 0; i < 8; i++)
                {
                    Dir d = dirs[std::rand() % 4];
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
            else
            {
                x = tryX;
                y = tryY;
            }

            // Sortie autorisée si la porte est ouverte ET si c'est leur tour
            if (g.ghostDoorState == GameState::DoorState::Open &&
                g.elapsed_ticks >= releaseTime_ticks)
            {
                houseState = HouseState::Leaving;
            }

            animTick++;
            return;
        }

        // ----------------- Leaving : alignement + franchir la porte -----------------
        case HouseState::Leaving:
        {
            int speed = GHOST_SPEED;

            row = y / TILE_SIZE;
            col = x / TILE_SIZE;

            // Alignement vers la porte
            if (col < g.maze.ghost_door_col)
                dir = Dir::Right;
            else if (col > g.maze.ghost_door_col)
                dir = Dir::Left;
            else if (row < g.maze.ghost_door_row)
                dir = Dir::Down;
            else if (row > g.maze.ghost_door_row)
                dir = Dir::Up;

            int nextX = x + dirX(dir) * speed;
            int nextY = y + dirY(dir) * speed;

            bool blocked = checkCollision(g, nextX, nextY, dir, false, HouseState::Leaving, this);

            if (!blocked)
            {
                x = nextX;
                y = nextY;

                row = y / TILE_SIZE;
                col = x / TILE_SIZE;
            }

            // Déterminer la case "dehors" en fonction de la porte
            int out_row = g.maze.ghost_door_row;
            int out_col = g.maze.ghost_door_col;

            if (g.maze.ghost_door_row < g.maze.ghost_center_row)
                out_row = g.maze.ghost_door_row - 1;
            else if (g.maze.ghost_door_row > g.maze.ghost_center_row)
                out_row = g.maze.ghost_door_row + 1;
            else if (g.maze.ghost_door_col < g.maze.ghost_center_col)
                out_col = g.maze.ghost_door_col - 1;
            else if (g.maze.ghost_door_col > g.maze.ghost_center_col)
                out_col = g.maze.ghost_door_col + 1;

            // Quand la case dehors est atteinte → Outside
            if (row == out_row && col == out_col)
            {
                houseState = HouseState::Outside;

                // Mode initial : suivre le mode global
                mode = (g.global_mode == GlobalGhostMode::Scatter)
                       ? Mode::Scatter
                       : Mode::Chase;
            }

            animTick++;
            return;
        }

        case HouseState::Outside:
            // On laisse continuer vers l'IA normale
            break;
    }

    // --------------------------------------------------------
    // 2) IA normale (Outside)
    // --------------------------------------------------------

    // Vitesse de base
    int speed = GHOST_SPEED;
    TileType curr_t = g.maze.tiles[row][col];
    bool in_tunnel = (curr_t == TileType::Tunnel || curr_t == TileType::TunnelEntry);

    if (!is_eyes && in_tunnel)
    {
        speed = GHOST_SPEED / 2;
        if (speed <= 0) speed = 1;
    }
    else if (mode == Mode::Frightened)
    {
        speed = GHOST_SPEED / 2;
        if (speed <= 0) speed = 1;
		if (frightened_timer > 0)
		{
			frightened_timer--;

			if (frightened_timer == 0)
			{
				on_end_frightened();
			}
		}
    }

    // --- Choix de direction quand centré sur la grille ---
    if (isCentered)
    {
        if (mode == Mode::Eaten)
        {
            // Retour maison via BFS
            if (g.maze.ghost_center_row >= 0 && g.maze.ghost_center_col >= 0)
            {
                if (path.empty())
                {
                    find_shortest_path_to_house(
                        g.maze,
                        row, col,
                        g.maze.ghost_center_row,
                        g.maze.ghost_center_col,
                        path
                    );
                }
                if (!path.empty())
                {
                    dir = path.front();
                    path.erase(path.begin());
                }
            }
        }
        else if (mode == Mode::Frightened)
        {
            // Mouvement aléatoire, filtré (pas de demi-tour si possible)
            dir = chooseRandomDir(g, row, col, is_eyes);
        }
        else
        {
            // Scatter / Chase
            // On aligne d'abord le mode interne avec le global, hors frightened/eaten
            if (g.global_mode == GlobalGhostMode::Scatter)
                mode = Mode::Scatter;
            else
                mode = Mode::Chase;

            int targetRow = row;
            int targetCol = col;

            if (mode == Mode::Scatter)
                getScatterTarget(g, targetRow, targetCol);
            else // Mode::Chase
                getChaseTarget(g, targetRow, targetCol);

            dir = chooseDirectionTowardsTarget(g, row, col, targetRow, targetCol, is_eyes);
        }
    }

    // --------------------------------------------------------
    // 3) Mouvement + tunnel + fallback
    // --------------------------------------------------------

    int nextX = x + dirX(dir) * speed;
    int nextY = y + dirY(dir) * speed;

    int tmpX = nextX;
    int tmpY = nextY;

    // Tunnel wrap
    if (try_tunnel_wrap(g, *this, tmpX, tmpY))
    {
        x = tmpX;
        y = tmpY;
    }
    else
    {
        bool blocked = checkCollision(g, nextX, nextY, dir, is_eyes, houseState, this);

        if (!blocked)
        {
            x = nextX;
            y = nextY;
        }
        else
        {
            // Fallback : un fantôme Outside ne doit pas rester coincé s'il est centré
            if (houseState == HouseState::Outside && isCentered)
            {
                if (mode == Mode::Frightened)
                {
                    Dir newDir = chooseRandomDir(g, row, col, is_eyes);
                    int altX = x + dirX(newDir) * speed;
                    int altY = y + dirY(newDir) * speed;
                    if (!checkCollision(g, altX, altY, newDir, is_eyes, houseState, this))
                    {
                        dir = newDir;
                        x = altX;
                        y = altY;
                    }
                }
                else if (mode == Mode::Eaten)
                {
                    // On recalcule un pas BFS si nécessaire
                    if (path.empty() && g.maze.ghost_center_row >= 0 && g.maze.ghost_center_col >= 0)
                    {
                        find_shortest_path_to_house(
                            g.maze,
                            row, col,
                            g.maze.ghost_center_row,
                            g.maze.ghost_center_col,
                            path
                        );
                    }
                    if (!path.empty())
                    {
                        Dir newDir = path.front();
                        int altX = x + dirX(newDir) * speed;
                        int altY = y + dirY(newDir) * speed;
                        if (!checkCollision(g, altX, altY, newDir, is_eyes, houseState, this))
                        {
                            dir = newDir;
                            x = altX;
                            y = altY;
                            path.erase(path.begin());
                        }
                    }
                }
                else
                {
                    // Scatter / Chase : on tente une autre direction vers la même cible
                    int targetRow = row;
                    int targetCol = col;

                    if (mode == Mode::Scatter)
                        getScatterTarget(g, targetRow, targetCol);
                    else
                        getChaseTarget(g, targetRow, targetCol);

                    Dir newDir = chooseDirectionTowardsTarget(g, row, col, targetRow, targetCol, is_eyes);
                    int altX = x + dirX(newDir) * speed;
                    int altY = y + dirY(newDir) * speed;
                    if (!checkCollision(g, altX, altY, newDir, is_eyes, houseState, this))
                    {
                        dir = newDir;
                        x = altX;
                        y = altY;
                    }
                }
            }
        }
    }

    animTick++;
}

// ------------------------------------------------------------
// Draw
// ------------------------------------------------------------

void Ghost::draw() const
{
    const uint16_t* anim[2];

    bool is_frightened = (mode == Mode::Frightened);
    bool is_eyes       = (mode == Mode::Eaten);

    if (is_eyes)
    {
        anim[0] = ghost_blue_0;
        anim[1] = ghost_blue_1;
    }
    else if (is_frightened)
	{
		bool blinking = (frightened_timer < FRIGHTENED_BLINK_START_TICKS);

		if (blinking)
		{
			// Clignotement bleu ↔ blanc
			bool flash = ((animTick / 8) % 2) == 0;

			if (flash)
			{
				anim[0] = ghost_white_0;
				anim[1] = ghost_white_1;
			}
			else
			{
				anim[0] = ghost_scared_0;
				anim[1] = ghost_scared_1;
			}
		}
		else
		{
			// Frightened normal (bleu)
			anim[0] = ghost_scared_0;
			anim[1] = ghost_scared_1;
		}
	}
    else
    {
        switch (id)
        {
            case 0: anim[0] = ghost_red_0;    anim[1] = ghost_red_1;    break;
            case 1: anim[0] = ghost_blue_0;   anim[1] = ghost_blue_1;   break;
            case 2: anim[0] = ghost_pink_0;   anim[1] = ghost_pink_1;   break;
            case 3: anim[0] = ghost_orange_0; anim[1] = ghost_orange_1; break;
            default: anim[0] = ghost_red_0;   anim[1] = ghost_red_1;    break;
        }
    }
	
	int frame    = (animTick / 8) % 2;
    int screen_x = x;
    int screen_y = y - (int)g_camera_y;
	
	gfx_drawSprite(screen_x, screen_y, anim[frame], GHOST_SIZE, GHOST_SIZE);

	if (!is_frightened && !is_eyes)
	{
		const EyeOffset& off = eyeOffsets[(int)(dir)-1];

		const uint16_t* eyes = nullptr;
		switch (dir)
		{
			case Dir::Left:  eyes = ghost_eyes_left;  break;
			case Dir::Right: eyes = ghost_eyes_right; break;
			case Dir::Up:    eyes = ghost_eyes_up;    break;
			case Dir::Down:  eyes = ghost_eyes_down;  break;
			default:         eyes = ghost_eyes_left;  break;
		}

		gfx_drawSprite(screen_x + off.dx, screen_y + off.dy, eyes, 10, 5);
	}




}