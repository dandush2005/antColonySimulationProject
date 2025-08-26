#include "pheromones.h"
#include "config.h"
#include "utils.h"
#include "world.h"
#include "visualization.h"  // for is_unicode_enabled()
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Pheromone deposit and evaporation
void deposit_pheromone(World* world, Ant* ant) {
    if (world == NULL || ant == NULL) return;
    
    Cell* cell = get_cell(world, ant->pos.x, ant->pos.y);
    if (cell == NULL) return;
    
    if (ant->state & ANT_STATE_SEARCHING) {
        // Searching ants deposit home pheromone
        cell->pheromone_home += PHEROMONE_DEPOSIT_AMOUNT;
        if (cell->pheromone_home > PHEROMONE_MAX) {
            cell->pheromone_home = PHEROMONE_MAX;
        }
        
        LOG_PHEROMONE_INFO("Ant %d deposited home pheromone at (%d, %d), level: %.1f", 
                           ant->id, ant->pos.x, ant->pos.y, cell->pheromone_home);
        
    } else if (ant->state & ANT_STATE_RETURNING) {
        // Returning ants deposit food pheromone
        cell->pheromone_food += PHEROMONE_DEPOSIT_AMOUNT;
        if (cell->pheromone_food > PHEROMONE_MAX) {
            cell->pheromone_food = PHEROMONE_MAX;
        }
        
        LOG_PHEROMONE_INFO("Ant %d deposited food pheromone at (%d, %d), level: %.1f", 
                           ant->id, ant->pos.x, ant->pos.y, cell->pheromone_food);
    }
}

void deposit_pheromone_at_position(World* world, int x, int y, int type, float amount) {
    if (world == NULL || !is_valid_position(world, x, y)) return;
    
    Cell* cell = get_cell(world, x, y);
    if (cell == NULL) return;
    
    if (type == PHEROMONE_TYPE_FOOD) {
        cell->pheromone_food += amount;
        if (cell->pheromone_food > PHEROMONE_MAX) {
            cell->pheromone_food = PHEROMONE_MAX;
        }
    } else if (type == PHEROMONE_TYPE_HOME) {
        cell->pheromone_home += amount;
        if (cell->pheromone_home > PHEROMONE_MAX) {
            cell->pheromone_home = PHEROMONE_MAX;
        }
    }
}

void evaporate_pheromones(World* world) {
    if (world == NULL) return;
    
    for (int y = 0; y < world->height; y++) {
        for (int x = 0; x < world->width; x++) {
            Cell* cell = &world->grid[y][x];
            
            // Evaporate food pheromone
            cell->pheromone_food *= (1.0f - PHEROMONE_EVAPORATION_RATE);
            if (cell->pheromone_food < PHEROMONE_MIN_THRESHOLD) {  // USE CONSTANT
                cell->pheromone_food = 0.0f;
            }
            
            // Evaporate home pheromone
            cell->pheromone_home *= (1.0f - PHEROMONE_EVAPORATION_RATE);
            if (cell->pheromone_home < PHEROMONE_MIN_THRESHOLD) {  // USE CONSTANT
                cell->pheromone_home = 0.0f;
            }
        }
    }
}

void diffuse_pheromones(World* world) {
    if (world == NULL) return;
    
    // Create temporary grid for diffusion calculations
    float** temp_food = (float**)safe_malloc(world->height * sizeof(float*));
    float** temp_home = (float**)safe_malloc(world->height * sizeof(float*));
    
    if (temp_food == NULL || temp_home == NULL) {
        // Clean up and return if allocation failed
        if (temp_food) safe_free(temp_food);
        if (temp_home) safe_free(temp_home);
        return;
    }
    
    // Allocate rows
    for (int i = 0; i < world->height; i++) {
        temp_food[i] = (float*)safe_calloc(world->width, sizeof(float));
        temp_home[i] = (float*)safe_calloc(world->width, sizeof(float));
        
        if (temp_food[i] == NULL || temp_home[i] == NULL) {
            // Clean up THIS row if partially allocated
            if (temp_food[i]) {
                safe_free(temp_food[i]);
                temp_food[i] = NULL;
            }
            if (temp_home[i]) {
                safe_free(temp_home[i]);
                temp_home[i] = NULL;
            }
            
            // Clean up all previous rows
            for (int j = 0; j < i; j++) {
                if (temp_food[j]) safe_free(temp_food[j]);
                if (temp_home[j]) safe_free(temp_home[j]);
            }
            
            // Free the arrays themselves
            safe_free(temp_food);
            safe_free(temp_home);
            return;
        }
    }
    
    // Copy current pheromone levels to temp grid
    for (int y = 0; y < world->height; y++) {
        for (int x = 0; x < world->width; x++) {
            temp_food[y][x] = world->grid[y][x].pheromone_food;
            temp_home[y][x] = world->grid[y][x].pheromone_home;
        }
    }
    
    // Apply diffusion
    for (int y = 0; y < world->height; y++) {
        for (int x = 0; x < world->width; x++) {
            float neighbor_food_contribution = 0.0f;
            float neighbor_home_contribution = 0.0f;
            int valid_neighbors = 0;
            
            // Check all 8 neighbors
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue; // Skip center
                    
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (is_valid_position(world, nx, ny)) {
                        neighbor_food_contribution += temp_food[ny][nx];
                        neighbor_home_contribution += temp_home[ny][nx];
                        valid_neighbors++;
                    }
                }
            }
            
            // Apply proper diffusion: keep most original + small neighbor influence
            if (valid_neighbors > 0) {
                world->grid[y][x].pheromone_food = temp_food[y][x] * (1.0f - PHEROMONE_DIFFUSION_RATE) + 
                                                   (neighbor_food_contribution * PHEROMONE_DIFFUSION_RATE) / valid_neighbors;
                world->grid[y][x].pheromone_home = temp_home[y][x] * (1.0f - PHEROMONE_DIFFUSION_RATE) + 
                                                   (neighbor_home_contribution * PHEROMONE_DIFFUSION_RATE) / valid_neighbors;
            } else {
                // No neighbors, just keep original values
                world->grid[y][x].pheromone_food = temp_food[y][x];
                world->grid[y][x].pheromone_home = temp_home[y][x];
            }
        }
    }
    
    // Clean up temporary arrays
    for (int i = 0; i < world->height; i++) {
        safe_free(temp_food[i]);
        safe_free(temp_home[i]);
    }
    safe_free(temp_food);
    safe_free(temp_home);
}

// Pheromone queries
float get_pheromone_intensity(const World* world, int x, int y, int type) {
    if (!is_valid_position(world, x, y)) return 0.0f;
    
    Cell* cell = &world->grid[y][x];
    
    switch (type) {
        case PHEROMONE_TYPE_FOOD:
            return cell->pheromone_food;
        case PHEROMONE_TYPE_HOME:
            return cell->pheromone_home;
        default:
            return 0.0f;
    }
}

float get_max_pheromone_neighbor(const World* world, int x, int y, int type) {
    if (!is_valid_position(world, x, y)) return 0.0f;
    
    float max_pheromone = 0.0f;
    
    // Check 8 neighbors
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue; // Skip center cell
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (is_valid_position(world, nx, ny)) {
                float pheromone = get_pheromone_intensity(world, nx, ny, type);
                if (pheromone > max_pheromone) {
                    max_pheromone = pheromone;
                }
            }
        }
    }
    
    return max_pheromone;
}

// Pheromone utilities
void reset_pheromones(World* world) {
    if (world == NULL) return;
    
    for (int y = 0; y < world->height; y++) {
        for (int x = 0; x < world->width; x++) {
            world->grid[y][x].pheromone_food = PHEROMONE_INITIAL;
            world->grid[y][x].pheromone_home = PHEROMONE_INITIAL;
        }
    }
    
    print_info("All pheromones reset");
}

void normalize_pheromones(World* world) {
    if (world == NULL) return;
    
    float max_food = 0.0f;
    float max_home = 0.0f;
    
    // Find maximum values
    for (int y = 0; y < world->height; y++) {
        for (int x = 0; x < world->width; x++) {
            if (world->grid[y][x].pheromone_food > max_food) {
                max_food = world->grid[y][x].pheromone_food;
            }
            if (world->grid[y][x].pheromone_home > max_home) {
                max_home = world->grid[y][x].pheromone_home;
            }
        }
    }
    
    // Normalize if maximum is greater than 0
    if (max_food > 0.0f) {
        for (int y = 0; y < world->height; y++) {
            for (int x = 0; x < world->width; x++) {
                world->grid[y][x].pheromone_food = 
                    (world->grid[y][x].pheromone_food / max_food) * PHEROMONE_MAX;
            }
        }
    }
    
    if (max_home > 0.0f) {
        for (int y = 0; y < world->height; y++) {
            for (int x = 0; x < world->width; x++) {
                world->grid[y][x].pheromone_home = 
                    (world->grid[y][x].pheromone_home / max_home) * PHEROMONE_MAX;
            }
        }
    }
    
    print_info("Pheromones normalized");
}

float calculate_pheromone_strength(float base_strength, float distance) {
    // Exponential decay based on distance
    return base_strength * expf(-distance * 0.1f);
}

// Pheromone visualization helpers
char get_pheromone_symbol(float intensity) {
    if (intensity <= 0.0f) return ' ';
    
    if (is_unicode_enabled()) {
        if (intensity < 50.0f) return '░';      // Very light
        else if (intensity < 200.0f) return '▒'; // Light
        else if (intensity < 500.0f) return '▓'; // Medium
        else return '█';                         // Dark
    } else {
        // ASCII fallbacks
        if (intensity < 50.0f) return '.';      // Very light
        else if (intensity < 200.0f) return ':'; // Light
        else if (intensity < 500.0f) return '#'; // Medium
        else return '@';                         // Dark
    }
}

int get_pheromone_color(float intensity) {
    if (intensity <= 0.0f) return COLOR_WHITE;
    if (intensity < PHEROMONE_MAX * 0.25f) return COLOR_BLUE;
    if (intensity < PHEROMONE_MAX * 0.5f) return COLOR_CYAN;
    if (intensity < PHEROMONE_MAX * 0.75f) return COLOR_YELLOW;
    return COLOR_BRIGHT_RED;
}
