#ifndef ANT_LOGIC_H
#define ANT_LOGIC_H

#include "data_structures.h"

// Ant creation and management
Ant* create_ant(int id, int colony_id, Position pos);
void destroy_ant(Ant* ant);
void add_ant_to_colony(Colony* colony, Ant* ant);
void remove_ant_from_colony(Colony* colony, Ant* ant);

// Ant movement
void move_ant(Ant* ant, World* world, int direction);
void move_randomly(Ant* ant, World* world);
void follow_pheromone_gradient(Ant* ant, World* world, int pheromone_type);

// Ant behavior
void update_ant(World* world, Ant* ant);
void decide_direction(Ant* ant, World* world);
void handle_food_interaction(Ant* ant, World* world);
void handle_nest_return(Ant* ant, World* world);

// Ant state management
void set_ant_state(Ant* ant, uint8_t state);
void clear_ant_state(Ant* ant, uint8_t state);
int has_ant_state(const Ant* ant, uint8_t state);
void toggle_ant_state(Ant* ant, uint8_t state);

// Colony ant management
void spawn_ant(World* world, int colony_id);
void cleanup_dead_ants(Colony* colony);
void update_all_ants(World* world);

// Path tracking
void add_path_node(Ant* ant, Position pos, float pheromone);
void clear_path_history(Ant* ant);

// Direction arrays (extern declarations)
extern const int dx[8];
extern const int dy[8];

#endif // ANT_LOGIC_H
