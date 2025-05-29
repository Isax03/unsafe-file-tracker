#include "file_tracker.h"

/*
 * Crea una nuova struttura file_info
 * BUG INTENZIONALE: Memory leak - non libera memoria in caso di errore
 */
file_info_t* file_info_create(const char* path, time_t modified, off_t size) {
    if (!path) return NULL;

    file_info_t* info = malloc(sizeof(file_info_t));
    if (!info) return NULL;

    // BUG: Memory leak - se malloc fallisce, la memoria di info non viene liberata
    info->path = malloc(strlen(path) + 1);
    if (!info->path) {
        // VULNERABILITY: Memory Leak - info non viene liberata
        return NULL;
    }

    strcpy(info->path, path);
    info->last_modified = modified;
    info->size = size;
    info->next = NULL;

    return info;
}

/*
 * Distrugge una struttura file_info
 */
void file_info_destroy(file_info_t* info) {
    if (!info) return;

    free(info->path);
    free(info);
}

/*
 * Verifica se un filename corrisponde a un pattern
 * BUG INTENZIONALE: Out-of-bounds read durante la comparazione
 */
int file_matches_pattern(const char* filename, const char* pattern) {
    if (!filename || !pattern) return 0;

    // Implementazione semplificata per pattern con wildcard *
    if (pattern[0] == '*') {
        int filename_len = strlen(filename);
        
        // Pattern tipo *.txt
        const char* extension = pattern + 1;
        int ext_len = strlen(extension);

        // BUG: Out-of-bounds read - non verifica se filename è abbastanza lungo
        if (filename_len >= ext_len) {
            // VULNERABILITY: Potential Out-of-Bounds Read
            return strcmp(filename + filename_len - ext_len, extension) == 0;
        }
        return 0;
    }
    else {
        // Pattern esatto
        return strcmp(filename, pattern) == 0;
    }
}

/*
 * Formatta un timestamp in stringa leggibile
 * BUG INTENZIONALE: Dangling pointer - restituisce puntatore a memoria locale
 */
char* format_timestamp(time_t timestamp) {
    // BUG: Dangling pointer - restituisce puntatore a array locale
    static char buffer[64];  // Dovrebbe essere allocato dinamicamente o gestito diversamente
    struct tm* tm_info = localtime(&timestamp);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

    // VULNERABILITY: Returning pointer to static local array
    // In un uso concorrente, questo può causare problemi
    return buffer;
}

/*
 * Stampa le informazioni di utilizzo del programma
 */
void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS] <directory>\n", program_name);
    printf("File Tracker - Monitor file changes in a directory\n\n");
    printf("Options:\n");
    printf("  -l <logfile>    Specify log file (default: tracker.log)\n");
    printf("  -p <pattern>    Add file pattern to monitor (e.g., *.c, *.txt)\n");
    printf("  -i <interval>   Monitoring interval in seconds (default: 5)\n");
    printf("  -v              Verbose output\n");
    printf("  -h              Show this help\n\n");
    printf("Examples:\n");
    printf("  %s /home/user/documents\n", program_name);
    printf("  %s -l changes.log -p '*.c' -p '*.h' /home/user/project\n", program_name);
}
