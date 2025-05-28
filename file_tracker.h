#ifndef FILE_TRACKER_H
#define FILE_TRACKER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#define MAX_PATH_LENGTH 512
#define MAX_FILES 1000
#define LOG_BUFFER_SIZE 256

// Struttura per rappresentare un file monitorato
typedef struct file_info {
    char *path;                    // Path del file
    time_t last_modified;          // Timestamp ultima modifica
    off_t size;                    // Dimensione file
    struct file_info *next;        // Puntatore al prossimo elemento (lista)
} file_info_t;

// Struttura per il tracker principale
typedef struct {
    file_info_t *files;            // Lista dei file monitorati
    char *log_file_path;           // Path del file di log
    char *monitored_directory;     // Directory monitorata
    int file_count;                // Numero di file attualmente monitorati
    char **watch_patterns;         // Pattern di file da monitorare (es. *.c, *.txt)
    int pattern_count;             // Numero di pattern
} file_tracker_t;

// Funzioni principali
file_tracker_t* tracker_init(const char *directory, const char *log_file);
void tracker_destroy(file_tracker_t *tracker);
int tracker_scan_directory(file_tracker_t *tracker);
int tracker_check_changes(file_tracker_t *tracker);
void tracker_add_pattern(file_tracker_t *tracker, const char *pattern);
void tracker_log_event(file_tracker_t *tracker, const char *event, const char *filepath);

// Funzioni di utilità
file_info_t* file_info_create(const char *path, time_t modified, off_t size);
void file_info_destroy(file_info_t *info);
int file_matches_pattern(const char *filename, const char *pattern);
char* format_timestamp(time_t timestamp);
void print_usage(const char *program_name);

#endif
