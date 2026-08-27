# Natemu

Static recompiler for NES ROMs, written in C.

Instead of interpreting 6502 machine code at runtime, natemu statically analyzes a ROM and translates it directly into C source code. That code is compiled and linked against a `raylib`-based runtime, producing a standalone executable for that specific game.

**Status: experimental.** Expect bugs, missing features, and unsupported games.

## Features

* Static 6502-to-C recompilation
* Standalone output executable per ROM
* `raylib`-based rendering and input

## Prerequisites

* A C compiler (`gcc` or `clang`)
* `make`
* `raylib` — see the [raylib wiki](https://github.com/raysan5/raylib/wiki) for installation instructions on your OS

## Supported Games

⚠️ **Only Mapper 0 (NROM) is supported.** Any ROM using another mapper will fail to build or run correctly.

NROM covers a good chunk of early NES titles (*Super Mario Bros.*, *Donkey Kong*, *Excitebike*, ...). See [this list of Mapper 0 games](https://nesdir.github.io/mapper0.html) for what's compatible.

## Building

```bash
make ROM=path/to/your/game.nes
```

If `ROM` is not specified, it defaults to `test.rom`.

## Usage

```bash
./build/out
```

## Roadmap

* Expanding mapper support
* Audio (APU) support
* Improved CLI and configuration
* Automatic header dependency tracking in the build system

## License

Copyright (C) 2026 Maxime Delhaye

This program is free software: you can redistribute it and/or modify it under the terms of the **GNU General Public License v3.0**. See the `LICENSE` file for details.
