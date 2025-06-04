# File Tracker - Caso Studio Memory Safety

> [!WARNING]
> Questo progetto (così come la documentazione) è stato generato con GitHub Copilot e revisionato successivamente. Contiene bug intenzionali per scopi educativi e di analisi della memory safety in C.
> Il primo commit rappresenta la versione originale generata da Copilot (che può contenere errori in qualsiasi punto del progetto) con poche modifiche, dal secondo commit in poi saranno presenti modifiche al codice e alla documentazione per integrare con nuovi test e analisi.

## Descrizione

Questo progetto implementa un file tracker in C che monitora le modifiche ai file in una directory specificata. Il codice contiene intenzionalmente diversi bug legati alla memory safety per scopi educativi e di analisi.

## Compilazione

### Versione con bug (per analisi)

```bash
gcc -o file_tracker main.c tracker_core.c utils.c -std=c11 ./lib/xmalloc/xmalloc.c -lsafestring_shared
```

### Versione per debug con simboli

```bash
gcc -g -O0 -o file_tracker_debug main.c tracker_core.c utils.c -std=c11 ./lib/xmalloc/xmalloc.c -lsafestring_shared
```

### Versione con AddressSanitizer

```bash
gcc -fsanitize=address -g -O0 -o file_tracker_asan main.c tracker_core.c utils.c -std=c11 ./lib/xmalloc/xmalloc.c -lsafestring_shared
```

## Utilizzo

```bash
# Monitoraggio di base
./file_tracker /path/to/directory

# Con opzioni avanzate
./file_tracker -l changes.log -p '*.c' -p '*.txt' -i 3 -v /path/to/directory
```

### Opzioni

-   `-l <logfile>`: Specifica il file di log (default: tracker.log)
-   `-p <pattern>`: Aggiunge un pattern di file da monitorare (es. _.c, _.txt)
-   `-i <interval>`: Intervallo di monitoraggio in secondi (default: 5)
-   `-v`: Output verboso
-   `-h`: Mostra l'aiuto

## Bug Intenzionali Implementati

### 1. Out-of-Bounds Write

-   **File**: `main.c`
-   **Linee**: 27, 42
-   **Descrizione**: Se i pattern sono più di 5 o lunghi più di 63 caratteri, la copia della stringa va oltre i limiti dell'array

### 2. Use-After-Free (UAF)

-   **File**: `main.c`
-   **Linea**: 127
-   **Descrizione**: La variabile `tracker` viene usata dopo la sua distruzione

### 3. Memory Leak

-   **File**: `utils.c`
-   **Linee**: 14-18
-   **Descrizione**: Se `malloc()` di `info->path` fallisce, `info` non viene liberata ma la funzione termina

### 4. Out-of-Bounds Read

-   **File**: `utils.c`
-   **Linea**: 55
-   **Descrizione**: Se il nome del file è più corto dell'estensione con cui viene confrontato, la lettura può uscire dai limiti della stringa

### 5. Dangling Pointer

-   **File**: `utils.c`
-   **Linee**: 69-76
-   **Descrizione**: Nel contesto attuale non particolarmente rilevante, ma in casi di concorrenza può portare all'uso di valori errati

### 6. Buffer Overflow

-   **File**: `tracker_core.c`
-   **Linee**: 14, 18
-   **Descrizione**: Scrittura di argomenti da linea di comando in buffer limitati

### 7. Double Free

-   **File**: `tracker_core.c`
-   **Linea**: 45
-   **Descrizione**: Evidente doppia deallocazione di `tracker->monitored_directory`

### 8. Dangling Pointer

-   **File**: `tracker_core.c`
-   **Linea**: 56
-   **Descrizione**: Dopo la deallocazione, il puntatore `tracker` dovrebbe essere uguale a `NULL`

### 9. Out-of-Bounds Write

-   **File**: `tracker_core.c`
-   **Linea**: 81
-   **Descrizione**: Copia di stringa in buffer senza controllo della lunghezza

### 10. Use-After-Free (UAF)

-   **File**: `tracker_core.c`
-   **Linea**: 161
-   **Descrizione**: Stampa dell'elemento rimosso dopo la sua deallocazione

### 11. Out-of-Bounds Write

-   **File**: `tracker_core.c`
-   **Linea**: 194
-   **Descrizione**: Copia di stringa in una matrice di caratteri senza controllo della lunghezza (su entrambe le dimensioni)

### 12. Out-of-Bounds Write

-   **File**: `tracker_core.c`
-   **Linea**: 216
-   **Descrizione**: Scrittura di stringa in un buffer troppo piccolo

## Strumenti di Analisi Usati

### Analisi Statica

```bash
# Cppcheck
cppcheck --enable=warning --inconclusive --std=c11 *.c ./lib/xmalloc/xmalloc.c -lsafestring_shared

# Clang Static Analyzer
clang --analyze -Xanalyzer -analyzer-checker=core,security *.c ./lib/xmalloc/xmalloc.c -lsafestring_shared

# Frama-C (GUI)
frama-c-gui -eva *.c
```

### Analisi Dinamica

```bash
# AddressSanitizer
clang -fsanitize=address -g -O0 -std=c11 -o file_tracker_asan main.c tracker_core.c utils.c ./lib/xmalloc/xmalloc.c -lsafestring_shared

./file_tracker_asan /tmp

# Valgrind (Memory errors) - eseguito sulla versione debug
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./file_tracker_debug /tmp

```

### Test cases tratati nella tesi

#### Buffer Overflow (`tracker_core.c:14,18`)

```bash
# Creazione di una directory annidata
mkdir -p $(printf 'test/test1/test2/%.0s' {1..15}) # crea 45 directory annidate

# ADDRESS SANITIZER #
# ----------------- #
# Esecuzione del file tracker (directory monitorata troppo lunga)
./file_tracker_asan ./$(printf 'test/test1/test2/%.0s' {1..15})
# oppure con un file di log nella directory annidata
./file_tracker_asan ./test/ -l ./$(printf 'test/test1/test2/%.0s' {1..15})/logfile.log


# VALGRIND #
# -------- #
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./file_tracker_debug ./$(printf 'test/test1/test2/%.0s' {1..15})
# oppure
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./file_tracker_debug ./test/ -l ./$(printf 'test/test1/test2/%.0s' {1..15})/logfile.log
```

#### Use-After-Free (`tracker_core.c:161`)

```bash
# Creazione della directory e di un file di test
mkdir test && touch test/test1.txt

# ADDRESS SANITIZER #
# ----------------- #
./file_tracker_asan ./test/
# ! Eliminazione del file appena dopo l'avvio del tracker ! #


# VALGRIND #
# -------- #
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./file_tracker_debug ./test/
# ! Eliminazione del file appena dopo l'avvio del tracker ! #
```

#### Double Free (`tracker_core.c:45`)

```bash
# Creazione della directory di test
mkdir test

# ADDRESS SANITIZER #
# ----------------- #
# Esecuzione del file tracker
./file_tracker_asan ./test/
# ! Terminazione del tracker con Ctrl+C ! #


# VALGRIND #
# -------- #
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./file_tracker_debug ./test/
# ! Terminazione del tracker con Ctrl+C ! #
```

## Note (Copilot-Generated)

Questo codice è progettato per essere:

1. **Realistico**: Simula un'applicazione reale con gestione file, logging, parsing argomenti
2. **Complesso**: Sufficientemente articolato per analisi approfondite
3. **Vulnerabile**: Contiene bug tipici del C legacy
4. **Educativo**: Ogni bug è documentato e facilmente identificabile

Il progetto può essere utilizzato per dimostrare:

-   Efficacia di tool statici vs dinamici
-   Differenze nell'output dei vari analyzer
-   Processo di fixing e validazione
-   Integrazione di security checks nell'SDLC
