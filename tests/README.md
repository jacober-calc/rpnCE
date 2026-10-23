# Tests

Run the desktop suites from the repository root:

```sh
sh tests/run_tests.sh
```

The 17 suites cover formatting, rounding, math, BASE, statistics, TVM,
units, DATE, resources, menus, keys, fonts, saved state, calculator commands,
entry sequences, display redraws and brightness. Host stand-ins use doubles
for arithmetic; rounding tests use packed decimal digits. Tests run from
a source checkout without requiring historical commits.

The sequence suite checks explicit arithmetic and editing examples, record
repair, digit limits and deterministic mixed entry at stack depths 4 and 12.
The unit matrix covers every unit's conversion, storage and rounding,
including temperature and fuel restrictions.

## Native CE checks

`native/` compiles the core with release `-Oz`/LTO settings and runs 83
checks in CEmu with TI-OS arithmetic. It covers arithmetic, DATE ranges,
CTZ/ATZ conversion, units, all four curve-fit models, TVM and real GraphX
pixels. The fixed clock makes results repeatable. The check count and
completion marker are verified independently of the display.

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
register, quits, and reopens it to verify actual appvar persistence.

## Release checks

Follow [RELEASING.md](../docs/RELEASING.md) for the consolidated checks and
package validation, and [HARDWARE.md](release/HARDWARE.md) for device
acceptance. Emulator results and physical-device results are recorded
separately.
