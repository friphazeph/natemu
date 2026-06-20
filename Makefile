CC := cc
CFLAGS := -Wall -Wextra -Werror -Wno-override-init
BUILD_DIR := ./build

.DEFAULT_GOAL := $(BUILD_DIR)/nesparser

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/nesparser: nesparser.c cut.h nes.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

game.c: $(BUILD_DIR)/nesparser
	$(BUILD_DIR)/nesparser > $@

$(BUILD_DIR)/out: main.c game.c neslib.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I. -Wno-unused -Wno-implicit-fallthrough -lraylib -o $@ $<

.PHONY: run
run: $(BUILD_DIR)/out
