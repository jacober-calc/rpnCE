# Native DATE regression checks

The CE v15 compiler miscompiled the private 64-bit `date_days` wrapper,
introducing a spurious bit at position 40. Desktop tests passed because
they use the host compiler and its calling conventions. The native build
reproduced `1987-01-17 + 15000` failing and corrupted date differences.

This suite compiles the real calculator core with release `-Oz` and LTO,
uses TI-OS real-number routines, and checks 83 results in CEmu. It covers
the reported addition, subtraction in both directions, NOWD, scalar-first
addition, leap days, STO arithmetic, epoch and zoned conversions, years 1
and 9999, and the 2038 boundary. Clock checks cover CTZ/ATZ alignment,
independent application time, UTC, fractional offsets and year rollovers,
without writing the hardware clock. Time subtraction is checked in both
operand orders, including conversion of signed seconds to Durations.
Additional checks cover decimal-comma entry formatting, SHOW, arithmetic,
raw DATE records, and backlight adjustment bounds. The backlight and OS
brightness are restored after those checks. A simulated clock is used in
this test. It does not load or write RPNCE's saved-data appvars.

Seven checks exercise the real GraphX display renderer and inspect the
visible LCD buffer: warning glyph height, original horizontal spacing,
unchanged status bar and arrows, and equally sized TVM and stack-clear
confirmations. The stack-clear case executes the real CLEAR commands.
This catches GraphX's clipped-text path ignoring scaling, which desktop
scale-call checks previously missed. Results are buffered outside VRAM
until drawing is finished, then published for the emulator's CRC check.

Additional release checks exercise PRIM, LCM, quantities and registers,
temperature/fuel restrictions, every unit's base round trip, all four
curve-fit models, TVM and VARS. The total is defined in `checks.h`; a
count mismatch fails the suite. See [the release workflow](../../docs/RELEASING.md)
for the consolidated runner and actual-program persistence tests.

With the CE toolchain and `cemu-autotester` on PATH, point these environment
variables to your own ROM and CE libraries:

```sh
export AUTOTESTER_ROM=/path/to/ti84CE.rom
export AUTOTESTER_LIBS_GROUP=/path/to/clibs.8xg
python3 tests/ce_date/run.py
```

For a ROM that requires the arTIfiCE v2 launcher:

```sh
python3 tests/ce_date/run.py /path/to/arTIfiCE_v2.1.8xp
```

The optional launcher runs inside the emulator; it is not installed on a
physical calculator by this script. Use a ROM supported by that launcher.
The test program, resource appvar, and temporary JSON are written under
this directory's ignored `bin/` and `obj/` folders. A passing memory CRC
checks the completed test count and every individual pass flag; it is
independent of screenshots or display formatting.

CEmu's test configuration is documented in its
[autotester reference](https://github.com/CE-Programming/CEmu/blob/master/tests/autotester/json_file_documentation.txt).
