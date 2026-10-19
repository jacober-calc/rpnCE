# Refactor measurements

Measured against original source commit
`08913263794c9ad04d0db0983a2473e9dd6b2d92`, with CE toolchain v15.0 on
macOS ARM, the repository's default `-Oz` flags, LTO and ZX7 compression.
Both versions were built from source with the same toolchain.

| Measurement | Original | Refactor | Saved |
| --- | ---: | ---: | ---: |
| Program image (`.header + .init + .text + .rodata + .data`) | 78,897 bytes | 77,101 bytes | 1,796 bytes (2.3%) |
| Static variables (`.bss`) | 4,799 bytes | 3,327 bytes | 1,472 bytes (30.7%) |
| Compressed `RPNCE.8xp` file | 43,539 bytes | 42,732 bytes | 807 bytes (1.9%) |

The program image and static variables together shrink by 3,268 bytes.
These measurements exclude runtime stack and library allocations.
`RPNCEres.8xv` is unchanged at 11,545 bytes. No release files in `prod/`
were replaced.

## Changes

- Save and load share one scratch buffer instead of keeping two large
  buffers resident throughout the program's lifetime.
- Plain and small dot-font rendering use one drawing loop, with one glyph
  lookup per ordinary character instead of two.
- One compact menu-action table replaces the chains of switches for DATE,
  BASE, TVM, statistics and ordinary commands. Unit indices still come from
  the existing resource file; 2ND recall behavior is preserved.
- The calculator uses the numeric constants and predicates already provided
  by `mathx`, removing duplicate storage and initialization.
- Successful results share the LASTX, unit-tag, stack-drop and lift
  bookkeeping. Statistics retain their distinct lift behavior.
- DATE commands compute quarter-hour operands only when needed. Setting a
  timezone reuses the operand already converted by the command handler.

The numerical algorithms, calculator state layout, saved-state version,
menu IDs and resource format are unchanged. No overall execution-speed
percentage has been measured.

## Verification

Run from the repository root:

```sh
sh tests/run_tests.sh
sh tests/check_core_equivalence.sh
EXTRA_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/check_core_equivalence.sh
make
```

The twelve host suites include golden fingerprints of every menu action
(486 IDs, both plain and shifted), font drawing commands, state persistence,
and tests of LASTX, units and lift rules at stack depths 4 and 12.

The equivalence script compares the old and new cores in the same process
across 23,016 command/state combinations. It compares values, DATE objects,
stack levels, registers, settings, messages, statistics, TVM variables and
clock writes. SEED is excluded because the host number stand-in contains
padding that the calculator's nine-byte number format does not have.
The script requires the baseline commit in the local Git history; the
regular suites also run from a source ZIP.

The original writer/new reader and new writer/original reader were also
cross-checked for state and settings compatibility, including byte-identical
state output on the host fixtures.

Host tests use stand-ins for TI numerical routines. They supplement the
existing decimal formatting/rounding tests and successful calculator builds;
they do not replace testing on a calculator or emulator. Hardware behavior
and emulator timings remain unverified.

To inspect target sections after a build, use the toolchain's
`z80-none-elf-size -A bin/RPNCE.obj` (under `CEdev/binutils/bin/`).
