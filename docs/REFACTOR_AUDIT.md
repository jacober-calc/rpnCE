# Initial conservative cleanup

1 November 2026 · Baseline commit `471a69f`

This pass removes verified leftovers and repeated work without changing
mathematical algorithms, commands, value layouts or saved-file formats.

## Changes

- Removed the unused menu and unit generators that wrote the former C
  tables. The shared parser, exact unit definitions and current resource
  generator remain. Existing command-line entry points still use genres.py.
- Removed repeated operand range checks and unit masking from the private
  real kernel. Its sole caller, value_eval(), already performs those checks.
  Checks after quantity conversion and all result/error guards remain.
- Removed the DATE error wrapper; callers use the shared error mapping.
- Removed an unused local variable from the native underflow check.
- Refreshed the hardware catalog's unit-source fingerprint. Numerical
  references and question procedures are unchanged.

## Measured size

CE toolchain v15.0, existing optimized/compressed build settings:

| Output | Before | After | Reduction |
| --- | ---: | ---: | ---: |
| Calculator, bin/rpnCE.8xp | 56,435 bytes | 56,342 bytes | 93 bytes (0.16%) |
| Linked program image, obj/rpnCE.obj | 122,416 bytes | 122,148 bytes | 268 bytes |
| Resource appvar, bin/rpnCEres.8xv | 19,225 bytes | 19,225 bytes | 0 |
| Zero-initialized static RAM (.bss) | 8,674 bytes | 8,674 bytes | 0 |

Most removed lines belonged to host-side tools and never occupied space
on the calculator. Source cleanup therefore produces a modest binary
reduction. All files in cem/ retain their original contents.

## Verification

- All three resource-generator entry points produce byte-identical
  resources, generated headers and host-test unit data. The generated
  function index is also unchanged.
- All 18 desktop suites pass, including error preservation, units, DATE,
  storage migration, complex values and display checks.
- All 183 native CE checks pass in CEmu.
- All 22 packaged-program keyboard/persistence checks pass in CEmu.
- All 17 hardware-tool tests pass; generated question data is current.

## Candidates for a later pass

The linker map identifies these larger functions after this cleanup:

| Function | Linked code |
| --- | ---: |
| value_eval | 18,986 bytes |
| calc_execute | 6,639 bytes |
| display_draw | 4,957 bytes |
| calc_date_execute | 4,899 bytes |

These sizes include inlined helpers. They identify places to inspect
shared command classification and repeated control flow; they are not
estimates of removable code. Any factoring should be measured against
the existing error and numeric regressions.

Storage also has separate current-state and version-5 import scratch
buffers of 2,958 and 1,428 bytes. Reviewing their lifetimes may offer a
RAM saving through shared workspace. That requires a separate change
with migration and failed-load preservation checks. The version-5 reader
has a live caller and is required to import existing saves.
