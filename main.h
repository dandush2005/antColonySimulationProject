#ifndef MAIN_H
#define MAIN_H

// Include all project headers
#include "config.h"
#include "data_structures.h"
#include "world.h"
#include "ant_logic.h"
#include "pheromones.h"
#include "visualization.h"
#include "file_io.h"
#include "algorithms.h"
#include "utils.h"

// Main program functions
int main(int argc, char* argv[]);
void run_simulation(World* world);
void handle_user_input(World* world);
void show_main_menu(void);
void show_settings_menu(World* world);

// Simulation control
void pause_simulation(World* world);
void resume_simulation(World* world);
void reset_simulation(World* world);
void quit_simulation(World* world);

// Program lifecycle
void initialize_program(void);
void cleanup_program(void);
void handle_program_exit(void);

// Menu functions
void create_new_simulation(void);
void load_simulation_from_menu(void);
void create_test_simulation(void);

#endif // MAIN_H
