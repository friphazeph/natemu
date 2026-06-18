CC := cc
CFLAGS := -std=c23 -Wpedantic -Wall -Wextra -Werror -Wno-override-init -ggdb
BUILD_DIR := ./build

.DEFAULT_GOAL := $(BUILD_DIR)/nesparser

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/nesparser: nesparser.c cut.h nes.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

generated_main.c: $(BUILD_DIR)/nesparser
	$(BUILD_DIR)/nesparser > $@

$(BUILD_DIR)/out: generated_main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I. -Wno-implicit-fallthrough -o $@ $<

.PHONY: run
run: $(BUILD_DIR)/out
