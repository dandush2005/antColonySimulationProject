#include "ant_logic.h"
#include "config.h"
#include "utils.h"
#include "pheromones.h"
#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Direction arrays for 8-directional movement
const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
const int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

// Ant creation and management
Ant* create_ant(int id, int colony_id, Position pos) {
    Ant* ant = (Ant*)safe_malloc(sizeof(Ant));
    if (ant == NULL) {
        return NULL;
    }
    
    // Initialize ant properties
    ant->id = id;
    ant->pos = pos;
    ant->last_pos = pos;
    ant->state = ANT_STATE_SEARCHING;  // Start searching for food
    ant->colony_id = colony_id;
    ant->energy = ANT_INITIAL_ENERGY;
    ant->food_carrying = 0;
    ant->steps_taken = 0;
    ant->food_delivered = 0;
    ant->preferred_direction = -1;  // No preferred direction initially
    ant->next = NULL;
    ant->path_history = NULL;
    
    print_info("Ant %d created for colony %d at (%d, %d)", id, colony_id, pos.x, pos.y);
    return ant;
}

void destroy_ant(Ant* ant) {
    if (ant == NULL) return;
    
    // Clear path history
    clear_path_history(ant);
    
    // Free the ant
    safe_free(ant);
}

void add_ant_to_colony(Colony* colony, Ant* ant) {
    if (colony == NULL || ant == NULL) return;
    
    // Add to front of linked list
    ant->next = colony->ants_head;
    colony->ants_head = ant;
    
    colony->total_ants++;
    colony->active_ants++;
    
    print_info("Ant %d added to colony %d", ant->id, colony->id);
}

void remove_ant_from_colony(Colony* colony, Ant* ant) {
    if (colony == NULL || ant == NULL) return;
    
    // Find and remove from linked list
    Ant** current = &colony->ants_head;
    while (*current != NULL && *current != ant) {
        current = &(*current)->next;
    }
    
    if (*current != NULL) {
        *current = ant->next;
        colony->active_ants--;
        print_info("Ant %d removed from colony %d", ant->id, colony->id);
    }
}

// Ant movement
void move_ant(Ant* ant, World* world, int direction) {
    if (ant == NULL || world == NULL || direction < 0 || direction >= 8) {
        return;
    }
    
    // Store last position
    ant->last_pos = ant->pos;
    
    // Calculate new position
    int new_x = ant->pos.x + dx[direction];
    int new_y = ant->pos.y + dy[direction];
    
    // Check if new position is valid and walkable
    if (is_valid_position(world, new_x, new_y) && is_walkable(world, new_x, new_y)) {
        ant->pos.x = new_x;
        ant->pos.y = new_y;
        ant->steps_taken++;
        
        // Add to path history
        add_path_node(ant, ant->pos, 0.0f);
        
        LOG_ANT_INFO("Ant %d moved to (%d, %d)", ant->id, new_x, new_y);
    } else {
        print_warning("Ant %d cannot move to (%d, %d)", new_x, new_y);
    }
}

void move_randomly(Ant* ant, World* world) {
    if (ant == NULL || world == NULL) return;
    
    // Try random directions until we find a valid one
    int attempts = 0;
    const int max_attempts = 10;
    
    while (attempts < max_attempts) {
        int direction = random_int(0, 7);
        int new_x = ant->pos.x + dx[direction];
        int new_y = ant->pos.y + dy[direction];
        
        if (is_valid_position(world, new_x, new_y) && is_walkable(world, new_x, new_y)) {
            move_ant(ant, world, direction);
            return;
        }
        attempts++;
    }
    
    // If no valid direction found, don't move
    LOG_ANT_INFO("Ant %d could not find valid random direction", ant->id);
}

void follow_pheromone_gradient(Ant* ant, World* world, int pheromone_type) {
    if (ant == NULL || world == NULL) return;
    
    float max_pheromone = 0.0f;
    int best_direction = -1;
    
    // Check all 8 neighboring cells
    for (int dir = 0; dir < 8; dir++) {
        int new_x = ant->pos.x + dx[dir];
        int new_y = ant->pos.y + dy[dir];
        
        if (is_valid_position(world, new_x, new_y) && is_walkable(world, new_x, new_y)) {
            float pheromone = get_pheromone_intensity(world, new_x, new_y, pheromone_type);
            if (pheromone > max_pheromone) {
                max_pheromone = pheromone;
                best_direction = dir;
            }
        }
    }
    
    // Move to best direction if pheromone found
    if (best_direction >= 0 && max_pheromone > 0.0f) {
        move_ant(ant, world, best_direction);
        LOG_ANT_INFO("Ant %d following pheromone gradient (type %d, strength %.1f)", 
                     ant->id, pheromone_type, max_pheromone);
    } else {
        // No pheromone trail found, move randomly
        move_randomly(ant, world);
    }
}

// Ant behavior
void update_ant(World* world, Ant* ant) {
    if (world == NULL || ant == NULL) return;
    
    // Decrease energy
    ant->energy -= ANT_ENERGY_PER_STEP;
    
    // Check if ant died
    if (ant->energy <= 0) {
        set_ant_state(ant, ANT_STATE_DEAD);
        print_info("Ant %d died from exhaustion", ant->id);
        return;
    }
    
    // Get current cell
    Cell* current_cell = get_cell(world, ant->pos.x, ant->pos.y);
    
    // Handle current state
    if (ant->state & ANT_STATE_SEARCHING) {
        // Check for food at current position FIRST
        if (current_cell && current_cell->terrain == TERRAIN_FOOD && 
            current_cell->food_amount > 0 && ant->food_carrying == 0) {
            
            // Pick up food
            ant->food_carrying = 1;
            current_cell->food_amount--;
            
            // Change state to returning
            clear_ant_state(ant, ANT_STATE_SEARCHING);
            set_ant_state(ant, ANT_STATE_RETURNING | ANT_STATE_CARRYING);
            
            // Boost energy
            ant->energy += ANT_ENERGY_FROM_FOOD;
            
            // Don't swap positions immediately - just reverse direction for next move
            // Store the direction we came from so we can go back
            int reverse_direction = -1;
            for (int dir = 0; dir < 8; dir++) {
                if (ant->last_pos.x == ant->pos.x + dx[dir] && 
                    ant->last_pos.y == ant->pos.y + dy[dir]) {
                    reverse_direction = dir;
                    break;
                }
            }
            
            // If we found the reverse direction, store it for next movement
            if (reverse_direction != -1) {
                // We'll use this in the next movement decision
                ant->preferred_direction = reverse_direction;
            }
            
            LOG_ANT_INFO("Ant %d picked up food at (%d, %d)", 
                         ant->id, ant->last_pos.x, ant->last_pos.y);
            
            // If food depleted, clear the cell
            if (current_cell->food_amount <= 0) {
                current_cell->terrain = TERRAIN_EMPTY;
            }
            
            // DON'T MOVE THIS TURN - just deposit pheromone
            deposit_pheromone(world, ant);
            return;  // Exit early
        }
        
        // No food here, so search for it
        if (random_probability() < FOLLOW_PHEROMONE_PROBABILITY) {
            follow_pheromone_gradient(ant, world, PHEROMONE_TYPE_FOOD);
        } else {
            move_randomly(ant, world);
        }
        
        // Deposit home pheromone while searching
        deposit_pheromone(world, ant);
        
    } else if (ant->state & ANT_STATE_RETURNING) {
        // Check if at nest
        if (current_cell && current_cell->terrain == TERRAIN_NEST && 
            current_cell->colony_id == ant->colony_id && ant->food_carrying > 0) {
            
            // Deliver food
            Colony* colony = &world->colonies[ant->colony_id];
            colony->food_collected++;
            ant->food_delivered++;
            ant->food_carrying = 0;
            
            // Change state back to searching
            clear_ant_state(ant, ANT_STATE_RETURNING | ANT_STATE_CARRYING);
            set_ant_state(ant, ANT_STATE_SEARCHING);
            
            LOG_ANT_INFO("Ant %d delivered food to colony %d", ant->id, ant->colony_id);
            
            // Set direction to go back where we came from
            int reverse_direction = -1;
            for (int dir = 0; dir < 8; dir++) {
                if (ant->last_pos.x == ant->pos.x + dx[dir] && 
                    ant->last_pos.y == ant->pos.y + dy[dir]) {
                    reverse_direction = dir;
                    break;
                }
            }
            
            if (reverse_direction != -1) {
                ant->preferred_direction = reverse_direction;
            }
            
            // Don't move this turn
            return;
        }
        
        // Not at nest, keep returning
        follow_pheromone_gradient(ant, world, PHEROMONE_TYPE_HOME);
        
        // Deposit food pheromone while returning
        deposit_pheromone(world, ant);
    }
    
    // Check if ant is tired
    if (ant->energy < ANT_INITIAL_ENERGY * 0.2f) {
        set_ant_state(ant, ANT_STATE_TIRED);
    }
}

void decide_direction(Ant* ant, World* world) {
    if (ant == NULL || world == NULL) return;
    
    // If ant has a preferred direction (like returning from food), use it
    if (ant->preferred_direction >= 0 && ant->preferred_direction < 8) {
        move_ant(ant, world, ant->preferred_direction);
        ant->preferred_direction = -1; // Clear preference after use
        return;
    }
    
    if (ant->state & ANT_STATE_SEARCHING) {
        // Looking for food
        if (random_probability() < FOLLOW_PHEROMONE_PROBABILITY) {
            follow_pheromone_gradient(ant, world, PHEROMONE_TYPE_FOOD);
        } else {
            move_randomly(ant, world);
        }
    } else if (ant->state & ANT_STATE_RETURNING) {
        // Returning with food
        follow_pheromone_gradient(ant, world, PHEROMONE_TYPE_HOME);
    } else {
        // Default to random movement
        move_randomly(ant, world);
    }
}

void handle_food_interaction(Ant* ant, World* world) {
    if (ant == NULL || world == NULL) return;
    
    Cell* cell = get_cell(world, ant->pos.x, ant->pos.y);
    if (cell == NULL) return;
    
    if (cell->terrain == TERRAIN_FOOD && cell->food_amount > 0 && ant->food_carrying == 0) {
        // Pick up food
        ant->food_carrying = 1;
        cell->food_amount--;
        
        // Change state to returning
        clear_ant_state(ant, ANT_STATE_SEARCHING);
        set_ant_state(ant, ANT_STATE_RETURNING);
        
        // Boost energy
        ant->energy += ANT_ENERGY_FROM_FOOD;
        
        print_info("Ant %d picked up food at (%d, %d)", ant->id, ant->pos.x, ant->pos.y);
        
        // If food is depleted, clear the cell
        if (cell->food_amount <= 0) {
            cell->terrain = TERRAIN_EMPTY;
        }
    }
}

void handle_nest_return(Ant* ant, World* world) {
    if (ant == NULL || world == NULL) return;
    
    Cell* cell = get_cell(world, ant->pos.x, ant->pos.y);
    if (cell == NULL) return;
    
    if (cell->terrain == TERRAIN_NEST && cell->colony_id == ant->colony_id && ant->food_carrying > 0) {
        // Deliver food to nest
        Colony* colony = &world->colonies[ant->colony_id];
        colony->food_collected += ant->food_carrying;
        ant->food_delivered += ant->food_carrying;
        ant->food_carrying = 0;
        
        // Change state back to searching
        clear_ant_state(ant, ANT_STATE_RETURNING);
        set_ant_state(ant, ANT_STATE_SEARCHING);
        
        print_info("Ant %d delivered food to colony %d nest", ant->id, ant->colony_id);
    }
}

// Ant state management
void set_ant_state(Ant* ant, uint8_t state) {
    if (ant == NULL) return;
    ant->state |= state;
}

void clear_ant_state(Ant* ant, uint8_t state) {
    if (ant == NULL) return;
    ant->state &= ~state;
}

int has_ant_state(const Ant* ant, uint8_t state) {
    if (ant == NULL) return 0;
    return (ant->state & state) != 0;
}

void toggle_ant_state(Ant* ant, uint8_t state) {
    if (ant == NULL) return;
    ant->state ^= state;
}

// Colony ant management
void spawn_ant(World* world, int colony_id) {
    if (world == NULL || colony_id < 0 || colony_id >= world->colony_count) {
        return;
    }
    
    Colony* colony = &world->colonies[colony_id];
    
    // Check if we can spawn more ants
    if (colony->total_ants >= MAX_ANTS_PER_COLONY) {
        print_warning("Colony %d at maximum ant capacity", colony_id);
        return;
    }
    
    // Create ant at nest position
    Ant* ant = create_ant(colony->total_ants + 1, colony_id, colony->nest_pos);
    if (ant != NULL) {
        add_ant_to_colony(colony, ant);
    }
}

void cleanup_dead_ants(Colony* colony) {
    if (colony == NULL) return;
    
    Ant** current = &colony->ants_head;
    int removed_count = 0;
    
    while (*current != NULL) {
        if ((*current)->state & ANT_STATE_DEAD) {
            Ant* dead = *current;
            *current = (*current)->next;
            
            // Update colony statistics
            colony->total_ants--;
            colony->active_ants--;
            
            // Destroy the dead ant
            destroy_ant(dead);
            removed_count++;
        } else {
            current = &(*current)->next;
        }
    }
    
    if (removed_count > 0) {
        print_info("Colony %d: %d dead ants removed", colony->id, removed_count);
    }
}

void update_all_ants(World* world) {
    if (world == NULL) return;
    
    for (int i = 0; i < world->colony_count; i++) {
        Colony* colony = &world->colonies[i];
        Ant* current = colony->ants_head;
        
        while (current != NULL) {
            Ant* next = current->next;  // Store next before updating
            
            if (!(current->state & ANT_STATE_DEAD)) {
                update_ant(world, current);
            }
            
            current = next;
        }
        
        // Clean up dead ants after updating all
        cleanup_dead_ants(colony);
    }
}

// Path tracking
void add_path_node(Ant* ant, Position pos, float pheromone) {
    if (ant == NULL) return;
    
    PathNode* node = (PathNode*)safe_malloc(sizeof(PathNode));
    if (node == NULL) return;
    
    node->pos = pos;
    node->pheromone_strength = pheromone;
    node->next = ant->path_history;
    ant->path_history = node;
}

void clear_path_history(Ant* ant) {
    if (ant == NULL) return;
    
    PathNode* current = ant->path_history;
    while (current != NULL) {
        PathNode* next = current->next;
        safe_free(current);
        current = next;
    }
    ant->path_history = NULL;
}
