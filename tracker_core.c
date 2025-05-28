#include "file_tracker.h"

/* 
 * Inizializza il file tracker
 * BUG INTENZIONALE: Possibile buffer overflow in strcpy
 */
file_tracker_t* tracker_init(const char *directory, const char *log_file) {
    file_tracker_t *tracker = malloc(sizeof(file_tracker_t));
    if (!tracker) {
        return NULL;
    }
    
    // BUG: Buffer overflow - non verifica lunghezza stringa
    tracker->monitored_directory = malloc(256);
    strcpy(tracker->monitored_directory, directory);  // VULNERABILITY: Buffer Overflow
    
    // BUG: Possibile buffer overflow anche qui
    tracker->log_file_path = malloc(256);
    strcpy(tracker->log_file_path, log_file);         // VULNERABILITY: Buffer Overflow
    
    tracker->files = NULL;
    tracker->file_count = 0;
    tracker->watch_patterns = malloc(sizeof(char*) * 10);
    tracker->pattern_count = 0;
    
    return tracker;
}

/*
 * Distrugge il tracker e libera la memoria
 * BUG INTENZIONALE: Double free e dangling pointer
 */
void tracker_destroy(file_tracker_t *tracker) {
    if (!tracker) return;
    
    // Libera la lista dei file
    file_info_t *current = tracker->files;
    while (current) {
        file_info_t *next = current->next;
        file_info_destroy(current);
        current = next;
    }
    
    // BUG: Double free - libera due volte la stessa memoria
    free(tracker->monitored_directory);
    free(tracker->monitored_directory);               // VULNERABILITY: Double Free
    
    free(tracker->log_file_path);
    
    // Libera i pattern
    for (int i = 0; i < tracker->pattern_count; i++) {
        free(tracker->watch_patterns[i]);
    }
    free(tracker->watch_patterns);
    
    free(tracker);
    // BUG: Dangling pointer - tracker non viene impostato a NULL
}

/*
 * Scansiona la directory e aggiunge nuovi file alla lista
 * BUG INTENZIONALE: Out-of-bounds write nell'array
 */
int tracker_scan_directory(file_tracker_t *tracker) {
    if (!tracker || !tracker->monitored_directory) {
        return -1;
    }
    
    DIR *dir = opendir(tracker->monitored_directory);
    if (!dir) {
        perror("opendir");
        return -1;
    }
    
    struct dirent *entry;
    char full_path[MAX_PATH_LENGTH];
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;  // Salta file nascosti
        
        // BUG: Out-of-bounds write - non verifica lunghezza
        sprintf(full_path, "%s/%s", tracker->monitored_directory, entry->d_name);
        
        struct stat file_stat;
        if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            
            // Verifica se il file corrisponde ai pattern
            int matches = 0;
            if (tracker->pattern_count == 0) {
                matches = 1;  // Se nessun pattern, monitora tutti i file
            } else {
                for (int i = 0; i < tracker->pattern_count; i++) {
                    if (file_matches_pattern(entry->d_name, tracker->watch_patterns[i])) {
                        matches = 1;
                        break;
                    }
                }
            }
            
            if (matches) {
                // Verifica se il file è già nella lista
                file_info_t *existing = tracker->files;
                int found = 0;
                while (existing) {
                    if (strcmp(existing->path, full_path) == 0) {
                        found = 1;
                        break;
                    }
                    existing = existing->next;
                }
                
                if (!found) {
                    file_info_t *new_file = file_info_create(full_path, 
                                                           file_stat.st_mtime, 
                                                           file_stat.st_size);
                    if (new_file) {
                        new_file->next = tracker->files;
                        tracker->files = new_file;
                        tracker->file_count++;
                        
                        tracker_log_event(tracker, "ADDED", full_path);
                    }
                }
            }
        }
    }
    
    closedir(dir);
    return 0;
}

/*
 * Controlla se ci sono stati cambiamenti nei file monitorati
 * BUG INTENZIONALE: Use-after-free
 */
int tracker_check_changes(file_tracker_t *tracker) {
    if (!tracker) return -1;
    
    file_info_t *current = tracker->files;
    file_info_t *prev = NULL;
    
    while (current) {
        struct stat file_stat;
        
        if (stat(current->path, &file_stat) != 0) {
            // File rimosso
            tracker_log_event(tracker, "REMOVED", current->path);
            
            if (prev) {
                prev->next = current->next;
            } else {
                tracker->files = current->next;
            }
            
            file_info_t *to_remove = current;
            current = current->next;
            
            // BUG: Use-after-free - usa la memoria dopo averla liberata
            file_info_destroy(to_remove);
            printf("Removed file: %s\n", to_remove->path);  // VULNERABILITY: Use-After-Free
            
            tracker->file_count--;
            continue;
        }
        
        // Controlla modifiche
        if (file_stat.st_mtime != current->last_modified) {
            tracker_log_event(tracker, "MODIFIED", current->path);
            current->last_modified = file_stat.st_mtime;
        }
        
        if (file_stat.st_size != current->size) {
            tracker_log_event(tracker, "SIZE_CHANGED", current->path);
            current->size = file_stat.st_size;
        }
        
        prev = current;
        current = current->next;
    }
    
    return 0;
}

/*
 * Aggiunge un pattern di file da monitorare
 * BUG INTENZIONALE: Out-of-bounds access nell'array
 */
void tracker_add_pattern(file_tracker_t *tracker, const char *pattern) {
    if (!tracker || !pattern) return;
    
    // BUG: Out-of-bounds access - non verifica i limiti dell'array
    tracker->watch_patterns[tracker->pattern_count] = malloc(strlen(pattern) + 1);
    strcpy(tracker->watch_patterns[tracker->pattern_count], pattern);
    tracker->pattern_count++;  // VULNERABILITY: Out-of-Bounds Write (può superare 10)
}

/*
 * Registra un evento nel file di log
 * BUG INTENZIONALE: Buffer overflow nel buffer temporaneo
 */
void tracker_log_event(file_tracker_t *tracker, const char *event, const char *filepath) {
    if (!tracker || !event || !filepath) return;
    
    FILE *log_file = fopen(tracker->log_file_path, "a");
    if (!log_file) {
        perror("fopen log file");
        return;
    }
    
    time_t now = time(NULL);
    char *timestamp = format_timestamp(now);
    
    // BUG: Buffer overflow - buffer troppo piccolo per contenere la stringa
    char log_entry[64];  // Buffer troppo piccolo
    sprintf(log_entry, "[%s] %s: %s\n", timestamp, event, filepath);  // VULNERABILITY: Buffer Overflow
    
    fprintf(log_file, "%s", log_entry);
    fclose(log_file);
    
    // Non facciamo free perché format_timestamp restituisce static buffer
}
