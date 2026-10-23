# Tests

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

`native/` compiles the core with `-Oz`/LTO settings and runs 126
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
nine checks include typed and recalled values at exponents −99 and +99. After
building the program, run it with the same emulator environment:

```sh
export AUTOTESTER_LAUNCHER=/path/to/launcher.8xp
python3 tests/ce_smoke/run.py
```

Omit `AUTOTESTER_LAUNCHER` for direct native execution.

## Numeric failures under investigation

The original 83 native checks pass. Fourteen of the added boundary checks
currently fail, so the native suite deliberately exits with failure.
Calculator code is unchanged by these tests.

Arithmetic can return stored exponents outside the documented −99..+99
range: `9E99 + 9E99`, `1E99 × 10`, `1E99 ÷ 0.1`, and `1E-99 ÷ 10`.
Converting `1E99 km` to metres returns `1E102`; converting `1E-99 m` to
kilometres returns `1E-102`. Underflow needs an explicit zero/error policy; the tests
only require a result inside the supported range or an error.

Polar conversion of coordinates `3E90, 4E90` overflows; coordinates
`3E-90, 4E-90` give a zero radius. Their radii should be `5E90` and
`5E-90`. `SINH(230.5)` overflows although its result is about
`6.3657280768316E99`.

`1.0000000000001 − 1` returns zero. Near-domain checks for
`ACOSH(1.0000000000001)`, `ATANH(0.99999999999999)`, and
`LN1+(−0.99999999999999)` also fail their decimal references. These
cases need investigation of TI-OS comparison and cancellation behavior.
Sample standard deviation of `100000001, 100000002, 100000003` returns
zero instead of 1.

The remaining coverage gaps include native I%YR convergence across extreme
cash flows, uncertainty/cancellation in noisy curve fits, intermediate
statistics overflow and recovery, native integer/combinatorial functions
at their upper ranges, and conversions between every pair of compatible
units at numeric limits. Emulator checks use one ROM; physical
LCD appearance, battery/power interruption and other OS versions still
need separate testing.
