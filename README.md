# RPNCE

An RPN calculator for the TI-84 Plus CE, written in C.

## Status

Early development: the stack, number entry, and basic math work. See [docs/ROADMAP.md](docs/ROADMAP.md) and the [key layout](docs/KEYS.md).

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
src/ui/      display and keypad handling for the CE
docs/        roadmap and design notes
```

Keeping `core/` free of hardware code means it can later be tested on a PC.

## License

MIT. See [LICENSE](LICENSE).
