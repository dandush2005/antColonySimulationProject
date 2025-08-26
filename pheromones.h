#ifndef PHEROMONES_H
#define PHEROMONES_H

#include "data_structures.h"

// Pheromone deposit and evaporation
void deposit_pheromone(World* world, Ant* ant);
void deposit_pheromone_at_position(World* world, int x, int y, int type, float amount);
void evaporate_pheromones(World* world);
void diffuse_pheromones(World* world);

// Pheromone queries
float get_pheromone_intensity(const World* world, int x, int y, int type);
float get_max_pheromone_neighbor(const World* world, int x, int y, int type);

// Pheromone type constants
#define PHEROMONE_TYPE_FOOD 0
#define PHEROMONE_TYPE_HOME 1

// Pheromone utilities
void reset_pheromones(World* world);
void normalize_pheromones(World* world);
float calculate_pheromone_strength(float base_strength, float distance);

// Pheromone visualization helpers
char get_pheromone_symbol(float intensity);
int get_pheromone_color(float intensity);

#endif // PHEROMONES_H
