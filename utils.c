#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <windows.h>

// Random number generation
static int random_initialized = 0;

void init_random(void) {
    if (!random_initialized) {
        srand((unsigned int)time(NULL));
        random_initialized = 1;
    }
}

int random_int(int min, int max) {
    if (!random_initialized) {
        init_random();
    }
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    return min + (rand() % (max - min + 1));
}

float random_float(float min, float max) {
    if (!random_initialized) {
        init_random();
    }
    if (min > max) {
        float temp = min;
        min = max;
        max = temp;
    }
    float scale = (float)rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

float random_probability(void) {
    if (!random_initialized) {
        init_random();
    }
    return (float)rand() / (float)RAND_MAX;
}

// Memory utilities
void* safe_malloc(size_t size) {
    if (size == 0) {
        print_error("Attempted to allocate 0 bytes");
        return NULL;
    }
    
    void* ptr = malloc(size);
    if (ptr == NULL) {
        print_error("Memory allocation failed");
    }
    return ptr;
}

void* safe_calloc(size_t count, size_t size) {
    if (count == 0 || size == 0) {
        print_error("Attempted to allocate 0 bytes");
        return NULL;
    }
    
    void* ptr = calloc(count, size);
    if (ptr == NULL) {
        print_error("Memory allocation failed");
    }
    return ptr;
}

void safe_free(void* ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}

// String utilities
int safe_strcpy(char* dest, const char* src, size_t dest_size) {
    if (dest == NULL || src == NULL || dest_size == 0) {
        return 0;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        src_len = dest_size - 1;
    }
    
    memcpy(dest, src, src_len);
    dest[src_len] = '\0';
    return 1;
}

int safe_strcat(char* dest, const char* src, size_t dest_size) {
    if (dest == NULL || src == NULL || dest_size == 0) {
        return 0;
    }
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len + src_len >= dest_size) {
        src_len = dest_size - dest_len - 1;
    }
    
    if (src_len > 0) {
        memcpy(dest + dest_len, src, src_len);
        dest[dest_len + src_len] = '\0';
    }
    
    return 1;
}

void trim_string(char* str) {
    if (str == NULL) return;
    
    char* start = str;
    char* end = str + strlen(str) - 1;
    
    // Trim leading whitespace
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {
        start++;
    }
    
    // Trim trailing whitespace
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    
    // Move trimmed string to beginning
    if (start != str) {
        memmove(str, start, end - start + 1);
        str[end - start + 1] = '\0';
    } else {
        str[end - start + 1] = '\0';
    }
}

// Time utilities
void sleep_ms(int milliseconds) {
    Sleep(milliseconds);
}

uint64_t get_time_ms(void) {
    FILETIME ft;
    ULARGE_INTEGER ui;
    
    GetSystemTimeAsFileTime(&ft);
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    
    // Convert to milliseconds (100-nanosecond intervals to milliseconds)
    return (ui.QuadPart - 116444736000000000ULL) / 10000ULL;
}

// Math utilities
float clamp_float(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int clamp_int(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float lerp(float a, float b, float t) {
    t = clamp_float(t, 0.0f, 1.0f);
    return a + t * (b - a);
}

// Debug utilities - disabled to avoid compilation issues
// Can be enabled later if needed for debugging
/*
#ifdef _DEBUG
void debug_print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[DEBUG] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void debug_print_world_state(const World* world) {
    if (world == NULL) {
        printf("[DEBUG] World is NULL\n");
        return;
    }
    
    printf("[DEBUG] World State:\n");
    printf("  Dimensions: %dx%d\n", world->width, world->height);
    printf("  Colonies: %d\n", world->colony_count);
    printf("  Current Step: %d\n", world->current_step);
    printf("  Running: %s\n", world->is_running ? "Yes" : "No");
    printf("  Paused: %s\n", world->paused ? "Yes" : "No");
}

void debug_print_ant_state(const Ant* ant) {
    if (ant == NULL) {
        printf("[DEBUG] Ant is NULL\n");
        return;
    }
    
    printf("[DEBUG] Ant %d State:\n", ant->id);
    printf("  Position: (%d, %d)\n", ant->pos.x, ant->pos.y);
    printf("  Colony: %d\n", ant->colony_id);
    printf("  Energy: %.1f\n", ant->energy);
    printf("  State: 0x%02X\n", ant->state);
    printf("  Food Carrying: %d\n", ant->food_carrying);
    printf("  Steps: %d\n", ant->steps_taken);
}
#endif
*/

// Error handling
void print_error(const char* format, ...) {
    va_list args;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), COLOR_BRIGHT_RED);
    printf("[ERROR] ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), COLOR_WHITE);
}

void print_warning(const char* format, ...) {
    va_list args;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), COLOR_BRIGHT_YELLOW);
    printf("[WARNING] ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), COLOR_WHITE);
}

void print_info(const char* format, ...) {
    va_list args;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), COLOR_BRIGHT_CYAN);
    printf("[INFO] ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), COLOR_WHITE);
}
