# Refactor measurements

The first table records the conservative cleanup at `1b6b8c2`, before the
hardware bug fixes and quantity arithmetic described below. It is not the
size of the latest development build.

Measured against original source commit
`08913263794c9ad04d0db0983a2473e9dd6b2d92`, with CE toolchain v15.0 on
macOS ARM, the repository's default `-Oz` flags, LTO and ZX7 compression.
Both versions were built from source with the same toolchain.

| Measurement | Original | Refactor | Saved |
| --- | ---: | ---: | ---: |
| Program image (`.header + .init + .text + .rodata + .data`) | 78,897 bytes | 77,767 bytes | 1,130 bytes (1.4%) |
| Static variables (`.bss`) | 4,799 bytes | 3,577 bytes | 1,222 bytes (25.5%) |
| Compressed `RPNCE.8xp` file | 43,539 bytes | 43,022 bytes | 517 bytes (1.2%) |

The program image and static variables together shrink by 2,352 bytes.
These measurements exclude runtime stack and library allocations.
`RPNCEres.8xv` is unchanged at 11,545 bytes. No release files in `prod/`
were replaced.

## Merge preparation (0.7.0)

Before merging into `dev`, the branch was adjusted as follows. The
old-core comparison and every host suite still pass.

- **Shortcuts:** 2ND LINK (DATE) and 2ND ANGLE (CONV) were removed, leaving
  seven shortcuts. Both keys are kept free for complex-number entry, as in
  RPN83P; `test_keys.c` checks that they do nothing.
- **Register units moved to their own appvar.** `RPNCEdat` is back to the
  exact 0.6.6 layout, with no extension. Register units are saved in
  `RPNCEunt` (written only when a register holds a unit), with a version,
  a checksum, and an FNV-1a fingerprint of the register values the tags
  belong to. A missing, damaged, truncated or mismatched `RPNCEunt` only
  drops the register units; every value still loads. Saves from earlier
  test builds of this branch also retain their register units: when no
  `RPNCEunt` exists, a complete recognized unit footer is read and migrated
  on the next save. A damaged or unknown footer drops only its unit labels,
  leaving numeric and DATE state available. A present `RPNCEunt` takes
  precedence, including its checksum and value-fingerprint validation.
- **HELP moved into the RPNCEres appvar.** The pages are written in
  `help/help.txt` and packed by `tools/genres.py` (resource format 2), which
  checks that each page fits the screen. Page 1 no longer lists the removed
  shortcuts.
- **Version 0.7.0.**

| Measurement | Before merge preparation (`8800256`) | 0.7.0 | Change |
| --- | ---: | ---: | ---: |
| Program image | 82,593 bytes | 82,968 bytes | +375 bytes |
| Static variables (`.bss`) | 3,843 bytes | 3,855 bytes | +12 bytes |
| Compressed `RPNCE.8xp` | 45,976 bytes | 46,287 bytes | +311 bytes |
| `RPNCEres.8xv` | 11,545 bytes | 12,308 bytes | +763 bytes (HELP text) |

The local ZIP integration also preserves earlier refactor register-unit
footers during the move to `RPNCEunt`. The migration shares the unit-file
scratch buffer. It adds 118 bytes to the program image and 40 bytes to the
compressed program compared with the supplied ZIP's build, with no extra
static RAM. Storage regressions and direct old/new writer-reader checks
cover migration with 0, 5 and 100 extra registers.

The HELP text left the program, but the unit-file checks and the HELP
reader were added, so the program is slightly larger overall.

## Hardware bug fixes and quantity support

The current `refactor` development build adds behavior and tint presets to
the cleanup above:

| Measurement | Before bug fixes (`1b6b8c2`) | Current build | Added |
| --- | ---: | ---: | ---: |
| Program image | 77,767 bytes | 82,593 bytes | 4,826 bytes |
| Static variables (`.bss`) | 3,577 bytes | 3,843 bytes | 266 bytes |
| Compressed `RPNCE.8xp` | 43,022 bytes | 45,976 bytes | 2,954 bytes |

`RPNCEres.8xv` remains 11,545 bytes, but its stamp changes because TVM and
HELP moved. Transfer both files from `bin/` together. The release files in
`prod/` remain unchanged.

- PRIM's factor calculation was already correct: 1981 gives 7. PRIM now
  pushes the factor and preserves the original in Y, following RPN83P's
  PRIM / factorization workflow.
- LCM uses `(Y / GCD(Y,X)) * X`, with an explicit zero case, avoiding the
  OS LCM entry point reported to return Y on hardware. The replacement
  passes host regressions; the original OS failure has not been reproduced
  in a calculator emulator.
- Quantities keep the full TI numeric value with unit metadata, extending
  the existing stack representation to ordinary registers. A private
  `calc_quantity` module prepares arithmetic operands and result units;
  incompatible inputs fail before committing results.
- Compatible quantities convert X to Y's unit for +, -, MIN, MAX, MOD and
  %CH. Scalar scaling, compatible ratios, rounding and LASTX preserve the
  appropriate quantity semantics. The exact 2 kWh / 2000 kcal %CH example
  returns approximately 16.222222222222%.
- STO, RCL, register arithmetic, X<> and VARS preserve/display unit values.
  Statistics, TVM and numeric settings reject quantities. DATE and BASE
  operations refuse incompatible quantity operands.
- Register units are an optional extension after the existing v5 save
  payload. Legacy saves load with scalar registers; the original reader
  still reads new saves' numeric and DATE values. Older programs cannot
  retain the new register-unit metadata when they save again.
- VARS lists the seven shared TVM variables, including zero values, and
  always ends with LASTX (also including zero). ENTER
  recalls them through the existing TVM recall command. Catalog capacity
  includes all 140 numbered registers, LASTX and TVM; regression cases check
  live values, zero FV, lift behavior, scrolling and preservation of TVM data.
  A percent glyph makes I%YR readable in the dot font.
- Nine shortcuts were implemented at first (two were later removed; see
  "Merge preparation" above), TVM is on HOME's first row, HELP is on its
  second, and HELP has three reference pages.

The tint update renames DARK to INVERTED and adds AYU DARK, MONOKAI and PASTEL.
Before the subsequent ANGLE shortcut, these presets added 567 bytes to the
image, 3 bytes to static RAM and 371 bytes to the compressed program compared
with the preceding bug-fix build. The
existing four tint indices and their colors are preserved. See
[TINTS.md](TINTS.md) for palettes and preview details. COMMODORE appends a
near-black and phosphor-green option using the same theme roles.

See [HARDWARE_TEST.md](HARDWARE_TEST.md) for transfer and test steps. These
are functional fixes and additions, so this round grows the executable.

## Conservative follow-up

Before the hardware fixes, compared with the first refactor commit `2e29e01`, this round adds 666 bytes
to the program image, 250 bytes of static RAM, and 290 bytes to the compressed
program. It trades a small amount of memory for clearer module boundaries
and less formatting work during redraws.

- Entry editing moved to `calc_entry.c` (280 lines) and DATE integration to
  `calc_date.c` (477 lines). `calc.c` is now 1,644 lines instead of 2,398.
  Extracted function bodies retain their original operations and ordering;
  only helper names and linkage changed. The private headers describe the
  boundaries, while the public `calc.h` API and state structure are unchanged.
- Four stack rows cache their formatted text and value bytes. The cache
  invalidates on changes to display style, digits, radix, word size or DATE
  representation, and whenever the display is reopened. Unit labels,
  warnings, prompts, status text and the catalog continue to use live state.
- Unchanged, modifier, menu and clock redraws make zero numeric formatting
  calls after the first frame, instead of four. Updating only X reformats
  one value. These are measured host call counts, not device timing claims.
- Every frame is still fully drawn and swapped. No partial-frame drawing or
  changes to the main event loop were introduced.

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

The cleanup stage preserved numerical algorithms, calculator state layout,
saved-state version, menu IDs and resource format. The subsequent changes
are described above. No overall execution-speed
percentage has been measured.

## Verification

Run from the repository root:

```sh
sh tests/run_tests.sh
sh tests/check_core_equivalence.sh
EXTRA_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/run_tests.sh
EXTRA_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/check_core_equivalence.sh
make
```

The fifteen host suites include golden fingerprints of every menu action
(486 IDs, both plain and shifted), font and whole-display drawing commands,
state persistence, and tests of LASTX, units and lift rules at stack depths
4 and 12. Display cases cover editing, prompt previews, scrolling, BASE
warnings, DATE formats, catalog navigation and display reopening. Explicit
call-count checks verify cache hits and all formatting dependencies.

The equivalence script compares the old and new cores in the same process
across 22,848 command/state combinations and 8,568 entry-sequence steps.
Entry traces include exponent limits, invalid-record repair, compact
durations, BASE transitions, cancellation and a deterministic mixture of
editing commands. The tests compare values, DATE objects,
stack levels, registers, settings, messages, statistics, TVM variables and
clock writes. The matrix uses scalar X and Y while retaining tags on deeper
stack levels. New quantity semantics and PRIM's stack push are exercised by
`test_fixes.c`; those intentional behavior changes and the corrected Y type
checks for percentage commands are excluded from the old-core comparison.
SEED is excluded because the host number stand-in contains
padding that the calculator's nine-byte number format does not have.
The script requires the baseline commit in the local Git history; the
regular suites also run from a source ZIP. Baseline selection supports
commits before the core was split into multiple files; this follow-up was
also compared directly against `2e29e01`, including under sanitizers.

The original writer/new reader and new writer/original reader were also
cross-checked with 0, 5 and 100 extra registers. New saves preserve the old
numeric/DATE payload and append register-unit metadata. Storage regressions
cover legacy loading, quantity round trips, shrinking register counts,
invalid tags and truncated extensions without partial calculator updates.
The host suites and old-core comparison also pass with AddressSanitizer and
UndefinedBehaviorSanitizer.

Host tests use stand-ins for TI numerical routines. They supplement the
existing decimal formatting/rounding tests and successful calculator builds;
they do not replace testing on a calculator or emulator. Hardware behavior
and emulator timings remain unverified.

To inspect target sections after a build, use the toolchain's
`z80-none-elf-size -A bin/RPNCE.obj` (under `CEdev/binutils/bin/`).
