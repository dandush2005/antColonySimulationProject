#include "algorithms.h"
#include "config.h"
#include "utils.h"
#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Sorting algorithms
void quicksort_ants_by_efficiency(Ant** ants, int left, int right) {
    if (left < right) {
        int pivot = partition_ants(ants, left, right);
        quicksort_ants_by_efficiency(ants, left, pivot - 1);
        quicksort_ants_by_efficiency(ants, pivot + 1, right);
    }
}

int partition_ants(Ant** ants, int left, int right) {
    // Choose rightmost element as pivot
    float pivot_efficiency = calculate_ant_efficiency(ants[right]);
    int i = left - 1;
    
    for (int j = left; j < right; j++) {
        float current_efficiency = calculate_ant_efficiency(ants[j]);
        
        // Sort in descending order (highest efficiency first)
        if (current_efficiency >= pivot_efficiency) {
            i++;
            // Swap ants
            Ant* temp = ants[i];
            ants[i] = ants[j];
            ants[j] = temp;
        }
    }
    
    // Place pivot in correct position
    Ant* temp = ants[i + 1];
    ants[i + 1] = ants[right];
    ants[right] = temp;
    
    return i + 1;
}

void sort_ants_by_efficiency(Ant** ants, int count) {
    if (ants == NULL || count <= 1) return;
    
    print_info("Sorting %d ants by efficiency...", count);
    quicksort_ants_by_efficiency(ants, 0, count - 1);
    print_info("Ant sorting complete");
}

// Searching algorithms
Ant* binary_search_ant_by_id(Ant** sorted_ants, int count, int target_id) {
    if (sorted_ants == NULL || count <= 0) return NULL;
    
    int left = 0;
    int right = count - 1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int current_id = sorted_ants[mid]->id;
        
        if (current_id == target_id) {
            return sorted_ants[mid];
        }
        
        if (current_id < target_id) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return NULL; // Ant not found
}

Ant* linear_search_ant_by_id(Ant* head, int target_id) {
    if (head == NULL) return NULL;
    
    Ant* current = head;
    while (current != NULL) {
        if (current->id == target_id) {
            return current;
        }
        current = current->next;
    }
    
    return NULL; // Ant not found
}

// Linked list utilities
Ant** list_to_array(Ant* head, int* count) {
    if (head == NULL || count == NULL) return NULL;
    
    // Count ants in list
    *count = 0;
    Ant* current = head;
    while (current != NULL) {
        (*count)++;
        current = current->next;
    }
    
    if (*count == 0) return NULL;
    
    // Allocate array of ant pointers
    Ant** array = (Ant**)safe_malloc(*count * sizeof(Ant*));
    if (array == NULL) {
        *count = 0;
        return NULL;
    }
    
    // Fill array with ant pointers
    current = head;
    int index = 0;
    while (current != NULL && index < *count) {
        array[index] = current;
        current = current->next;
        index++;
    }
    
    return array;
}

void free_ant_array(Ant** array) {
    if (array != NULL) {
        safe_free(array);
    }
}

// Pathfinding algorithms
int find_path_astar(const World* world, Position start, Position goal, Position** path) {
    if (world == NULL || path == NULL) return 0;
    
    // Simple A* implementation for demonstration
    // In a full implementation, this would use a priority queue and proper heuristics
    
    // For now, return a simple straight-line path if walkable
    int max_path_length = manhattan_distance(start, goal) + 10;
    *path = (Position*)safe_malloc(max_path_length * sizeof(Position));
    
    if (*path == NULL) return 0;
    
    int path_length = 0;
    Position current = start;
    
    // Simple pathfinding: move towards goal one step at a time
    while (path_length < max_path_length && 
           (current.x != goal.x || current.y != goal.y)) {
        
        (*path)[path_length] = current;
        path_length++;
        
        // Calculate direction to goal
        int dx = (goal.x > current.x) ? 1 : (goal.x < current.x) ? -1 : 0;
        int dy = (goal.y > current.y) ? 1 : (goal.y < current.y) ? -1 : 0;
        
        // Try to move towards goal
        int new_x = current.x + dx;
        int new_y = current.y + dy;
        
        if (is_valid_position(world, new_x, new_y) && is_walkable(world, new_x, new_y)) {
            current.x = new_x;
            current.y = new_y;
        } else {
            // Try alternative directions if direct path blocked
            int alt_dx = (dx == 0) ? 1 : 0;
            int alt_dy = (dy == 0) ? 1 : 0;
            
            new_x = current.x + alt_dx;
            new_y = current.y + alt_dy;
            
            if (is_valid_position(world, new_x, new_y) && is_walkable(world, new_x, new_y)) {
                current.x = new_x;
                current.y = new_y;
            } else {
                // Try the other alternative
                new_x = current.x - alt_dx;
                new_y = current.y - alt_dy;
                
                if (is_valid_position(world, new_x, new_y) && is_walkable(world, new_x, new_y)) {
                    current.x = new_x;
                    current.y = new_y;
                } else {
                    // Path blocked, break
                    break;
                }
            }
        }
    }
    
    // Add final position if we reached the goal
    if (current.x == goal.x && current.y == goal.y) {
        (*path)[path_length] = current;
        path_length++;
    }
    
    return path_length;
}

void free_path(Position* path) {
    if (path != NULL) {
        safe_free(path);
    }
}

// Efficiency calculations
float calculate_ant_efficiency(const Ant* ant) {
    if (ant == NULL) return 0.0f;
    
    if (ant->steps_taken == 0) return 0.0f;
    
    // Efficiency = food delivered / steps taken
    // Higher efficiency means more food delivered with fewer steps
    float efficiency = (float)ant->food_delivered / (float)ant->steps_taken;
    
    // Bonus for ants that are still alive and active
    if (!(ant->state & ANT_STATE_DEAD)) {
        efficiency *= 1.2f;
    }
    
    // Bonus for ants with high energy
    if (ant->energy > ANT_INITIAL_ENERGY * 0.8f) {
        efficiency *= 1.1f;
    }
    
    return efficiency;
}

float calculate_colony_efficiency(const Colony* colony) {
    if (colony == NULL) return 0.0f;
    
    if (colony->total_ants == 0) return 0.0f;
    
    // Colony efficiency = total food collected / total ants
    float base_efficiency = (float)colony->food_collected / (float)colony->total_ants;
    
    // Bonus for colonies with high active ant ratio
    float active_ratio = (float)colony->active_ants / (float)colony->total_ants;
    if (active_ratio > 0.8f) {
        base_efficiency *= 1.3f;
    } else if (active_ratio > 0.6f) {
        base_efficiency *= 1.1f;
    }
    
    return base_efficiency;
}

// Utility algorithms
int manhattan_distance(Position a, Position b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

float euclidean_distance(Position a, Position b) {
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return sqrtf((float)(dx * dx + dy * dy));
}

Position get_random_position(int max_x, int max_y) {
    Position pos;
    pos.x = random_int(0, max_x - 1);
    pos.y = random_int(0, max_y - 1);
    return pos;
}
