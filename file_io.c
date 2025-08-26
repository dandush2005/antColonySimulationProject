#include "file_io.h"
#include "utils.h"
#include "world.h"
#include "ant_logic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Save and load simulation
int save_simulation(const World* world, const char* filename) {
    if (world == NULL || filename == NULL) {
        return FILE_IO_ERROR_INVALID_FORMAT;
    }
    
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        print_error("Failed to open file for writing");
        return FILE_IO_ERROR_OPEN;
    }
    
    // Write header
    if (fwrite(SAVE_FILE_HEADER, sizeof(char), strlen(SAVE_FILE_HEADER), file) != strlen(SAVE_FILE_HEADER)) {
        print_error("Failed to write file header");
        fclose(file);
        return FILE_IO_ERROR_WRITE;
    }
    
    // Write version
    if (fwrite(SAVE_FILE_VERSION, sizeof(char), strlen(SAVE_FILE_VERSION), file) != strlen(SAVE_FILE_VERSION)) {
        print_error("Failed to write file version");
        fclose(file);
        return FILE_IO_ERROR_WRITE;
    }
    
    // Write world dimensions
    if (fwrite(&world->width, sizeof(int), 1, file) != 1 ||
        fwrite(&world->height, sizeof(int), 1, file) != 1 ||
        fwrite(&world->colony_count, sizeof(int), 1, file) != 1) {
        print_error("Failed to write world dimensions");
        fclose(file);
        return FILE_IO_ERROR_WRITE;
    }
    
    // Write colony data
    for (int i = 0; i < world->colony_count; i++) {
        Colony* colony = &world->colonies[i];
        if (fwrite(&colony->nest_pos, sizeof(Position), 1, file) != 1 ||
            fwrite(&colony->food_collected, sizeof(int), 1, file) != 1 ||
            fwrite(&colony->total_ants, sizeof(int), 1, file) != 1 ||
            fwrite(&colony->active_ants, sizeof(int), 1, file) != 1 ||
            fwrite(&colony->efficiency_score, sizeof(float), 1, file) != 1) {
            print_error("Failed to write colony data");
            fclose(file);
            return FILE_IO_ERROR_WRITE;
        }
    }
    
    // Write grid data
    for (int y = 0; y < world->height; y++) {
        for (int x = 0; x < world->width; x++) {
            Cell* cell = &world->grid[y][x];
            if (fwrite(&cell->terrain, sizeof(TerrainType), 1, file) != 1 ||
                fwrite(&cell->pheromone_food, sizeof(float), 1, file) != 1 ||
                fwrite(&cell->pheromone_home, sizeof(float), 1, file) != 1 ||
                fwrite(&cell->food_amount, sizeof(int), 1, file) != 1 ||
                fwrite(&cell->colony_id, sizeof(int), 1, file) != 1) {
                print_error("Failed to write grid data");
                fclose(file);
                return FILE_IO_ERROR_WRITE;
            }
        }
    }
    
    // Write ants data
    for (int i = 0; i < world->colony_count; i++) {
        Colony* colony = &world->colonies[i];
        Ant* current = colony->ants_head;
        
        while (current != NULL) {
            if (fwrite(&current->id, sizeof(int), 1, file) != 1 ||
                fwrite(&current->pos, sizeof(Position), 1, file) != 1 ||
                fwrite(&current->last_pos, sizeof(Position), 1, file) != 1 ||
                fwrite(&current->state, sizeof(uint8_t), 1, file) != 1 ||
                fwrite(&current->colony_id, sizeof(int), 1, file) != 1 ||
                fwrite(&current->energy, sizeof(float), 1, file) != 1 ||
                fwrite(&current->food_carrying, sizeof(int), 1, file) != 1 ||
                fwrite(&current->steps_taken, sizeof(int), 1, file) != 1 ||
                fwrite(&current->food_delivered, sizeof(int), 1, file) != 1) {
                print_error("Failed to write ant data");
                fclose(file);
                return FILE_IO_ERROR_WRITE;
            }
            current = current->next;
        }
        
        // Write end marker for this colony
        int end_marker = -1;
        if (fwrite(&end_marker, sizeof(int), 1, file) != 1) {
            print_error("Failed to write colony end marker");
            fclose(file);
            return FILE_IO_ERROR_WRITE;
        }
    }
    
    fclose(file);
    print_info("Simulation saved to %s", filename);
    return FILE_IO_SUCCESS;
}

World* load_simulation(const char* filename) {
    if (filename == NULL) return NULL;
    
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        print_error("Failed to open file for reading");
        return NULL;
    }
    
    // Read and verify header
    char header[32];
    if (fread(header, sizeof(char), strlen(SAVE_FILE_HEADER), file) != strlen(SAVE_FILE_HEADER)) {
        print_error("Failed to read file header");
        fclose(file);
        return NULL;
    }
    
    if (strncmp(header, SAVE_FILE_HEADER, strlen(SAVE_FILE_HEADER)) != 0) {
        print_error("Invalid file format");
        fclose(file);
        return NULL;
    }
    
    // Read version
    char version[16];
    if (fread(version, sizeof(char), strlen(SAVE_FILE_VERSION), file) != strlen(SAVE_FILE_VERSION)) {
        print_error("Failed to read file version");
        fclose(file);
        return NULL;
    }
    
    // Read world dimensions
    int width, height, colony_count;
    if (fread(&width, sizeof(int), 1, file) != 1 ||
        fread(&height, sizeof(int), 1, file) != 1 ||
        fread(&colony_count, sizeof(int), 1, file) != 1) {
        print_error("Failed to read world dimensions");
        fclose(file);
        return NULL;
    }
    
    // Create world
    World* world = create_world(width, height, colony_count);
    if (world == NULL) {
        print_error("Failed to create world for loading");
        fclose(file);
        return NULL;
    }
    
    // Read colony data
    for (int i = 0; i < colony_count; i++) {
        Colony* colony = &world->colonies[i];
        if (fread(&colony->nest_pos, sizeof(Position), 1, file) != 1 ||
            fread(&colony->food_collected, sizeof(int), 1, file) != 1 ||
            fread(&colony->total_ants, sizeof(int), 1, file) != 1 ||
            fread(&colony->active_ants, sizeof(int), 1, file) != 1 ||
            fread(&colony->efficiency_score, sizeof(float), 1, file) != 1) {
            print_error("Failed to read colony data");
            fclose(file);
            destroy_world(world);
            return NULL;
        }
        
        // Update grid to reflect colony position
        if (is_valid_position(world, colony->nest_pos.x, colony->nest_pos.y)) {
            world->grid[colony->nest_pos.y][colony->nest_pos.x].terrain = TERRAIN_NEST;
            world->grid[colony->nest_pos.y][colony->nest_pos.x].colony_id = i;
        }
    }
    
    // Read grid data
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Cell* cell = &world->grid[y][x];
            if (fread(&cell->terrain, sizeof(TerrainType), 1, file) != 1 ||
                fread(&cell->pheromone_food, sizeof(float), 1, file) != 1 ||
                fread(&cell->pheromone_home, sizeof(float), 1, file) != 1 ||
                fread(&cell->food_amount, sizeof(int), 1, file) != 1 ||
                fread(&cell->colony_id, sizeof(int), 1, file) != 1) {
                print_error("Failed to read grid data");
                fclose(file);
                destroy_world(world);
                return NULL;
            }
        }
    }
    
    // Read ants data
    for (int i = 0; i < colony_count; i++) {
        Colony* colony = &world->colonies[i];
        
        while (1) {
            int ant_id;
            if (fread(&ant_id, sizeof(int), 1, file) != 1) {
                print_error("Failed to read ant ID");
                fclose(file);
                destroy_world(world);
                return NULL;
            }
            
            if (ant_id == -1) break; // End marker
            
            // Read ant data
            Position pos, last_pos;
            uint8_t state;
            int colony_id;
            float energy;
            int food_carrying, steps_taken, food_delivered;
            
            if (fread(&pos, sizeof(Position), 1, file) != 1 ||
                fread(&last_pos, sizeof(Position), 1, file) != 1 ||
                fread(&state, sizeof(uint8_t), 1, file) != 1 ||
                fread(&colony_id, sizeof(int), 1, file) != 1 ||
                fread(&energy, sizeof(float), 1, file) != 1 ||
                fread(&food_carrying, sizeof(int), 1, file) != 1 ||
                fread(&steps_taken, sizeof(int), 1, file) != 1 ||
                fread(&food_delivered, sizeof(int), 1, file) != 1) {
                print_error("Failed to read ant data");
                fclose(file);
                destroy_world(world);
                return NULL;
            }
            
            // Create and add ant
            Ant* ant = create_ant(ant_id, colony_id, pos);
            if (ant != NULL) {
                ant->last_pos = last_pos;
                ant->state = state;
                ant->energy = energy;
                ant->food_carrying = food_carrying;
                ant->steps_taken = steps_taken;
                ant->food_delivered = food_delivered;
                add_ant_to_colony(colony, ant);
            }
        }
    }
    
    fclose(file);
    print_info("Simulation loaded from %s", filename);
    return world;
}

// Statistics and data export
int save_statistics(const World* world, const char* filename) {
    if (world == NULL || filename == NULL) {
        return FILE_IO_ERROR_INVALID_FORMAT;
    }
    
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        print_error("Failed to open statistics file");
        return FILE_IO_ERROR_OPEN;
    }
    
    // Get current timestamp
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // Write CSV header if file is empty
    if (ftell(file) == 0) {
        fprintf(file, "Timestamp,Step,Colony,Food_Collected,Total_Ants,Active_Ants,Efficiency\n");
    }
    
    // Write statistics for each colony
    for (int i = 0; i < world->colony_count; i++) {
        Colony* colony = &world->colonies[i];
        fprintf(file, "%s,%d,%d,%d,%d,%d,%.2f\n",
                timestamp,
                world->current_step,
                colony->id,
                colony->food_collected,
                colony->total_ants,
                colony->active_ants,
                colony->efficiency_score);
    }
    
    fclose(file);
    return FILE_IO_SUCCESS;
}

int export_map(const World* world, const char* filename) {
    if (world == NULL || filename == NULL) {
        return FILE_IO_ERROR_INVALID_FORMAT;
    }
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        print_error("Failed to open map file for writing");
        return FILE_IO_ERROR_OPEN;
    }
    
    // Write header
    fprintf(file, "# Ant Colony Optimization Simulator - Map Export\n");
    fprintf(file, "# Dimensions: %dx%d\n", world->width, world->height);
    fprintf(file, "# Colonies: %d\n", world->colony_count);
    fprintf(file, "# Generated: %s\n", __DATE__);
    fprintf(file, "\n");
    
    // Write legend
    fprintf(file, "# Legend:\n");
    fprintf(file, "# N = Nest (colony home)\n");
    fprintf(file, "# F = Food source\n");
    fprintf(file, "# # = Wall/Obstacle\n");
    fprintf(file, "# . = Empty space\n");
    fprintf(file, "\n");
    
    // Write map
    for (int y = 0; y < world->height; y++) {
        for (int x = 0; x < world->width; x++) {
            Cell* cell = &world->grid[y][x];
            char symbol;
            
            switch (cell->terrain) {
                case TERRAIN_NEST: symbol = 'N'; break;
                case TERRAIN_FOOD: symbol = 'F'; break;
                case TERRAIN_WALL: symbol = '#'; break;
                case TERRAIN_WATER: symbol = '~'; break;
                default: symbol = '.'; break;
            }
            
            fprintf(file, "%c", symbol);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    print_info("Map exported to %s", filename);
    return FILE_IO_SUCCESS;
}

int load_map(World* world, const char* filename) {
    if (world == NULL || filename == NULL) {
        return FILE_IO_ERROR_INVALID_FORMAT;
    }
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        print_error("Failed to open map file for reading");
        return FILE_IO_ERROR_OPEN;
    }
    
    char line[256];
    int line_number = 0;
    int y = 0;
    
    while (fgets(line, sizeof(line), file) && y < world->height) {
        line_number++;
        
        // Skip comment lines and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        // Parse map line
        int x = 0;
        for (int i = 0; line[i] != '\0' && line[i] != '\n' && line[i] != '\r' && x < world->width; i++) {
            char symbol = line[i];
            
            if (symbol == 'N') {
                // Find which colony this nest belongs to
                for (int c = 0; c < world->colony_count; c++) {
                    if (world->colonies[c].nest_pos.x == x && world->colonies[c].nest_pos.y == y) {
                        world->grid[y][x].terrain = TERRAIN_NEST;
                        world->grid[y][x].colony_id = c;
                        break;
                    }
                }
            } else if (symbol == 'F') {
                world->grid[y][x].terrain = TERRAIN_FOOD;
                world->grid[y][x].food_amount = 50; // Default food amount
            } else if (symbol == '#') {
                world->grid[y][x].terrain = TERRAIN_WALL;
            } else if (symbol == '~') {
                world->grid[y][x].terrain = TERRAIN_WATER;
            } else {
                world->grid[y][x].terrain = TERRAIN_EMPTY;
            }
            
            x++;
        }
        
        y++;
    }
    
    fclose(file);
    print_info("Map loaded from %s", filename);
    return FILE_IO_SUCCESS;
}

// File validation and error handling
int validate_save_file(const char* filename) {
    if (filename == NULL) return 0;
    
    FILE* file = fopen(filename, "rb");
    if (file == NULL) return 0;
    
    // Check header
    char header[32];
    if (fread(header, sizeof(char), strlen(SAVE_FILE_HEADER), file) != strlen(SAVE_FILE_HEADER)) {
        fclose(file);
        return 0;
    }
    
    int valid = (strncmp(header, SAVE_FILE_HEADER, strlen(SAVE_FILE_HEADER)) == 0);
    fclose(file);
    
    return valid;
}

int create_backup_save(const char* filename) {
    if (filename == NULL) return FILE_IO_ERROR_INVALID_FORMAT;
    
    char backup_name[512];
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    
    // Create backup filename with timestamp
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", timeinfo);
    snprintf(backup_name, sizeof(backup_name), "%s.backup_%s", filename, timestamp);
    
    // Copy file
    FILE* source = fopen(filename, "rb");
    if (source == NULL) return FILE_IO_ERROR_OPEN;
    
    FILE* backup = fopen(backup_name, "wb");
    if (backup == NULL) {
        fclose(source);
        return FILE_IO_ERROR_OPEN;
    }
    
    char buffer[1024];
    size_t bytes_read;
    int success = 1;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, 1, bytes_read, backup) != bytes_read) {
            success = 0;
            break;
        }
    }
    
    fclose(source);
    fclose(backup);
    
    if (success) {
        print_info("Backup created: %s", backup_name);
        return FILE_IO_SUCCESS;
    } else {
        print_error("Failed to create backup");
        return FILE_IO_ERROR_WRITE;
    }
}
