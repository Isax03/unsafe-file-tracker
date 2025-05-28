#!/bin/bash

# Script per il caso studio della tesi - Analisi Memory Safety
# Esegue una serie completa di test e analisi su file_tracker

echo "=================================================="
echo "File Tracker - Analisi Memory Safety Case Study"
echo "=================================================="

# Colori per l'output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_section() {
	echo -e "\n${BLUE}=== $1 ===${NC}"
}

print_success() {
	echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
	echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
	echo -e "${RED}✗ $1${NC}"
}

# Crea directory di test
mkdir -p test_results; mkdir -p analysis_test_dir

print_section "1. BUILD DEL PROGETTO"

# Build normale
echo "Building normale version..."
make clean > /dev/null 2>&1
if make all > test_results/build.log 2>&1; then
	print_success "Build normale completato"
else
	print_error "Build normale fallito"
	cat test_results/build.log
fi

# Build debug
echo "Building debug version..."
if make debug > test_results/build_debug.log 2>&1; then
	print_success "Build debug completato"
else
	print_error "Build debug fallito"
fi

# Build AddressSanitizer
echo "Building AddressSanitizer version..."
if make asan > test_results/build_asan.log 2>&1; then
	print_success "Build AddressSanitizer completato"
else
	print_warning "Build AddressSanitizer fallito (potrebbe mancare il supporto)"
fi

print_section "2. ANALISI STATICA"

# Cppcheck
echo "Running Cppcheck..."
if command -v cppcheck > /dev/null 2>&1; then
	cppcheck --enable=all --inconclusive --std=c99 --xml *.c 2> test_results/cppcheck.xml
	cppcheck --enable=all --inconclusive --std=c99 *.c > test_results/cppcheck.txt 2>&1
	print_success "Cppcheck completato"
else
	print_warning "Cppcheck non disponibile"
fi

# GCC con warning avanzati
echo "Running GCC static analysis..."
gcc -Wall -Wextra -Wpedantic -Wformat-security -Wstack-protector -std=c99 -c *.c > test_results/gcc_warnings.txt 2>&1
if [ $? -eq 0 ]; then
	print_success "GCC static analysis completato"
else
	print_warning "GCC ha rilevato problemi (atteso)"
fi

# Clang Static Analyzer (se disponibile)
echo "Running Clang Static Analyzer..."
if command -v clang > /dev/null 2>&1; then
	clang --analyze *.c > test_results/clang_static.txt 2>&1
	print_success "Clang Static Analyzer completato"
else
	print_warning "Clang non disponibile"
fi

print_section "3. TEST FUNZIONALE"

# Crea alcuni file di test
echo "Creando file di test..."
echo "Test content 1" > analysis_test_dir/test1.txt
echo "Test content 2" > analysis_test_dir/test2.c
echo "Test content 3" > analysis_test_dir/test3.h

# Test base
echo "Testing basic functionality..."
timeout 10s ./build/file_tracker -v -i 1 analysis_test_dir > test_results/basic_test.txt 2>&1 &
TEST_PID=$!
sleep 3

# Modifica file durante il test
echo "Modified content" >> analysis_test_dir/test1.txt
touch analysis_test_dir/test4.txt
sleep 2

# Termina il test
kill $TEST_PID 2>/dev/null
wait $TEST_PID 2>/dev/null
print_success "Test funzionale base completato"

print_section "4. ANALISI DINAMICA"

# Valgrind
echo "Running Valgrind..."
if command -v valgrind > /dev/null 2>&1; then
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --xml=yes --xml-file=test_results/valgrind.xml timeout 15s ./build/file_tracker_debug -i 1 analysis_test_dir > test_results/valgrind.txt 2>&1 &
	VALGRIND_PID=$!
	sleep 5
	
	# Modifica file per trigger dei bug
	echo "More content" >> analysis_test_dir/test1.txt
	rm analysis_test_dir/test2.c
	sleep 3
	
	kill $VALGRIND_PID 2>/dev/null
	wait $VALGRIND_PID 2>/dev/null
	print_success "Valgrind completato"
else
	print_warning "Valgrind non disponibile"
fi

# AddressSanitizer
echo "Running AddressSanitizer..."
if [ -x "./build/file_tracker_asan" ]; then
	timeout 10s ./build/file_tracker_asan -i 1 analysis_test_dir > test_results/asan.txt 2>&1 &
	ASAN_PID=$!
	sleep 3
	
	# Trigger bug
	echo "Trigger content" >> analysis_test_dir/test1.txt
	sleep 2
	
	kill $ASAN_PID 2>/dev/null
	wait $ASAN_PID 2>/dev/null
	print_success "AddressSanitizer completato"
else
	print_warning "AddressSanitizer binary non disponibile"
fi

print_section "5. TEST DI STRESS E BUG TRIGGERING"

# Test buffer overflow con path lungo
echo "Testing buffer overflow..."
long_path=$(python3 -c "print('A' * 500)")
timeout 5s ./build/file_tracker "$long_path" > test_results/buffer_overflow.txt 2>&1
if [ $? -ne 0 ]; then
	print_success "Buffer overflow rilevato (crash atteso)"
else
	print_warning "Buffer overflow non ha causato crash"
fi

# Test out-of-bounds con molti pattern
echo "Testing out-of-bounds access..."
timeout 5s ./build/file_tracker -p "*.a" -p "*.b" -p "*.c" -p "*.d" -p "*.e" -p "*.f" -p "*.g" -p "*.h" analysis_test_dir > test_results/oob_access.txt 2>&1
if [ $? -ne 0 ]; then
	print_success "Out-of-bounds access rilevato"
else
	print_warning "Out-of-bounds access non rilevato"
fi

# Test memory pressure
echo "Testing memory pressure..."
for i in {1..50}; do
	echo "Content $i" > analysis_test_dir/file_$i.txt
done

timeout 10s ./build/file_tracker -v analysis_test_dir > test_results/memory_pressure.txt 2>&1 &
PRESSURE_PID=$!
sleep 3

# Rimuovi molti file rapidamente
rm analysis_test_dir/file_*.txt

sleep 2
kill $PRESSURE_PID 2>/dev/null
wait $PRESSURE_PID 2>/dev/null

print_success "Test memory pressure completato"

print_section "6. GENERAZIONE REPORT"

cat > test_results/summary_report.txt << EOF
REPORT ANALISI MEMORY SAFETY - FILE TRACKER
==========================================

Data: $(date)
Progetto: File Tracker Case Study

BUGS INTENZIONALI IMPLEMENTATI:
------------------------------
1. Buffer Overflow - tracker_init() (strcpy senza controllo lunghezza)
2. Double Free - tracker_destroy() (doppio free della stessa memoria)
3. Use-After-Free - tracker_check_changes() (uso dopo free)
4. Use-After-Free - main() (accesso dopo distruzione)
5. Out-of-Bounds Write - tracker_add_pattern() (scrittura oltre array)
6. Out-of-Bounds Write - main() (pattern array overflow)
7. Out-of-Bounds Read - file_matches_pattern() (lettura oltre stringa)
8. Memory Leak - file_info_create() (memoria non liberata su errore)
9. Buffer Overflow - tracker_log_event() (buffer troppo piccolo)

STRUMENTI UTILIZZATI:
-------------------
EOF

if command -v cppcheck > /dev/null 2>&1; then
	echo "✓ Cppcheck - Analisi statica" >> test_results/summary_report.txt
else
	echo "✗ Cppcheck - Non disponibile" >> test_results/summary_report.txt
fi

if command -v valgrind > /dev/null 2>&1; then
	echo "✓ Valgrind - Analisi dinamica" >> test_results/summary_report.txt
else
	echo "✗ Valgrind - Non disponibile" >> test_results/summary_report.txt
fi

if [ -x "./build/file_tracker_asan" ]; then
	echo "✓ AddressSanitizer - Analisi runtime" >> test_results/summary_report.txt
else
	echo "✗ AddressSanitizer - Non disponibile" >> test_results/summary_report.txt
fi

echo "✓ GCC Warnings - Analisi statica" >> test_results/summary_report.txt

cat >> test_results/summary_report.txt << EOF

FILES GENERATI:
--------------
EOF

ls -la test_results/ >> test_results/summary_report.txt

print_section "7. CLEANUP"

# Cleanup file di test
rm -rf analysis_test_dir
rm -f tracker.log *.plist *.o

print_success "Analisi completata!"
echo ""
echo "Risultati salvati in: test_results/"
echo "Report principale: test_results/summary_report.txt"
echo ""
echo "Per visualizzare il summary:"
echo "cat test_results/summary_report.txt"
