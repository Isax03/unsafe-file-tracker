# File Tracker - Caso Studio Memory Safety

> [!WARNING]
> Questo progetto (così come la documentazione) è stato generato con GitHub Copilot e revisionato successivamente. Contiene bug intenzionali per scopi educativi e di analisi della memory safety in C.
> Il primo commit rappresenta la versione originale generata da Copilot (che può contenere errori in qualsiasi punto del progetto) con poche modifiche, dal secondo commit in poi saranno presenti modifiche al codice e alla documentazione per integrare con nuovi test e analisi.

## Descrizione
Questo progetto implementa un file tracker in C che monitora le modifiche ai file in una directory specificata. Il codice contiene intenzionalmente diversi bug legati alla memory safety per scopi educativi e di analisi.

## Compilazione

### Versione con bug (per analisi)
```bash
gcc -o file_tracker main.c tracker_core.c utils.c -std=c99
```

### Versione per debug con simboli
```bash
gcc -g -O0 -o file_tracker_debug main.c tracker_core.c utils.c -std=c99
```

### Versione con AddressSanitizer
```bash
gcc -fsanitize=address -g -O0 -o file_tracker_asan main.c tracker_core.c utils.c -std=c99
```

## Utilizzo

```bash
# Monitoraggio di base
./file_tracker /path/to/directory

# Con opzioni avanzate
./file_tracker -l changes.log -p '*.c' -p '*.txt' -i 3 -v /path/to/directory
```

### Opzioni
- `-l <logfile>`: Specifica il file di log (default: tracker.log)
- `-p <pattern>`: Aggiunge un pattern di file da monitorare (es. *.c, *.txt)
- `-i <interval>`: Intervallo di monitoraggio in secondi (default: 5)
- `-v`: Output verboso
- `-h`: Mostra l'aiuto

## Bug Intenzionali Implementati

### 1. Buffer Overflow (BO)
- **File**: `tracker_core.c`, funzione `tracker_init()`
- **Linea**: ~15-20
- **Descrizione**: `strcpy()` senza controllo della lunghezza del buffer di destinazione

### 2. Double Free (DF)
- **File**: `tracker_core.c`, funzione `tracker_destroy()`
- **Linea**: ~45
- **Descrizione**: `free()` chiamato due volte sulla stessa memoria

### 3. Use-After-Free (UAF)
- **File**: `tracker_core.c`, funzione `tracker_check_changes()`
- **Linea**: ~125
- **Descrizione**: Accesso a memoria dopo `free()`
- **File**: `main.c`, funzione `main()`
- **Linea**: ~145
- **Descrizione**: Accesso al tracker dopo la distruzione

### 4. Out-of-Bounds Write (OOB)
- **File**: `tracker_core.c`, funzione `tracker_add_pattern()`
- **Linea**: ~155
- **Descrizione**: Scrittura oltre i limiti dell'array senza controlli
- **File**: `main.c`, funzione `main()`
- **Linea**: ~55-65
- **Descrizione**: Scrittura in array fisso senza controllo dei limiti

### 5. Out-of-Bounds Read (OOB)
- **File**: `utils.c`, funzione `file_matches_pattern()`
- **Linea**: ~35
- **Descrizione**: Lettura potenziale oltre i limiti della stringa

### 6. Memory Leak
- **File**: `utils.c`, funzione `file_info_create()`
- **Linea**: ~15
- **Descrizione**: Memoria allocata non liberata in caso di errore

### 7. Buffer Overflow nel logging
- **File**: `tracker_core.c`, funzione `tracker_log_event()`
- **Linea**: ~175
- **Descrizione**: Buffer troppo piccolo per contenere la stringa formattata

## Strumenti di Analisi Consigliati

### Analisi Statica
```bash
# Cppcheck
cppcheck --enable=all --inconclusive --std=c99 *.c

# Clang Static Analyzer
clang --analyze *.c

# GCC con warning avanzati
gcc -Wall -Wextra -Wpedantic -Wformat-security -Wstack-protector *.c
```

### Analisi Dinamica
```bash
# Valgrind (Memory errors)
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./file_tracker /tmp

# AddressSanitizer
./file_tracker_asan /tmp

# GDB per debugging
gdb ./file_tracker_debug
```

### Test case per trigger dei bug
```bash
# Per buffer overflow: usare path molto lunghi
./file_tracker $(python -c "print('A' * 1000)")

# Per use-after-free: interrompere durante il monitoraggio
./file_tracker -v /tmp &
# Creare/rimuovere file rapidamente
touch /tmp/test_file && rm /tmp/test_file

# Per out-of-bounds: aggiungere molti pattern
./file_tracker -p "*.a" -p "*.b" -p "*.c" -p "*.d" -p "*.e" -p "*.f" /tmp
```

## Struttura del Progetto

```
file_tracker/
├── file_tracker.h     # Header con definizioni strutture e funzioni
├── main.c            # Funzione principale e parsing argomenti
├── tracker_core.c    # Logica principale del tracking
├── utils.c           # Funzioni di utilità
├── README.md         # Questa documentazione
└── Makefile          # (Opzionale) Script di build
```

## Note per la Tesi

Questo codice è progettato per essere:
1. **Realistico**: Simula un'applicazione reale con gestione file, logging, parsing argomenti
2. **Complesso**: Sufficientemente articolato per analisi approfondite
3. **Vulnerabile**: Contiene bug tipici del C legacy
4. **Educativo**: Ogni bug è documentato e facilmente identificabile

Il progetto può essere utilizzato per dimostrare:
- Efficacia di tool statici vs dinamici
- Differenze nell'output dei vari analyzer
- Processo di fixing e validazione
- Integrazione di security checks nell'SDLC
