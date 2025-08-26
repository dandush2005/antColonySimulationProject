#ifndef VISUALIZATION_H
#define VISUALIZATION_H

// Charset selection API
typedef enum {
    CHARSET_AUTO = 0,
    CHARSET_UNICODE,
    CHARSET_ASCII
} RenderCharset;

void set_render_charset(RenderCharset mode);   // optional override
int  is_unicode_enabled(void);                 // 0 = ASCII, 1 = Unicode

#include "data_structures.h"

// ============================
// Render view state & dispatcher
// ============================
typedef enum {
    VIEW_WORLD = 0,
    VIEW_ANT_LIST = 1,   // optional, only if you keep this feature
    VIEW_MENU = 2
} RenderView;

void set_active_view(RenderView v);
RenderView get_active_view(void);

// Centralized, single entry point to draw exactly one frame per tick
void render_frame(const World* world);

// Instead of clearing/printing from random places, request a full redraw
void request_full_redraw(void);

// Border and symbol helpers
const char* get_border_tl(void);  // Top-left corner
const char* get_border_tr(void);  // Top-right corner
const char* get_border_bl(void);  // Bottom-left corner
const char* get_border_br(void);  // Bottom-right corner
const char* get_border_h(void);   // Horizontal line
const char* get_border_v(void);   // Vertical line
char get_ant_search_symbol(void); // Ant without food
char get_ant_carry_symbol(void);  // Ant with food
char get_wall_symbol(void);       // Wall/obstacle

// Console initialization and management
void init_console(void);
void cleanup_console(void);
void set_color(int color);
void clear_screen(void);
void hide_cursor(void);
void show_cursor(void);

// World rendering
void render_world(const World* world);
void render_cell(const Cell* cell, int x, int y, const World* world);
void render_ant(const Ant* ant, int x, int y);
void render_border(const World* world);

// Statistics and information display
void render_statistics(const World* world);
void render_colony_info(const Colony* colony, int row);
void render_legend(void);
void render_controls(void);

// Color management
int get_terrain_color(TerrainType terrain);
int get_colony_color(int colony_id);
// get_pheromone_color declaration moved to pheromones.h

// Symbol management
char get_terrain_symbol(TerrainType terrain);
char get_ant_symbol(const Ant* ant);

// Console positioning
void gotoxy(int x, int y);
void set_console_size(int width, int height);

// Force a full screen refresh
void force_screen_refresh(void);

// Console management

#endif // VISUALIZATION_H
