CC := cc
CFLAGS := -Wall -Wextra -Werror -Wno-override-init
BUILD_DIR := ./build

.DEFAULT_GOAL := $(BUILD_DIR)/nesparser

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/nesparser: nesparser.c cut.h nes.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -ggdb -o $@ $<

game.c: $(BUILD_DIR)/nesparser
	$(BUILD_DIR)/nesparser > $@

game.o: game.c neslib.h
	$(CC) $(CFLAGS) -ggdb -Wno-unused -Wno-implicit-fallthrough -c -o $@ $<

$(BUILD_DIR)/out: main.c game.o neslib.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -ggdb -Wno-unused -Wno-implicit-fallthrough -lraylib -o $@ game.o $<

.PHONY: run
run: $(BUILD_DIR)/out
