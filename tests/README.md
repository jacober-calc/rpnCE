# Tests

## Guided hardware tests

The browser tool, test catalog, grading checks and usage instructions are
kept together in [hardware/](hardware/README.md). Open
[hardware/index.html](hardware/index.html) in a browser to begin.

## Desktop suites

Run the desktop suites from the repository root:

```sh
sh tests/run_tests.sh
```

The 17 suites cover formatting, rounding, math, BASE, statistics, TVM,
units, DATE, resources, menus, keys, fonts, saved state, calculator commands,
entry sequences, display redraws and brightness. Host stand-ins use doubles
for arithmetic; rounding tests use packed decimal digits.

The sequence suite checks explicit arithmetic and editing examples, record
repair, digit limits and deterministic mixed entry at stack depths 4 and 12.
The unit matrix covers every unit's conversion, storage and rounding,
including temperature and fuel restrictions.

Formatting checks cover signs, grouping and decimal marks across exponents
−99 through +99 and every display precision. Decimal rounding is checked
directly in the packed 14-digit representation, with integer reference
rounding across the same exponent range. Host math checks include tiny
arguments and series-transition boundaries; TVM checks include negative
and tiny rates and invalid payment/compounding settings.

## Native CE checks

`native/` compiles the core with `-Oz`/LTO settings and runs 161
checks in CEmu with TI-OS arithmetic. It covers arithmetic, DATE ranges,
CTZ/ATZ conversion, units, all four curve-fit models, TVM and real GraphX
pixels. The fixed clock makes results repeatable. The check count and
completion marker are verified independently of the display.

The native numeric cases compare against decimal references rather than
host `double` results. Exact checks compare the stored digits; approximate
checks use relative error so zero cannot pass for a tiny nonzero answer.

With `cedev-config` and `cemu-autotester` on PATH:

```sh
export AUTOTESTER_ROM=/path/to/ti84CE.rom
export AUTOTESTER_LIBS_GROUP=/path/to/clibs.8xg
python3 tests/native/run.py /path/to/launcher.8xp
```

Omit the launcher argument for direct native execution. The suite uses no
saved-data appvars and restores the shared brightness and TVM values it
changes inside the emulator.

`ce_smoke/` runs the built program through its keyboard loop, stores a
register, quits, and reopens it to verify actual appvar persistence. Its
nineteen checks include typed and recalled values at exponents −99 and +99,
and zero deviation for constant data before and after reopening. After
building the program, run it with the same emulator environment:

```sh
export AUTOTESTER_LAUNCHER=/path/to/launcher.8xp
python3 tests/ce_smoke/run.py
```

Omit `AUTOTESTER_LAUNCHER` for direct native execution.

The keyboard checks also cover CLEAR dismissing SHOW, errors and notices
without changing operands, ENTER performing its normal action after a
message, and ordinary CLEAR behavior returning after a notice times out.

## Numeric limit checks

The native suite now checks explicit overflow, underflow and precision
errors for the 14 cases confirmed on a TI-84 Plus CE running OS 5.8.4.0058.
The [original build-25 report](hardware/results/rpnce-hardware-c3165aca-19a2-48e9-b053-c58681dee30e.json)
is preserved. The [build-26 hardware retest](hardware/results/rpnce-hardware-2a9a2054-33d6-4bce-82e9-f1fb8e8df91a.json)
passes all 14 revised cases, with none skipped: six overflow, three underflow
and five precision warnings. This confirms the specified error behavior
for these cases on that device and OS; it is not a complete hardware audit.
The completed cases are retired from the browser pool; the native regression
checks and original device reports remain available.

The policy rejects out-of-range final values, required polar intermediates
outside the range, subtraction that silently turns unequal stored values
into zero, and ambiguous cancellation in variance calculations. Large
SINH may report overflow when its intermediate exponential exceeds the
range. Precision is not extended to force a numerical answer.

The 14 added native neighbor checks cover genuine zero results, ordinary
polar conversion and standard deviation, both range endpoints, rejected
statistics/register updates, and defensive formatting of legacy invalid
values. Native checks verify operand preservation, not just the message.
Desktop storage checks verify precision metadata round trips, damaged
metadata and legacy state without discarding the saved values.

Remaining limits include convergence for arbitrary TVM cash flows, noisy
curve fits, conversions between every pair of compatible units at numeric
limits, power interruption and other OS versions. These guards detect
specific failure classes; they do not guarantee every possible calculation.

Native regressions also cover silent underflow from powers, exponentials,
roots, cubes and percent, with valid zeros and the lower range endpoint as
neighbors. Converted-unit MOD rejects a noninteger quotient within 1E-12 relative
distance of an integer, preserving operands and LASTX.
Exact unit multiples and ordinary signed remainders remain covered.
