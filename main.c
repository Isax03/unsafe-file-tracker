#include "file_tracker.h"
#include <getopt.h>
#include <signal.h>

// Variabile globale per gestire l'interruzione del programma
static volatile int running = 1;

// Handler per SIGINT (Ctrl+C)
void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\nReceived SIGINT, shutting down gracefully...\n");
        running = 0;
    }
}

/*
 * Funzione principale
 * BUG INTENZIONALE: Vari problemi di gestione memoria e buffer
 */
int main(int argc, char *argv[]) {
    char *directory = NULL;
    char *log_file = "tracker.log";
    int interval = 5;
    int verbose = 0;
    
    // BUG: Buffer overflow potenziale in array di pattern
    char patterns[5][64];  // Array fisso per pattern
    int pattern_count = 0;
    
    // Setup signal handler
    signal(SIGINT, signal_handler);
    
    // Parsing argomenti da riga di comando
    int opt;
    while ((opt = getopt(argc, argv, "l:p:i:vh")) != -1) {
        switch (opt) {
            case 'l':
                log_file = optarg;
                break;
            case 'p':
                // BUG: Out-of-bounds write - non verifica i limiti dell'array
                if (pattern_count < 5) {
                    strcpy(patterns[pattern_count], optarg);  // VULNERABILITY: Potential Buffer Overflow
                    pattern_count++;
                } else {
                    // VULNERABILITY: Out-of-Bounds Write se pattern_count >= 5
                    strcpy(patterns[pattern_count], optarg);
                    pattern_count++;
                }
                break;
            case 'i':
                interval = atoi(optarg);
                if (interval <= 0) interval = 5;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Verifica che sia stata specificata una directory
    if (optind >= argc) {
        fprintf(stderr, "Error: No directory specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    directory = argv[optind];
    
    if (verbose) {
        printf("Starting file tracker...\n");
        printf("Directory: %s\n", directory);
        printf("Log file: %s\n", log_file);
        printf("Interval: %d seconds\n", interval);
    }
    
    // Inizializza il tracker
    file_tracker_t *tracker = tracker_init(directory, log_file);
    if (!tracker) {
        fprintf(stderr, "Error: Failed to initialize tracker\n");
        return 1;
    }
    
    // Aggiunge i pattern specificati
    for (int i = 0; i < pattern_count; i++) {
        tracker_add_pattern(tracker, patterns[i]);
        if (verbose) {
            printf("Added pattern: %s\n", patterns[i]);
        }
    }
    
    // Scansione iniziale
    if (verbose) printf("Performing initial scan...\n");
    if (tracker_scan_directory(tracker) != 0) {
        fprintf(stderr, "Error: Failed to scan directory\n");
        tracker_destroy(tracker);
        return 1;
    }
    
    if (verbose) {
        printf("Initial scan complete. Monitoring %d files.\n", tracker->file_count);
    }
    
    // Loop principale di monitoraggio
    while (running) {
        sleep(interval);
        
        if (verbose) printf("Checking for changes...\n");
        
        // Controlla modifiche ai file esistenti
        tracker_check_changes(tracker);
        
        // Scansiona per nuovi file
        tracker_scan_directory(tracker);
        
        if (verbose) {
            printf("Currently monitoring %d files.\n", tracker->file_count);
        }
    }
    
    // Cleanup
    if (verbose) printf("Cleaning up...\n");
    tracker_destroy(tracker);
    
    // BUG: Use-after-free - usa tracker dopo averlo distrutto
    if (verbose) {
        printf("Monitored %d files in total.\n", tracker->file_count);  // VULNERABILITY: Use-After-Free
    }
    
    printf("File tracker stopped.\n");
    return 0;
}
