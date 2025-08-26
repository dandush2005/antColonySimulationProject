#ifndef CONFIG_H
#define CONFIG_H

// Suppress unsafe function warnings for Windows
#define _CRT_SECURE_NO_WARNINGS

// World parameters
#define DEFAULT_WORLD_WIDTH 60
#define DEFAULT_WORLD_HEIGHT 30
#define MAX_WORLD_SIZE 100

// Ant parameters
#define INITIAL_ANTS_PER_COLONY 20
#define MAX_ANTS_PER_COLONY 50
#define ANT_INITIAL_ENERGY 1000
#define ANT_ENERGY_PER_STEP 1
#define ANT_ENERGY_FROM_FOOD 500

// Pheromone parameters
#define PHEROMONE_INITIAL 0.0f
#define PHEROMONE_MAX 1000.0f
#define PHEROMONE_MIN_THRESHOLD 0.1f  // ADD THIS LINE!
#define PHEROMONE_DEPOSIT_AMOUNT 100.0f
#define PHEROMONE_EVAPORATION_RATE 0.02f
#define PHEROMONE_DIFFUSION_RATE 0.01f
#define PHEROMONE_DISPLAY_THRESHOLD 1.0f  // Show pheromones even at low levels
#define DIRECTION_COUNT 8

// Buffer sizes
#define INPUT_BUFFER_SIZE 256
#define LINE_BUFFER_SIZE 1024
#define TIMESTAMP_BUFFER_SIZE 64
#define BACKUP_NAME_BUFFER_SIZE 512

// Limits
#define MAX_OBSTACLES_PERCENTAGE 25  // Maximum 25% of world can be obstacles
#define MIN_OBSTACLES_COUNT 3        // Minimum obstacles for interesting gameplay
#define QUICKSORT_RECURSION_LIMIT 100  // Switch to iterative above this

// Unicode support detection (now handled at runtime in visualization.c)

// Pheromone types
#define PHEROMONE_FOOD 0
#define PHEROMONE_HOME 1

// Terrain constants
#define TERRAIN_OBSTACLE 2  // Obstacle terrain type

// Behavior parameters
#define FOLLOW_PHEROMONE_PROBABILITY 0.8f
#define RANDOM_EXPLORATION_PROBABILITY 0.2f

// Ant state flags (bitwise)
#define ANT_STATE_IDLE        0x00
#define ANT_STATE_SEARCHING   0x01
#define ANT_STATE_RETURNING   0x02
#define ANT_STATE_FOLLOWING   0x04
#define ANT_STATE_CARRYING    0x08
#define ANT_STATE_SCOUT       0x10
#define ANT_STATE_TIRED       0x20
#define ANT_STATE_DEAD        0x40

// Debug mode
#ifdef _DEBUG
    #define DEBUG_MODE 1
    #define LOG(x) printf x
#else
    #define DEBUG_MODE 0
    #define LOG(x)
#endif

// Console colors (Windows)
#define COLOR_BLACK     0
#define COLOR_BLUE      1
#define COLOR_GREEN     2
#define COLOR_CYAN      3
#define COLOR_RED       4
#define COLOR_MAGENTA   5
#define COLOR_YELLOW    6
#define COLOR_WHITE     7
#define COLOR_BRIGHT_BLUE    9
#define COLOR_BRIGHT_GREEN   10
#define COLOR_BRIGHT_CYAN    11
#define COLOR_BRIGHT_RED     12
#define COLOR_BRIGHT_MAGENTA 13
#define COLOR_BRIGHT_YELLOW  14
#define COLOR_BRIGHT_WHITE   15

// Simulation parameters
#define RENDER_DELAY_MS 150  // Reduced from 200ms to 150ms for better viewing
#define MAX_SIMULATION_STEPS 10000

#endif // CONFIG_H
