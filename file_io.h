#ifndef FILE_IO_H
#define FILE_IO_H

#include "data_structures.h"

// Save and load simulation
int save_simulation(const World* world, const char* filename);
World* load_simulation(const char* filename);

// Statistics and data export
int save_statistics(const World* world, const char* filename);
int export_map(const World* world, const char* filename);
int load_map(World* world, const char* filename);

// File validation and error handling
int validate_save_file(const char* filename);
int create_backup_save(const char* filename);

// File format constants
#define SAVE_FILE_VERSION "1.0"
#define SAVE_FILE_HEADER "ACO_SIM"
#define MAX_FILENAME_LENGTH 256

// Error codes
#define FILE_IO_SUCCESS 0
#define FILE_IO_ERROR_OPEN -1
#define FILE_IO_ERROR_WRITE -2
#define FILE_IO_ERROR_READ -3
#define FILE_IO_ERROR_INVALID_FORMAT -4
#define FILE_IO_ERROR_MEMORY -5

#endif // FILE_IO_H
