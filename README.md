# RPNCE
![RPNCE Screenshot](assets/rpnce-promo1.png)

An RPN calculator for the TI-84 Plus CE, written in C. It turns the CE into
an HP-style RPN calculator: a 4-level stack (up to 12), numbered registers
in the style of the HP-32SII, and a full menu tree of functions along the
bottom of the screen, on a dot-matrix LCD-style display.

## Features

- **HP RPN stack:** X, Y, Z, T with standard HP stack-lift rules, LASTX,
  and an optional deeper stack of up to 12 levels.
- **Keyboard-first:** the keys do what they say, including their 2ND
  functions; everything else is in the menu row, opened with MATH.
- **Registers:** A-Z, i, indirect (i), the statistics registers, and up to
  100 extra numbered registers, with STO/RCL arithmetic, x<>, and a VARS
  catalog of everything stored.
- **Accurate numbers:** the calculator's 14-digit decimal math, shown with
  up to 12 digits, thousands separators, and FIX/SCI/ENG/ALL formats.
- **Functions:** MATH, NUM, PROB, HYP, CONV (polar, H.MMSS), STAT with
  curve fits, TVM (shared with TI's TVM Solver), 169 unit conversions, and
  BASE (HEX/OCT/BIN, logic, shifts, rotates, bit operations).
- **Remembers everything:** the stack, registers, statistics, and settings
  are saved when you quit and restored next time.
- **Screen tints:** LCD, LCD+ (for real screens), GRAY, and DARK.

See [docs/KEYS.md](docs/KEYS.md) for the full key and menu reference, and
[docs/ROADMAP.md](docs/ROADMAP.md) for what's done and what's next
(complex numbers, dates, and help are still to come).

## Installing

1. Download `RPNCE.8xp` from the releases (or build it, below) and the
   [CE C libraries](https://github.com/CE-Programming/libraries/releases)
   (`clibs.8xg`).
2. Send both to the calculator with TI Connect CE.
3. Run it from the home screen with `Asm(prgmRPNCE)`, or `prgmRPNCE` on
   newer OS versions. Calculators on OS 5.5 or newer need the arTIfiCE
   jailbreak to run C programs.

RPNCE stores its data in two appvars, `RPNCEdat` (calculator state) and
`RPNCEcfg` (display settings). Deleting them resets RPNCE.

## Quick start

| Keys | Result |
| --- | --- |
| `2 ENTER 3 +` | 5 |
| `2 ENTER 3 ÷` then `2ND ENTRY` | shows all 14 digits of 0.666... |
| `5 STO→ MATH`, later `2ND RCL MATH` | stores and recalls register A |
| `MODE`, then `FIX`, `4` | 4 decimal places |
| `MATH`, then a menu key | opens the menu tree; `ON` goes back |
| `2ND MODE` (QUIT) | saves and exits |

## Building

1. Install the [CE C/C++ toolchain](https://github.com/CE-Programming/toolchain/releases)
   (developed against v15.0).
2. Run `make` in this folder.
3. The program is written to `bin/RPNCE.8xp`. It is stored compressed and
   unpacks itself into RAM when started.

To try it without a calculator, load `bin/RPNCE.8xp` and `clibs.8xg` into
the [CEmu](https://ce-programming.github.io/CEmu/) emulator.

## Tests

The calculator logic is kept free of screen and keypad code, so it can be
tested on a PC. With a C compiler installed:

```
sh tests/run_tests.sh
```

This builds and runs seven suites (number formatting, decimal rounding,
math, BASE, statistics, TVM and units, and the calculator core), using
stand-ins for the calculator's number routines.

## Layout

```
src/main.c   entry point and main loop
src/core/    calculator logic: stack, registers, math, formatting, rounding,
             TVM, statistics, units, BASE (no screen or key code)
src/ui/      display, dot-matrix font, keypad, menus, VARS catalog
src/sys/     saving and loading state
menus/       menus.txt: the whole menu tree as editable text
tools/       genmenu.py: turns menus.txt into src/ui/menu_data.c/.h
             genunits.py: builds the unit tables from exact definitions
tests/       PC test suites and run_tests.sh
docs/        key reference and roadmap
```

## Editing menus and units

Change `menus/menus.txt`, then run `python3 tools/genmenu.py` (and
`python3 tools/genunits.py` if units changed) and rebuild. The generated
files are committed, so building doesn't need Python. genunits.py needs the
`mpmath` package.

## License

MIT. See [LICENSE](LICENSE). The menu tree in `menus/menus.txt` carries
its own credit notice.
