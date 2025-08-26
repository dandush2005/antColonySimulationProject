#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "data_structures.h"

// Sorting algorithms
void quicksort_ants_by_efficiency(Ant** ants, int left, int right);
int partition_ants(Ant** ants, int left, int right);
void sort_ants_by_efficiency(Ant** ants, int count);

// Searching algorithms
Ant* binary_search_ant_by_id(Ant** sorted_ants, int count, int target_id);
Ant* linear_search_ant_by_id(Ant* head, int target_id);

// Linked list utilities
Ant** list_to_array(Ant* head, int* count);
void free_ant_array(Ant** array);

// Pathfinding algorithms
int find_path_astar(const World* world, Position start, Position goal, Position** path);
void free_path(Position* path);

// Efficiency calculations
float calculate_ant_efficiency(const Ant* ant);
float calculate_colony_efficiency(const Colony* colony);

// Utility algorithms
int manhattan_distance(Position a, Position b);
float euclidean_distance(Position a, Position b);
Position get_random_position(int max_x, int max_y);

#endif // ALGORITHMS_H
