#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

Cmd cmd;
#define BUILD_DIR "./build/"
#define CFLAGS "-Wall", "-Wextra", "-ggdb"

int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	char *prog = shift(argv, argc);
	bool run = false;
	if (argc > 0) {
		if (strcmp("run", shift(argv, argc)) == 0) {
			run = true;
		}
	}

	mkdir_if_not_exists(BUILD_DIR);
	
	cmd_append(&cmd, "cc", CFLAGS);
	cmd_append(&cmd, "-o", BUILD_DIR"nes");
	cmd_append(&cmd, "nes.c");
	if(!cmd_run(&cmd)) return 1;

	if (run) {
		cmd_append(&cmd, BUILD_DIR"nes");
		if(!cmd_run(&cmd)) return 1;
	}

	return 0;
}
