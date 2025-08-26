#include "visualization.h"
#include "config.h"
#include "utils.h"
#include "pheromones.h"
#include "ant_logic.h"  // Needed for ant state constants
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>

// ---- Unified render state ----
static RenderView g_view = VIEW_WORLD;
static int g_full_redraw = 1; // draw the first frame fully

void set_active_view(RenderView v) { g_view = v; g_full_redraw = 1; }
RenderView get_active_view(void) { return g_view; }
void request_full_redraw(void)   { g_full_redraw = 1; }

// The ONLY function the main loop should call per tick
void render_frame(const World* world) {
    // Move cursor to top-left instead of clearing entire screen
    gotoxy(0, 0);

    switch (g_view) {
        case VIEW_WORLD:
            render_world(world);
            break;
        case VIEW_ANT_LIST:
            break;
        case VIEW_MENU:
            break;
    }

    g_full_redraw = 0;
    fflush(stdout);
}

// Unicode detection and charset management
static int g_unicode_enabled = 0;   // 0=ASCII, 1=Unicode
static RenderCharset g_forced_charset = CHARSET_AUTO;

static void detect_unicode_console(void) {
    if (g_forced_charset == CHARSET_ASCII) { g_unicode_enabled = 0; return; }
    if (g_forced_charset == CHARSET_UNICODE) { g_unicode_enabled = 1; return; }

    // Optional user override via env var
    char* force = getenv("ACO_FORCE_ASCII");
    if (force && (*force == '1' || *force == 'y' || *force == 'Y')) {
        g_unicode_enabled = 0;
        return;
    }

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0; 
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Switch output code page to UTF-8 (CP65001). If this fails, use ASCII.
    if (!SetConsoleOutputCP(CP_UTF8)) { 
        g_unicode_enabled = 0; 
        return; 
    }

    // Probe with a UTF-8 multi-byte box character. If it prints, assume OK.
    DWORD written = 0; 
    const char* probe = "╔"; // UTF-8 bytes
    BOOL ok = WriteConsoleA(h, probe, (DWORD)strlen(probe), &written, NULL);
    g_unicode_enabled = (ok && written > 0);
}

void set_render_charset(RenderCharset mode) { g_forced_charset = mode; }
int  is_unicode_enabled(void) { return g_unicode_enabled; }

// Centralized symbol helpers (static for internal use)
static const char* BX_TL(void) { return g_unicode_enabled ? "╔" : "+"; }
static const char* BX_TR(void) { return g_unicode_enabled ? "╗" : "+"; }
static const char* BX_BL(void) { return g_unicode_enabled ? "╚" : "+"; }
static const char* BX_BR(void) { return g_unicode_enabled ? "╝" : "+"; }
static const char* BX_H(void) { return g_unicode_enabled ? "═" : "-"; }
static const char* BX_V(void) { return g_unicode_enabled ? "║" : "|"; }

// Ant glyphs (static for internal use)
static char ANT_SEARCH(void) { return g_unicode_enabled ? '•' : 'o'; }
static char ANT_CARRY(void) { return g_unicode_enabled ? '●' : 'O'; }

// Wall block (static for internal use)
static char WALL_BLOCK(void) { return g_unicode_enabled ? '█' : '#'; }

// Public API functions for external use
const char* get_border_tl(void) { return BX_TL(); }
const char* get_border_tr(void) { return BX_TR(); }
const char* get_border_bl(void) { return BX_BL(); }
const char* get_border_br(void) { return g_unicode_enabled ? "╝" : "+"; }
const char* get_border_h(void) { return BX_H(); }
const char* get_border_v(void) { return BX_V(); }
char get_ant_search_symbol(void) { return ANT_SEARCH(); }
char get_ant_carry_symbol(void) { return ANT_CARRY(); }
char get_wall_symbol(void) { return WALL_BLOCK(); }

// Force a full screen refresh (useful for pause/resume)
void force_screen_refresh(void) {
    // This function is now a no-op since we always do full renders
    // Use request_full_redraw() instead for proper integration
}

// Console initialization and management
void init_console(void) {
    // Get console handle
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console == INVALID_HANDLE_VALUE) {
        print_error("Failed to get console handle");
        return;
    }
    
    // Set console size
    COORD size;
    size.X = 120;
    size.Y = 40;
    SetConsoleScreenBufferSize(console, size);
    
    // Set window size
    SMALL_RECT window;
    window.Left = 0;
    window.Top = 0;
    window.Right = 119;
    window.Bottom = 39;
    SetConsoleWindowInfo(console, TRUE, &window);
    
    // Detect Unicode support
    detect_unicode_console();
    
    // Hide cursor
    hide_cursor();
    
    // Clear screen
    clear_screen();
    
    print_info("Console initialized successfully");
}

void cleanup_console(void) {
    // Free screen buffer if it exists
    // This function is now a no-op since we're not using screen buffers
    
    // Reset state
    // This function is now a no-op since we're not using screen buffers
    
    show_cursor();
    clear_screen();
}

void set_color(int color) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(console, (WORD)color);
    }
}

void clear_screen(void) {
    system("cls");
}

void hide_cursor(void) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO cursor_info;
        GetConsoleCursorInfo(console, &cursor_info);
        cursor_info.bVisible = FALSE;
        SetConsoleCursorInfo(console, &cursor_info);
    }
}

void show_cursor(void) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO cursor_info;
        GetConsoleCursorInfo(console, &cursor_info);
        cursor_info.bVisible = TRUE;
        SetConsoleCursorInfo(console, &cursor_info);
    }
}

// World rendering
void render_world(const World* world) {
    if (world == NULL) return;

    set_color(COLOR_WHITE);

    // Top border (uses Unicode/ASCII helpers if present in file)
    printf("%s", BX_TL());
    for (int x = 0; x < world->width; ++x) printf("%s", BX_H());
    printf("%s\n", BX_TR());

    // --- Build temp grid for symbols only (colors applied at print time) ---
    const int W = world->width, H = world->height;
    char *grid = (char*)safe_malloc((size_t)W * (size_t)H);
    if (!grid) return; // fail-safe

    // 1) Terrain/pheromones baseline
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            Cell* cell = &world->grid[y][x];
            char symbol = ' ';
            switch (cell->terrain) {
                case TERRAIN_EMPTY: {
                    float f = cell->pheromone_food;
                    float h = cell->pheromone_home;
                    float m = (f > h) ? f : h;
                    symbol = (m > 0.0f) ? get_pheromone_symbol(m) : ' ';
                } break;
                case TERRAIN_WALL:  symbol = WALL_BLOCK(); break;
                case TERRAIN_FOOD:  symbol = 'F'; break;
                case TERRAIN_NEST:  symbol = 'N'; break;
                case TERRAIN_WATER: symbol = '~'; break;
            }
            grid[y*W + x] = symbol;
        }
    }

    // 2) Overlay ants
    for (int c = 0; c < world->colony_count; ++c) {
        Ant* ant = world->colonies[c].ants_head;
        while (ant) {
            if (!(ant->state & ANT_STATE_DEAD)) {
                int x = ant->pos.x, y = ant->pos.y;
                if (x >= 0 && x < W && y >= 0 && y < H) {
                    grid[y*W + x] = (ant->food_carrying > 0) ? ANT_CARRY() : ANT_SEARCH();
                }
            }
            ant = ant->next;
        }
    }

    // 3) Print map row-by-row with left/right borders and per-cell colors
    for (int y = 0; y < H; ++y) {
        printf("%s", BX_V());
        for (int x = 0; x < W; ++x) {
            Cell* cell = &world->grid[y][x];
            char ch = grid[y*W + x];
            int color = COLOR_WHITE;
            // derive color similar to old path
            switch (cell->terrain) {
                case TERRAIN_EMPTY: {
                    float f = cell->pheromone_food, h = cell->pheromone_home;
                    float m = (f > h) ? f : h;
                    color = (m > 0.0f) ? get_pheromone_color(m) : COLOR_BLACK;
                } break;
                case TERRAIN_WALL:  color = COLOR_BLACK; break;
                case TERRAIN_FOOD:  color = COLOR_BRIGHT_GREEN; break;
                case TERRAIN_NEST:  color = get_colony_color(cell->colony_id); break;
                case TERRAIN_WATER: color = COLOR_BLUE; break;
            }

            // If an ant is present, color by colony
            if (ch == ANT_SEARCH() || ch == ANT_CARRY()) {
                // colony color based on nest pheromone dominance heuristic
                // (optional; if you track ant per cell you can refine)
                color = COLOR_BRIGHT_RED; // safe, visible default
            }

            set_color(color);
            printf("%c", ch);
        }
        set_color(COLOR_WHITE);
        printf("%s\n", BX_V());
    }

    // Bottom border
    printf("%s", BX_BL());
    for (int x = 0; x < W; ++x) printf("%s", BX_H());
    printf("%s\n", BX_BR());

    safe_free(grid);

    // Compact stats/legend/controls printed **below** the map (no positioning)
    set_color(COLOR_WHITE);
    printf("\nSIMULATION STATISTICS                           \n");
    printf("Step: %-8d Status: %-15s                    \n", 
           world->current_step, world->paused ? "PAUSED" : "RUNNING");
    printf("Colonies: %-5d                                    \n", world->colony_count);
    for (int i = 0; i < world->colony_count; ++i) {
        const Colony* col = &world->colonies[i];
        set_color(get_colony_color(col->id));
        printf("Colony %d  ", col->id);
        set_color(COLOR_WHITE);
        printf("Food: %-4d Ants: %-2d/%-2d Eff: %-6.2f            \n",
               col->food_collected, col->active_ants, col->total_ants, col->efficiency_score);
    }

    printf("                                                        \n");
    printf("LEGEND  F=Food N=Nest %c/%c=Ant %c=Wall                \n",
           ANT_SEARCH(), ANT_CARRY(), WALL_BLOCK());
    printf("CONTROLS SPACE=Pause S=Save Q=Quit +/-=Speed           \n");
    printf("                                                        \n");

    set_color(COLOR_WHITE);
    
    // Push cursor below display area to prevent message overflow
    printf("\n\n\n\n\n");
    fflush(stdout);
}

void render_ant(const Ant* ant, int x, int y) {
    if (ant == NULL) return;
    
    char symbol = get_ant_symbol(ant);
    int color = get_colony_color(ant->colony_id);
    
    set_color(color);
    printf("%c", symbol);
}

// Statistics and information display
void render_statistics(const World* world) {
    if (world == NULL) return;
    
    set_color(COLOR_WHITE);
    printf("\n");
    printf("%s", BX_TL());
    for (int i = 0; i < 78; i++) printf("%s", BX_H());
    printf("%s\n", BX_TR());
    printf("%s                            SIMULATION STATISTICS                           %s\n", BX_V(), BX_V());
    printf("%s", BX_V());
    for (int i = 0; i < 78; i++) printf("%s", BX_H());
    printf("%s\n", BX_V());
    printf("%s Step: %-6d | Status: %-8s | Delay: %-4d ms                              %s\n", 
           BX_V(), world->current_step, 
           world->paused ? "PAUSED" : "RUNNING",
           world->render_delay_ms, BX_V());
    
    // Render colony information
    for (int i = 0; i < world->colony_count; i++) {
        render_colony_info(&world->colonies[i], i);
    }
    
    printf("%s", BX_BL());
    for (int i = 0; i < 78; i++) printf("%s", BX_H());
    printf("%s\n", BX_BR());
}

void render_colony_info(const Colony* colony, int row) {
    if (colony == NULL) return;
    
    set_color(get_colony_color(colony->id));
    printf("%s Colony %d: Food: %-4d | Ants: %-2d/%-2d | Efficiency: %-6.2f | Color: ", 
           BX_V(), colony->id, 
           colony->food_collected, 
           colony->active_ants, 
           colony->total_ants,
           colony->efficiency_score);
    
    // Show color sample
    set_color(colony->color);
    printf("%s", g_unicode_enabled ? "██" : "##");
    set_color(COLOR_WHITE);
    printf("                    %s\n", BX_V());
}

void render_legend(void) {
    printf("\n");
    printf("LEGEND:\n");
    printf("N = Nest (colony home)  %c = Ant without food  %c = Ant carrying food\n", ANT_SEARCH(), ANT_CARRY());
    printf("F = Food source         %c = Wall/Obstacle     .:*# = Pheromone intensity\n", WALL_BLOCK());
    printf("Colors: Different colonies have different colors\n");
}

void render_controls(void) {
    printf("\n");
    printf("CONTROLS:\n");
    printf("SPACE = Pause/Resume  S = Save  L = Load  Q = Quit  +/- = Speed  R = Reset\n");
}

// Color management
int get_terrain_color(TerrainType terrain) {
    switch (terrain) {
        case TERRAIN_EMPTY: return COLOR_WHITE;
        case TERRAIN_WALL: return COLOR_BRIGHT_WHITE;
        case TERRAIN_FOOD: return COLOR_BRIGHT_GREEN;
        case TERRAIN_NEST: return COLOR_BRIGHT_YELLOW;
        case TERRAIN_WATER: return COLOR_BRIGHT_CYAN;
        default: return COLOR_WHITE;
    }
}

int get_colony_color(int colony_id) {
    // Return different colors for different colonies
    switch (colony_id % 8) {
        case 0: return COLOR_BRIGHT_RED;
        case 1: return COLOR_BRIGHT_BLUE;
        case 2: return COLOR_BRIGHT_GREEN;
        case 3: return COLOR_BRIGHT_MAGENTA;
        case 4: return COLOR_BRIGHT_CYAN;
        case 5: return COLOR_BRIGHT_YELLOW;
        case 6: return COLOR_BRIGHT_WHITE;
        case 7: return COLOR_BRIGHT_RED;
        default: return COLOR_WHITE;
    }
}

// get_pheromone_color function moved to pheromones.c to avoid duplicate definition

// Symbol management
char get_terrain_symbol(TerrainType terrain) {
    switch (terrain) {
        case TERRAIN_EMPTY: return ' ';
        case TERRAIN_WALL: 
            return WALL_BLOCK();
        case TERRAIN_FOOD: return 'F';
        case TERRAIN_NEST: return 'N';
        case TERRAIN_WATER: return '~';
        default: return '?';
    }
}

char get_ant_symbol(const Ant* ant) {
    if (ant == NULL) return '?';
    
    if (ant->food_carrying > 0) {
        return ANT_CARRY(); // Ant carrying food
    } else {
        return ANT_SEARCH(); // Ant without food
    }
}

// Console positioning
void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void set_console_size(int width, int height) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != INVALID_HANDLE_VALUE) {
        COORD size;
        size.X = width;
        size.Y = height;
        SetConsoleScreenBufferSize(console, size);
        
        SMALL_RECT window;
        window.Left = 0;
        window.Top = 0;
        window.Right = width - 1;
        window.Bottom = height - 1;
        SetConsoleWindowInfo(console, TRUE, &window);
    }
}
