#include "main.h"
#include "data_structures.h"
#include "config.h"
#include "visualization.h"   // render_frame(), request_full_redraw()
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global variables for program state
static World* g_world = NULL;
static int g_program_running = 1;

// Main program functions
int main(int argc, char* argv[]) {
    initialize_program();
    
    // Handle command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Ant Colony Optimization Simulator\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --help, -h     Show this help message\n");
            printf("  --load <file>  Load simulation from file\n");
            printf("  --test         Run test scenario\n");
            return 0;
        } else if (strcmp(argv[1], "--load") == 0 && argc > 2) {
            g_world = load_simulation(argv[2]);
            if (g_world == NULL) {
                print_error("Failed to load simulation from %s", argv[2]);
                return 1;
            }
            // Set the world as running for command line loading
            g_world->is_running = 1;
        } else if (strcmp(argv[1], "--test") == 0) {
            g_world = create_world(DEFAULT_WORLD_WIDTH, DEFAULT_WORLD_HEIGHT, 2);
            if (g_world != NULL) {
                create_test_scenario(g_world);
                spawn_initial_ants(g_world);
                // Set the world as running for command line test
                g_world->is_running = 1;
            }
        }
    }
    
    // If no world was created, show main menu
    if (g_world == NULL) {
        show_main_menu();
    }
    
    // Main program loop
    while (g_program_running && g_world != NULL) {
        if (g_world->is_running) {
            run_simulation(g_world);
        } else {
            // Show menu when not running
            show_main_menu();
        }
    }
    
    cleanup_program();
    return 0;
}

void run_simulation(World* world) {
    if (world == NULL) return;
    
    // Initialize random number generator
    init_random();
    
    print_info("Starting simulation...");
    world->is_running = 1;
    
    // Main simulation loop
    while (world->is_running && g_program_running) {
        // Handle user input (non-blocking)
        if (_kbhit()) {
            handle_user_input(world);
        }
        
        if (!world->paused) {
            // Update simulation
            update_all_ants(world);
            evaporate_pheromones(world);
            diffuse_pheromones(world);
            update_colony_statistics(world);
            
            world->current_step++;
            
            // Check for simulation end conditions
            if (world->current_step >= MAX_SIMULATION_STEPS) {
                print_info("Simulation reached maximum steps");
                world->is_running = 0;
                break;
            }
            
            // Check if all food is collected
            int total_food = 0;
            for (int y = 0; y < world->height; y++) {
                for (int x = 0; x < world->width; x++) {
                    if (world->grid[y][x].terrain == TERRAIN_FOOD) {
                        total_food += world->grid[y][x].food_amount;
                    }
                }
            }
            
            if (total_food == 0) {
                print_info("All food collected! Simulation complete.");
                world->is_running = 0;
                break;
            }
        }
        
        // Render a single frame via the unified dispatcher
        render_frame(world);
        
        // Save statistics periodically
        if (world->current_step % 100 == 0) {
            save_statistics(world, "data/saves/statistics.csv");
        }
        
        // Sleep for frame delay
        sleep_ms(world->render_delay_ms);
    }
    
    print_info("Simulation ended");
}

void handle_user_input(World* world) {
    if (world == NULL) return;
    
    int key = _getch();
    
    switch (key) {
        case ' ': // SPACE - Pause/Resume
            if (world->paused) {
                resume_simulation(world);
            } else {
                pause_simulation(world);
            }
            break;
            
        case 's': // S - Save
        case 'S':
            {
                char filename[256];
                snprintf(filename, sizeof(filename), "data/saves/simulation_%d.sav", world->current_step);
                if (save_simulation(world, filename) == FILE_IO_SUCCESS) {
                    print_info("Simulation saved to %s", filename);
                }
            }
            break;
            
        case 'l': // L - Load
        case 'L':
            {
                print_info("Enter filename to load (or press Enter to cancel): ");
                char filename[256];
                if (fgets(filename, sizeof(filename), stdin)) {
                    // Remove newline
                    filename[strcspn(filename, "\n")] = 0;
                    if (strlen(filename) > 0) {
                        World* new_world = load_simulation(filename);
                        if (new_world != NULL) {
                            destroy_world(g_world);  // Use global variable directly
                            g_world = new_world;     // Update global variable
                            print_info("Simulation loaded successfully");
                        } else {
                            print_error("Failed to load simulation from %s", filename);
                        }
                    }
                }
            }
            break;
            
        case 'q': // Q - Quit
        case 'Q':
            quit_simulation(world);
            break;
            
        case '+': // Speed up
        case '=':
            world->render_delay_ms = clamp_int(world->render_delay_ms - 50, 10, 1000);
            print_info("Speed increased, delay: %d ms", world->render_delay_ms);
            break;
            
        case '-': // Speed down
            world->render_delay_ms = clamp_int(world->render_delay_ms + 50, 10, 1000);
            print_info("Speed decreased, delay: %d ms", world->render_delay_ms);
            break;
            
        case 'r': // R - Reset
        case 'R':
            reset_simulation(world);
            break;
            
        case 'e': // E - Export map
        case 'E':
            {
                char filename[256];
                snprintf(filename, sizeof(filename), "data/maps/world_%d.map", world->current_step);
                if (export_map(world, filename) == FILE_IO_SUCCESS) {
                    print_info("Map exported to %s", filename);
                }
            }
            break;
            
        case 't': // T - Test scenario
        case 'T':
            create_test_scenario(world);
            spawn_initial_ants(world);
            print_info("Test scenario created");
            break;
    }
}

void show_main_menu(void) {
    clear_screen();
    
    // Print top border
    printf("%s", get_border_tl());
    for (int i = 0; i < 78; i++) printf("%s", get_border_h());
    printf("%s\n", get_border_tr());
    
    // Print title
    printf("%s                    ANT COLONY OPTIMIZATION SIMULATOR                       %s\n", get_border_v(), get_border_v());
    
    // Print separator
    printf("%s", get_border_v());
    for (int i = 0; i < 78; i++) printf("%s", get_border_h());
    printf("%s\n", get_border_v());
    
    // Print menu items
    printf("%s                                                                              %s\n", get_border_v(), get_border_v());
    printf("%s  1. New Simulation                                                          %s\n", get_border_v(), get_border_v());
    printf("%s  2. Load Simulation                                                         %s\n", get_border_v(), get_border_v());
    printf("%s  3. Test Scenario                                                           %s\n", get_border_v(), get_border_v());
    printf("%s  4. Settings                                                                %s\n", get_border_v(), get_border_v());
    printf("%s  5. Exit                                                                    %s\n", get_border_v(), get_border_v());
    printf("%s                                                                              %s\n", get_border_v(), get_border_v());
    
    // Print bottom border
    printf("%s", get_border_bl());
    for (int i = 0; i < 78; i++) printf("%s", get_border_h());
    printf("%s\n", get_border_br());
    printf("\nEnter your choice (1-5): ");
    
    int choice;
    if (scanf("%d", &choice) != 1) {
        print_warning("Invalid input. Please enter a number.");
        while (getchar() != '\n'); // Clear input buffer
        sleep_ms(2000);
        return;
    }
    
    // Clear input buffer after scanf
    while (getchar() != '\n');
    
    if (choice >= 1 && choice <= 5) {
        switch (choice) {
            case 1:
                create_new_simulation();
                break;
            case 2:
                load_simulation_from_menu();
                break;
            case 3:
                create_test_simulation();
                break;
            case 4:
                if (g_world != NULL) {
                    show_settings_menu(g_world);
                } else {
                    print_warning("No simulation running. Create one first.");
                    sleep_ms(2000);
                }
                break;
            case 5:
                g_program_running = 0;
                break;
        }
    } else {
        print_warning("Invalid choice. Please try again.");
        sleep_ms(2000);
    }
}

void create_new_simulation(void) {
    clear_screen();
    printf("%s", get_border_tl());
    for (int i = 0; i < 78; i++) printf("%s", get_border_h());
    printf("%s\n", get_border_tr());
    printf("%s                              NEW SIMULATION                                %s\n", get_border_v(), get_border_v());
    printf("%s", get_border_bl());
    for (int i = 0; i < 78; i++) printf("%s", get_border_h());
    printf("%s\n", get_border_br());
    
    int width, height, colonies;
    
    printf("Enter world width (10-100): ");
    if (scanf("%d", &width) != 1) {
        print_error("Invalid width input");
        while (getchar() != '\n'); // Clear input buffer
        return;
    }
    width = clamp_int(width, 10, MAX_WORLD_SIZE);
    
    printf("Enter world height (10-100): ");
    if (scanf("%d", &height) != 1) {
        print_error("Invalid height input");
        while (getchar() != '\n'); // Clear input buffer
        return;
    }
    height = clamp_int(height, 10, MAX_WORLD_SIZE);
    
    printf("Enter number of colonies (1-5): ");
    if (scanf("%d", &colonies) != 1) {
        print_error("Invalid colonies input");
        while (getchar() != '\n'); // Clear input buffer
        return;
    }
    colonies = clamp_int(colonies, 1, 5);
    
    // Clear input buffer after all scanf calls
    while (getchar() != '\n');
    
    // Create world
    g_world = create_world(width, height, colonies);
    if (g_world != NULL) {
        // Place colonies
        for (int i = 0; i < colonies; i++) {
            int x = (width / (colonies + 1)) * (i + 1);
            int y = height / 2;
            place_colony(g_world, i, x, y);
        }
        
        // Initialize world
        initialize_world_random(g_world);
        spawn_initial_ants(g_world);
        
        // Set the world as running so the main loop will enter simulation mode
        g_world->is_running = 1;
        
        print_info("New simulation created successfully!");
        sleep_ms(2000);
    } else {
        print_error("Failed to create simulation");
        sleep_ms(2000);
    }
}

void load_simulation_from_menu(void) {
    clear_screen();
    printf("%s", get_border_tl());
    for (int i = 0; i < 78; i++) printf("%s", get_border_h());
    printf("%s\n", get_border_tr());
    printf("%s                              LOAD SIMULATION                               %s\n", get_border_v(), get_border_v());
    printf("%s", get_border_bl());
    for (int i = 0; i < 78; i++) printf("%s", get_border_h());
    printf("%s\n", get_border_br());
    
    printf("Enter filename to load: ");
    char filename[256];
    filename[0] = '\0'; // Initialize to empty string
    
    // Clear input buffer first
    while (getchar() != '\n');
    
    if (fgets(filename, sizeof(filename), stdin)) {
        // Remove newline
        filename[strcspn(filename, "\n")] = 0;
        
        if (strlen(filename) > 0) {
            g_world = load_simulation(filename);
            if (g_world != NULL) {
                // Set the world as running so the main loop will enter simulation mode
                g_world->is_running = 1;
                
                print_info("Simulation loaded successfully!");
                sleep_ms(2000);
            } else {
                print_error("Failed to load simulation");
                sleep_ms(2000);
            }
        } else {
            print_warning("No filename entered");
            sleep_ms(2000);
        }
    } else {
        print_error("Failed to read filename");
        sleep_ms(2000);
    }
}

void create_test_simulation(void) {
    g_world = create_world(DEFAULT_WORLD_WIDTH, DEFAULT_WORLD_HEIGHT, 2);
    if (g_world != NULL) {
        create_test_scenario(g_world);
        spawn_initial_ants(g_world);
        
        // Set the world as running so the main loop will enter simulation mode
        g_world->is_running = 1;
        
        print_info("Test simulation created successfully!");
        sleep_ms(2000);
    } else {
        print_error("Failed to create test simulation");
        sleep_ms(2000);
    }
}

void show_settings_menu(World* world) {
    if (world == NULL) return;
    
    clear_screen();
    printf("%s", get_border_tl());
    for (int i = 0; i < 78; i++) printf("%s", get_border_h());
    printf("%s\n", get_border_tr());
    printf("%s                                 SETTINGS                                   %s\n", get_border_v(), get_border_v());
    printf("%s", get_border_bl());
    for (int i = 0; i < 78; i++) printf("%s", get_border_h());
    printf("%s\n", get_border_br());
    
    printf("Current Settings:\n");
    printf("Render Delay: %d ms\n", world->render_delay_ms);
    printf("World Size: %dx%d\n", world->width, world->height);
    printf("Colonies: %d\n", world->colony_count);
    printf("\n");
    
    printf("1. Change render delay\n");
    printf("2. Export current map\n");
    printf("3. Reset pheromones\n");
    printf("4. Back to main menu\n");
    printf("\nEnter your choice (1-4): ");
    
    int choice;
    scanf("%d", &choice);
    
    switch (choice) {
        case 1:
            printf("Enter new render delay (10-1000 ms): ");
            int delay;
            scanf("%d", &delay);
            world->render_delay_ms = clamp_int(delay, 10, 1000);
            print_info("Render delay updated to %d ms", world->render_delay_ms);
            break;
        case 2:
            {
                char filename[256];
                snprintf(filename, sizeof(filename), "data/maps/world_%d.map", world->current_step);
                if (export_map(world, filename) == FILE_IO_SUCCESS) {
                    print_info("Map exported to %s", filename);
                }
            }
            break;
        case 3:
            reset_pheromones(world);
            print_info("Pheromones reset");
            break;
        case 4:
            return;
        default:
            print_warning("Invalid choice");
            break;
    }
    
    sleep_ms(2000);
}

// Simulation control
void pause_simulation(World* world) {
    if (world != NULL) {
        world->paused = 1;
        request_full_redraw();
        print_info("Simulation paused");
    }
}

void resume_simulation(World* world) {
    if (world != NULL) {
        world->paused = 0;
        request_full_redraw();
        print_info("Simulation resumed");
    }
}

void reset_simulation(World* world) {
    if (world != NULL) {
        print_info("Resetting simulation...");
        
        // Reset step counter
        world->current_step = 0;
        
        // Reset pheromones
        reset_pheromones(world);
        
        // Reset colony statistics
        for (int i = 0; i < world->colony_count; i++) {
            world->colonies[i].food_collected = 0;
            world->colonies[i].efficiency_score = 0.0f;
        }
        
        // Clear all ants
        for (int i = 0; i < world->colony_count; i++) {
            Colony* colony = &world->colonies[i];
            Ant* current = colony->ants_head;
            while (current != NULL) {
                Ant* next = current->next;
                destroy_ant(current);
                current = next;
            }
            colony->ants_head = NULL;
            colony->total_ants = 0;
            colony->active_ants = 0;
        }
        
        // Spawn new ants
        spawn_initial_ants(world);
        
        print_info("Simulation reset complete");
    }
}

void quit_simulation(World* world) {
    if (world != NULL) {
        world->is_running = 0;
    }
    g_program_running = 0;
}

// Program lifecycle
void initialize_program(void) {
    print_info("Initializing Ant Colony Optimization Simulator...");
    
    // Initialize console
    init_console();
    
    // Initialize random number generator
    init_random();
    
    print_info("Program initialization complete");
}

void cleanup_program(void) {
    print_info("Cleaning up program...");
    
    // Save final statistics if world exists
    if (g_world != NULL) {
        save_statistics(g_world, "data/saves/statistics.csv");
        destroy_world(g_world);
        g_world = NULL;
    }
    
    // Cleanup console
    cleanup_console();
    
    print_info("Program cleanup complete");
}

void handle_program_exit(void) {
    print_info("Exiting program...");
    cleanup_program();
}
