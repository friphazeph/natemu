#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

Cmd cmd;
#define BUILD_DIR "./build/"
#define CFLAGS "-Werror", "-Wall", "-Wextra", "-ggdb"

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
	cmd_append(&cmd, "-o", BUILD_DIR"nesparser");
	cmd_append(&cmd, "nesparser.c");
	if(!cmd_run(&cmd)) return 1;

	if (run) {
		cmd_append(&cmd, BUILD_DIR"nesparser");
		if(!cmd_run(&cmd, .stdout_path=BUILD_DIR"main.c")) return 1;
		cmd_append(&cmd, "cc", CFLAGS, "-I.", "-Wno-implicit-fallthrough");
		cmd_append(&cmd, "-o", BUILD_DIR"out");
		cmd_append(&cmd, BUILD_DIR"main.c");
		if(!cmd_run(&cmd)) return 1;
	}

	return 0;
}
