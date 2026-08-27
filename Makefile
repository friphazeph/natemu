CC        := cc
CFLAGS    := -Wall -Wextra -Werror -Wno-override-init -Wno-unused \
             -I. -I./runtime -I./frontend -I./recompiler
BUILD_DIR := ./build

ROM ?= test.rom

# Recompiler source and object definitions
RECOMP_SRCS  := recompiler/nesparser.c recompiler/nesrom.c recompiler/c_emitter.c
RECOMP_OBJS  := $(patsubst %.c,$(BUILD_DIR)/%.o,$(RECOMP_SRCS))

# Runtime and frontend source and object definitions
RUNTIME_SRCS := runtime/bus.c runtime/ppu.c runtime/runtime.c frontend/main.c
RUNTIME_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(RUNTIME_SRCS))

.DEFAULT_GOAL := $(BUILD_DIR)/out

# Ensure build subdirectories exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR) $(BUILD_DIR)/recompiler $(BUILD_DIR)/runtime $(BUILD_DIR)/frontend

# Generic rule to compile every individual .c file into its own .o
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -ggdb -c -o $@ $<

# Relocatable pre-link: combines main.o and all hardware subsystem .o's into runtime.o
$(BUILD_DIR)/runtime.o: $(RUNTIME_OBJS) | $(BUILD_DIR)
	$(CC) -r -o $@ $^

# 1. Standalone Recompiler Executable: Links recompiler .o files
$(BUILD_DIR)/nesparser: $(RECOMP_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -ggdb -o $@ $^

# 2. Code Generation Step: Runs recompiler executable to produce game.c
$(BUILD_DIR)/game.c: $(BUILD_DIR)/nesparser
	$(BUILD_DIR)/nesparser $(ROM) > $@

# 3. Game Logic Object: Compiles generated game.c into game.o
$(BUILD_DIR)/game.o: $(BUILD_DIR)/game.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -ggdb -Wno-unused -Wno-implicit-fallthrough -c -o $@ $<

# 4. Final Executable: Links combined runtime.o and game.o
$(BUILD_DIR)/out: $(BUILD_DIR)/runtime.o $(BUILD_DIR)/game.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) -ggdb -Wno-unused -Wno-implicit-fallthrough -o $@ $^ -lraylib

.PHONY: run clean

run: $(BUILD_DIR)/out
	./$(BUILD_DIR)/out

clean:
	rm -rf $(BUILD_DIR) game.c
