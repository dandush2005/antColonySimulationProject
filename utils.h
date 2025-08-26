#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "data_structures.h"

// Random number generation
void init_random(void);
int random_int(int min, int max);
float random_float(float min, float max);
float random_probability(void);

// Memory utilities
void* safe_malloc(size_t size);
void* safe_calloc(size_t count, size_t size);
void safe_free(void* ptr);

// String utilities
int safe_strcpy(char* dest, const char* src, size_t dest_size);
int safe_strcat(char* dest, const char* src, size_t dest_size);
void trim_string(char* str);

// Time utilities
void sleep_ms(int milliseconds);
uint64_t get_time_ms(void);

// Math utilities
float clamp_float(float value, float min, float max);
int clamp_int(int value, int min, int max);
float lerp(float a, float b, float t);

// Debug utilities
#ifdef _DEBUG
void debug_print(const char* format, ...);
void debug_print_world_state(const World* world);
void debug_print_ant_state(const Ant* ant);
#else
#define debug_print(...)
#define debug_print_world_state(world)
#define debug_print_ant_state(ant)
#endif

// Error handling
void print_error(const char* format, ...);
void print_warning(const char* format, ...);
void print_info(const char* format, ...);

#endif // UTILS_H
