# RPNCE

An RPN calculator for the TI-84 Plus CE, written in C.

## Status

An RPN calculator with an always-on menu tree in the style of RPN83P. See [docs/ROADMAP.md](docs/ROADMAP.md) and the [key layout](docs/KEYS.md).

## Building

1. Install the [CE C/C++ toolchain](https://github.com/CE-Programming/toolchain/releases)
   (developed against v15.0).
2. Run `make` in this folder.
3. The program is written to `bin/RPNCE.8xp`.

## Running

- In the [CEmu](https://ce-programming.github.io/CEmu/) emulator: drag
  `bin/RPNCE.8xp` (and the toolchain's `clibs.8xg`) onto the emulator.
- On a calculator: send `RPNCE.8xp` and `clibs.8xg` with TI Connect CE.
  Calculators on OS 5.5 or newer need the arTIfiCE jailbreak to run C programs.

## Layout

```
src/main.c   entry point and main loop
src/core/    calculator logic (stack, math, modes); no screen or key code
src/ui/      display, keypad, and menu handling for the CE
src/sys/     saving and loading state
menus/       menus.txt: the whole menu tree as editable text
tools/       genmenu.py: turns menus.txt into src/ui/menu_data.c/.h
             genunits.py: builds the unit tables from exact definitions
tests/       tests that run on a PC (see each file for the command): format,
             rounding, TVM and units, stats, math, BASE, and the core
docs/        roadmap, key layout, and design notes
```

Keeping `core/` free of hardware code means it can later be tested on a PC.

## Editing menus and units

Change `menus/menus.txt`, then run `python3 tools/genmenu.py` (and
`python3 tools/genunits.py` if units changed) and rebuild. The generated
files are committed, so building doesn't need Python. genunits.py needs the
`mpmath` package.

## License

MIT. See [LICENSE](LICENSE).
