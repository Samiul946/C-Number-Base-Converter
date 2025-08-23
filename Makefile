# === Project Settings ===
CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -O2 -Iinclude
BUILD   := build
SRC     := src
TESTS   := tests

# === Targets ===
all: $(BUILD)/converter

# Build the main converter
$(BUILD)/converter: $(SRC)/NumberBaseConverter.c include/NumberBaseConverter.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SRC)/NumberBaseConverter.c

# Build the universal test runner
$(BUILD)/test_converter: $(TESTS)/test_converter.c $(SRC)/NumberBaseConverter.c include/NumberBaseConverter.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(TESTS)/test_converter.c

# Run universal tests (covers bases 2-36, edge cases, invalid inputs)
test: $(BUILD)/test_converter
	@echo "=== Running Universal Base Converter Tests ==="
	@./$(BUILD)/test_converter

# Clean build artifacts
clean:
	rm -rf $(BUILD)

.PHONY: all test clean
