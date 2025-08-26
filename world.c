#include "world.h"
#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// World creation and destruction
World* create_world(int width, int height, int colony_count) {
    if (width <= 0 || height <= 0 || colony_count <= 0) {
        print_error("Invalid world parameters");
        return NULL;
    }
    
    if (width > MAX_WORLD_SIZE || height > MAX_WORLD_SIZE) {
        print_error("World size exceeds maximum allowed");
        return NULL;
    }
    
    // Allocate world struct
    World* world = (World*)safe_malloc(sizeof(World));
    if (world == NULL) {
        return NULL;
    }
    
    // Initialize world properties
    world->width = width;
    world->height = height;
    world->colony_count = colony_count;
    world->current_step = 0;
    world->is_running = 0;
    world->paused = 0;
    world->render_delay_ms = RENDER_DELAY_MS;
    
    // Allocate colonies array
    world->colonies = (Colony*)safe_calloc(colony_count, sizeof(Colony));
    if (world->colonies == NULL) {
        safe_free(world);
        return NULL;
    }
    
    // Initialize colonies
    for (int i = 0; i < colony_count; i++) {
        world->colonies[i].id = i;
        world->colonies[i].food_collected = 0;
        world->colonies[i].total_ants = 0;
        world->colonies[i].active_ants = 0;
        world->colonies[i].ants_head = NULL;
        world->colonies[i].efficiency_score = 0.0f;
        world->colonies[i].color = i + 1; // Different color for each colony
    }
    
    // Allocate 2D grid
    world->grid = (Cell**)safe_malloc(height * sizeof(Cell*));
    if (world->grid == NULL) {
        safe_free(world->colonies);
        safe_free(world);
        return NULL;
    }
    
    // Allocate each row
    for (int i = 0; i < height; i++) {
        world->grid[i] = (Cell*)safe_calloc(width, sizeof(Cell));
        if (world->grid[i] == NULL) {
            // Clean up already allocated rows
            for (int j = 0; j < i; j++) {
                safe_free(world->grid[j]);
            }
            safe_free(world->grid);
            safe_free(world->colonies);
            safe_free(world);
            return NULL;
        }
        
        // Initialize all cells to empty
        for (int j = 0; j < width; j++) {
            world->grid[i][j].terrain = TERRAIN_EMPTY;
            world->grid[i][j].pheromone_food = PHEROMONE_INITIAL;
            world->grid[i][j].pheromone_home = PHEROMONE_INITIAL;
            world->grid[i][j].food_amount = 0;
            world->grid[i][j].colony_id = -1;
        }
    }
    
    print_info("World created successfully");
    return world;
}

void destroy_world(World* world) {
    if (world == NULL) return;
    
    // Free all ants in all colonies
    for (int i = 0; i < world->colony_count; i++) {
        Colony* colony = &world->colonies[i];
        Ant* current = colony->ants_head;
        while (current != NULL) {
            Ant* next = current->next;
            safe_free(current);
            current = next;
        }
    }
    
    // Free grid rows
    if (world->grid != NULL) {
        for (int i = 0; i < world->height; i++) {
            safe_free(world->grid[i]);
        }
        safe_free(world->grid);
    }
    
    // Free colonies array
    safe_free(world->colonies);
    
    // Free world struct
    safe_free(world);
    
    print_info("World destroyed successfully");
}

// World manipulation
void place_colony(World* world, int colony_id, int x, int y) {
    if (world == NULL || colony_id < 0 || colony_id >= world->colony_count) {
        print_error("Invalid colony placement parameters");
        return;
    }
    
    if (!is_valid_position(world, x, y)) {
        print_error("Invalid position for colony placement");
        return;
    }
    
    // Check if position is already occupied
    if (world->grid[y][x].terrain != TERRAIN_EMPTY) {
        print_warning("Position already occupied, clearing first");
        clear_cell(world, x, y);
    }
    
    // Place colony
    world->grid[y][x].terrain = TERRAIN_NEST;
    world->grid[y][x].colony_id = colony_id;
    
    // Update colony position
    world->colonies[colony_id].nest_pos.x = x;
    world->colonies[colony_id].nest_pos.y = y;
    
    print_info("Colony %d placed at (%d, %d)", colony_id, x, y);
}

void place_food(World* world, int x, int y, int amount) {
    if (world == NULL || amount <= 0) {
        print_error("Invalid food placement parameters");
        return;
    }
    
    if (!is_valid_position(world, x, y)) {
        print_error("Invalid position for food placement");
        return;
    }
    
    // Check if position is already occupied
    if (world->grid[y][x].terrain != TERRAIN_EMPTY) {
        print_warning("Position already occupied, clearing first");
        clear_cell(world, x, y);
    }
    
    // Place food
    world->grid[y][x].terrain = TERRAIN_FOOD;
    world->grid[y][x].food_amount = amount;
    
    print_info("Food placed at (%d, %d) with amount %d", x, y, amount);
}

void place_obstacle(World* world, int x, int y) {
    if (world == NULL) {
        print_error("Invalid world parameter");
        return;
    }
    
    if (!is_valid_position(world, x, y)) {
        print_error("Invalid position for obstacle placement");
        return;
    }
    
    // Check if position is already occupied
    if (world->grid[y][x].terrain != TERRAIN_EMPTY) {
        print_warning("Position already occupied, clearing first");
        clear_cell(world, x, y);
    }
    
    // Place obstacle
    world->grid[y][x].terrain = TERRAIN_WALL;
    
    print_info("Obstacle placed at (%d, %d)", x, y);
}

void clear_cell(World* world, int x, int y) {
    if (world == NULL || !is_valid_position(world, x, y)) {
        return;
    }
    
    world->grid[y][x].terrain = TERRAIN_EMPTY;
    world->grid[y][x].pheromone_food = PHEROMONE_INITIAL;
    world->grid[y][x].pheromone_home = PHEROMONE_INITIAL;
    world->grid[y][x].food_amount = 0;
    world->grid[y][x].colony_id = -1;
}

// World queries
int is_valid_position(const World* world, int x, int y) {
    if (world == NULL) return 0;
    return (x >= 0 && x < world->width && y >= 0 && y < world->height);
}

int is_walkable(const World* world, int x, int y) {
    if (!is_valid_position(world, x, y)) return 0;
    
    TerrainType terrain = world->grid[y][x].terrain;
    return (terrain == TERRAIN_EMPTY || terrain == TERRAIN_FOOD || terrain == TERRAIN_NEST);
}

Cell* get_cell(const World* world, int x, int y) {
    if (!is_valid_position(world, x, y)) return NULL;
    return &world->grid[y][x];
}

// World initialization
void initialize_world_random(World* world) {
    if (world == NULL) return;
    
    print_info("Initializing world with random obstacles...");
    
    // Add some random obstacles - ensure at least minimum for small worlds
    int total_cells = world->width * world->height;
    int obstacle_count = (total_cells + 19) / 20; // Ceiling division for 5%
    
    // Ensure minimum obstacles for gameplay
    if (obstacle_count < MIN_OBSTACLES_COUNT) obstacle_count = MIN_OBSTACLES_COUNT;
    if (obstacle_count > total_cells / 4) obstacle_count = total_cells / 4; // Max 25%
    
    for (int i = 0; i < obstacle_count; i++) {
        int x = random_int(0, world->width - 1);
        int y = random_int(0, world->height - 1);
        
        // Don't place obstacles on edges or where colonies will be
        if (x > 0 && x < world->width - 1 && y > 0 && y < world->height - 1) {
            if (world->grid[y][x].terrain == TERRAIN_EMPTY) {
                place_obstacle(world, x, y);
            }
        }
    }
    
    // Add some random food sources
    int food_count = random_int(3, 8);
    
    for (int i = 0; i < food_count; i++) {
        int x = random_int(0, world->width - 1);
        int y = random_int(0, world->height - 1);
        
        if (world->grid[y][x].terrain == TERRAIN_EMPTY) {
            int amount = random_int(20, 100);
            place_food(world, x, y, amount);
        }
    }
    
    print_info("Random world initialization complete");
}

void create_test_scenario(World* world) {
    if (world == NULL) return;
    
    print_info("Creating test scenario...");
    
    // Clear existing content
    for (int y = 0; y < world->height; y++) {
        for (int x = 0; x < world->width; x++) {
            clear_cell(world, x, y);
        }
    }
    
    // Place colonies at opposite ends
    if (world->colony_count >= 1) {
        place_colony(world, 0, 5, world->height / 2);
    }
    if (world->colony_count >= 2) {
        place_colony(world, 1, world->width - 6, world->height / 2);
    }
    
    // Create a maze-like pattern
    for (int y = 0; y < world->height; y++) {
        for (int x = 0; x < world->width; x++) {
            // Create walls around edges
            if (x == 0 || x == world->width - 1 || y == 0 || y == world->height - 1) {
                place_obstacle(world, x, y);
            }
            // Create some internal walls
            else if ((x % 8 == 0 && y % 6 != 0) || (y % 6 == 0 && x % 8 != 0)) {
                if (random_probability() < 0.3f) {
                    place_obstacle(world, x, y);
                }
            }
        }
    }
    
    // Place food sources
    place_food(world, world->width / 2, world->height / 2, 50);
    place_food(world, world->width / 4, world->height / 4, 30);
    place_food(world, 3 * world->width / 4, 3 * world->height / 4, 40);
    
    print_info("Test scenario created");
}

// Colony management
void spawn_initial_ants(World* world) {
    if (world == NULL) return;
    
    print_info("Spawning initial ants...");
    
    for (int i = 0; i < world->colony_count; i++) {
        Colony* colony = &world->colonies[i];
        
        // Actually spawn ants using the function from ant_logic.c
        for (int j = 0; j < INITIAL_ANTS_PER_COLONY; j++) {
            spawn_ant(world, i);  // THIS IS THE KEY FIX - actually creates ants!
        }
        
        print_info("Colony %d: %d ants spawned at (%d, %d)", 
                  i, INITIAL_ANTS_PER_COLONY, 
                  colony->nest_pos.x, colony->nest_pos.y);
    }
}

void update_colony_statistics(World* world) {
    if (world == NULL) return;
    
    for (int i = 0; i < world->colony_count; i++) {
        Colony* colony = &world->colonies[i];
        
        // Count active ants
        int active_count = 0;
        Ant* current = colony->ants_head;
        while (current != NULL) {
            if (!(current->state & ANT_STATE_DEAD)) {
                active_count++;
            }
            current = current->next;
        }
        
        colony->active_ants = active_count;
        
        // Calculate efficiency score
        if (colony->total_ants > 0) {
            colony->efficiency_score = (float)colony->food_collected / (float)colony->total_ants;
        }
    }
}
