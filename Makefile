CC=gcc
CFLAGS=-std=c99 -Wall
CFLAGS_DEBUG=-g -O0 -std=c99 -Wall
CFLAGS_ASAN=-fsanitize=address -g -O0 -std=c99 -Wall
CFLAGS_STATIC=-Wall -Wextra -Wpedantic -Wformat-security -std=c99

SOURCES=main.c tracker_core.c utils.c
TARGET=build/file_tracker
TARGET_DEBUG=build/file_tracker_debug
TARGET_ASAN=build/file_tracker_asan

.PHONY: all clean debug asan static-analysis test

# Build normale (con bug)
all: $(TARGET)

$(TARGET): $(SOURCES) file_tracker.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

# Build per debugging
debug: $(TARGET_DEBUG)

$(TARGET_DEBUG): $(SOURCES) file_tracker.h
	$(CC) $(CFLAGS_DEBUG) -o $(TARGET_DEBUG) $(SOURCES)

# Build con AddressSanitizer
asan: $(TARGET_ASAN)

$(TARGET_ASAN): $(SOURCES) file_tracker.h
	$(CC) $(CFLAGS_ASAN) -o $(TARGET_ASAN) $(SOURCES)

# Analisi statica
static-analysis:
	@echo "=== Running Cppcheck ==="
	cppcheck --enable=all --inconclusive --std=c99 $(SOURCES)
	@echo "\n=== Running GCC with extra warnings ==="
	$(CC) $(CFLAGS_STATIC) -c $(SOURCES)
	@echo "\n=== Running Clang Static Analyzer ==="
	clang --analyze $(SOURCES) 2>/dev/null || echo "Clang static analyzer not available"

# Test con Valgrind
test-valgrind: debug
	@echo "=== Testing with Valgrind ==="
	mkdir -p test_dir
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET_DEBUG) test_dir

# Test con AddressSanitizer
test-asan: asan
	@echo "=== Testing with AddressSanitizer ==="
	mkdir -p test_dir
	./$(TARGET_ASAN) test_dir

# Test per trigger dei bug
test-bugs: $(TARGET)
	@echo "=== Testing buffer overflow with long path ==="
	-./$(TARGET) $(shell python3 -c "print('A' * 1000)") 2>/dev/null || echo "Crashed as expected"
	@echo "\n=== Testing out-of-bounds with many patterns ==="
	-./$(TARGET) -p "*.a" -p "*.b" -p "*.c" -p "*.d" -p "*.e" -p "*.f" -p "*.g" test_dir 2>/dev/null || echo "May have crashed"

# Cleanup
clean:
	rm -f $(TARGET) $(TARGET_DEBUG) $(TARGET_ASAN)
	rm -f *.o *.plist
	rm -rf test_dir
	rm -f tracker.log changes.log

# Help
help:
	@echo "Available targets:"
	@echo "  all            - Build normal version (with bugs)"
	@echo "  debug          - Build debug version"
	@echo "  asan           - Build with AddressSanitizer"
	@echo "  static-analysis- Run static analysis tools"
	@echo "  test-valgrind  - Test with Valgrind"
	@echo "  test-asan      - Test with AddressSanitizer"
	@echo "  test-bugs      - Test scenarios that trigger bugs"
	@echo "  clean          - Remove all build artifacts"
	@echo "  help           - Show this help"
