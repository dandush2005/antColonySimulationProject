#ifndef WORLD_H
#define WORLD_H

#include "data_structures.h"

// World creation and destruction
World* create_world(int width, int height, int colony_count);
void destroy_world(World* world);

// World manipulation
void place_colony(World* world, int colony_id, int x, int y);
void place_food(World* world, int x, int y, int amount);
void place_obstacle(World* world, int x, int y);
void clear_cell(World* world, int x, int y);

// World queries
int is_valid_position(const World* world, int x, int y);
int is_walkable(const World* world, int x, int y);
Cell* get_cell(const World* world, int x, int y);

// World initialization
void initialize_world_random(World* world);
void create_test_scenario(World* world);

// Colony management
void spawn_initial_ants(World* world);
void update_colony_statistics(World* world);
void spawn_ant(World* world, int colony_id);  // ADD THIS LINE!

#endif // WORLD_H
