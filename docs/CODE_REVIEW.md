# Size and efficiency review before 0.7.0

Reviewed on 21 October 2026 against `dev` at `8c8590b`.
These are optimization candidates, not implemented changes. The priority
is identical calculator behavior, including errors, stack lift, LASTX,
units, display pixels and saved-data compatibility.

Update: the release follow-up removes unused `r_tenth` initialization and
uses read-only curve-fit references. These changes save 314 image bytes
and 89 compressed file bytes, with static RAM unchanged. BASE formatter
experiments were deferred after size comparisons. The repeatable process
and current status are consolidated in [RELEASING.md](RELEASING.md).
The remaining text records the original review rather than current work.

## Assessment

There is worthwhile work left, mostly in formatting and rendering. The
earlier refactor already addressed large sources of repetition: menu
dispatch is table-driven, stack results use common helpers, save/load
share a scratch buffer, and stack text is cached. A broad rewrite is not
justified by the remaining opportunities.

The existing CE linker map reports an 83,249-byte program image and
3,860 bytes of static storage. Its largest static objects are calculator
state (1,658 bytes), storage scratch (1,428 bytes), and the formatted-value
cache (244 bytes). These are existing build measurements, not a fresh
build or predictions of savings. The CE toolchain is not currently on PATH.

## Recommended first changes

### 1. Specialize formatting for HEX, OCT and BIN

Source: [base.c](../src/core/base.c), `base_format()`.

Each output digit currently uses division and remainder by a runtime
radix on a 64-bit word. The existing CE assembly confirms a call to the
64-bit division helper inside the digit loop. These three bases can
extract digits with a mask and shift by 4, 3 or 1 bits.

This is a strong candidate for reducing formatting work without changing
the numeric algorithms. Preserve any general-radix behavior exposed by
the helper. Other functions still require 64-bit arithmetic; this change
will not eliminate the division library from the program.

Verify zero, leading-digit behavior, every supported word width (1-44),
maximum values and representative/random words against the current
formatter in all three bases. Compare target image and compressed size.

### 2. Remove the unused `r_tenth` initialization

Source: [calc.c](../src/core/calc.c), constants and `init_constants()`.

`r_tenth` is declared and assigned but never read. The existing map still
allocates nine bytes to it. `mathx.c` has a separate, actively used
`mx_tenth`; that value must remain. Removing the unused variable and its
calculation is a small cleanup with a direct source-level justification.

Verify the CE map and startup behavior; record actual savings rather
than assuming the program file shrinks by the variable's size.

### 3. Reuse BASE validation during display redraws

Source: [display.c](../src/ui/display.c), `format_value()`,
`stack_value_text()` and `base_warning()`.

Formatting a BASE value validates and converts it with `base_fit()` via
`base_word()`. The warning line independently calls `base_fit()` for X.
Even when cached stack text is reused, the warning path repeats that
numeric work on menu, modifier and clock redraws when no message is set.

A shared validation result could serve both formatting and warnings.
Invalidate it when X, radix or word size changes, and preserve editing,
DATE handling, message precedence and immediate warning updates.
Confirm fewer conversion calls and identical display fingerprints.

## Further candidates that need measurement

### Long-value scrolling and glyph lookup

Sources: [display.c](../src/ui/display.c), `substring_width()` and
`draw_value()`; [dotfont.c](../src/ui/dotfont.c), `find_glyph()`.

Finding a scrolling window repeatedly copies and measures overlapping
substrings. Each measurement searches the glyph table for its characters;
drawing searches it again. Accumulating widths or using a range-measuring
helper would reduce repeated work. A faster glyph lookup is another
candidate, subject to the size cost of its code or lookup table.

Preserve all character widths, unknown-character behavior, decimal marks,
the small exponent E, arrow positions and scroll limits. In particular,
the current substring measurement interprets E relative to the substring
start; a replacement must account for that context. Compare rendering
fingerprints and native screenshots before accepting a change.

### Curve-fit data selection

Source: [stats.c](../src/core/stats.c), `fit_sums()` and its callers.

Each fit selects and copies five `real_t` values into a temporary
structure. Read-only references or an index table could avoid these
copies and simplify the repeated model-selection assignments. This may
reduce temporary stack use, but pointer accesses have a cost too.
Measure target code and stack use; retain the exact arithmetic order,
model checks, error behavior and BEST tie-breaking.

### Repeated setup and conversions

- [units.c](../src/core/units.c) parses factor and offset strings during
  conversions. A small cache might help repeated conversions, at a RAM
  and code-size cost. Pre-encoding the entire table changes the resource
  format and belongs in a later project.
- [display.c](../src/ui/display.c), `clock_text()`, reads and converts the
  clock on every redraw. Cache only if profiling justifies it, with
  invalidation for CTZ, ATZ, SETC and clock/calendar changes.
- [calc.c](../src/core/calc.c), `size_from_x()` and `whole_from_x()`, repeat
  validation, but intentionally differ: one clamps below its minimum and
  the other rejects. Any shared helper must preserve that difference.
- [tvm.c](../src/core/tvm.c), `factors()`, evaluates exp and expm1 for the
  same argument. The large-argument expm1 path evaluates exp again.
  Sharing this work is worth investigating later; small-rate accuracy
  and rounding make it unsuitable for casual consolidation.

## Boundaries for the release cleanup

Keep the existing numerical algorithms, DATE encoding, state/resource
formats and keyboard behavior. Keep the full-precision TI decimal type.
The storage scratch buffer supports validation before applying loaded
state; replacing it with direct loading needs a separate integrity review.
The value cache deliberately spends RAM to avoid repeated formatting.

The program already uses `-Oz`, link-time optimization and compression.
Removing source lines or splitting a switch does not establish a size or
speed improvement. Measure executable image, static RAM, temporary stack,
compressed program and resource file separately, using the same toolchain.
Use native timing or work counters for speed claims.

For each accepted change, run the relevant focused tests and the full
desktop suite, then the core-equivalence and native CE checks as applicable.
Rendering changes also need pixel comparisons; persistence changes need
old/new save compatibility checks. Complete the hardware release checklist
against the final build. Make one focused change per commit so a regression
can be isolated and reverted.

## Review verification

The 16 desktop suites passed on this unchanged source during the preceding
release-readiness assessment. This review also reran the core-equivalence
comparison: all 22,848 command/state cases and 8,568 entry steps matched
the baseline for the behavior covered by that comparison. No native
build, timing benchmark or new hardware execution was performed for this
review. This is an efficiency review, not a certification of all numerical
edge cases or an exhaustive bug audit.
