# Refactor audit

## Initial conservative cleanup

1 November 2026 · Baseline commit `471a69f`

This pass removes verified leftovers and repeated work without changing
mathematical algorithms, commands, value layouts or saved-file formats.

### Changes

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

### Measured size

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

### Verification

- All three resource-generator entry points produce byte-identical
  resources, generated headers and host-test unit data. The generated
  function index is also unchanged.
- All 18 desktop suites pass, including error preservation, units, DATE,
  storage migration, complex values and display checks.
- All 183 native CE checks pass in CEmu.
- All 22 packaged-program keyboard/persistence checks pass in CEmu.
- All 17 hardware-tool tests pass; generated question data is current.

### Original candidates

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


## Deeper refactor on `refactor`

1 November 2026 · Baseline commit `63dd939` on `dev`

The `refactor` branch starts from this baseline. It keeps the same
commands, value representation and saved-file versions.

### Structural changes

- Current saves and version-5 imports use one union workspace. Both codecs
  stage input before applying it; their lifetimes do not overlap. Frozen
  disk structures have explicit CE size assertions. Checksum, precision
  footer validation and math-state restoration now have shared helpers.
- `commands.c` defines operand, drop, paired-result, statistics and entry
  traits once. Evaluation and command execution consume that metadata;
  BASE and LINK resolve their value-dependent operand counts explicitly.
- Numerical settings are passed through the evaluation context. Real
  kernels write directly to the caller's result. Former global angle,
  rounding, unit, finance, statistics-pointer and result scratch state is
  removed. Complex logarithms receive an explicit radian angle argument.
- Constructors and the real/complex kernels retain separate call
  boundaries under CE LTO. Measurements showed that expanding these
  bodies into callers increased code size. This adds function calls;
  execution timing has not been benchmarked.
- Paired statistics use the evaluator's result flag rather than maintaining
  a second command list in stack execution.

Removing shared result scratch also removes stale-result dependence:
model selection succeeds after a failed arithmetic operation, even when
that operation left an out-of-range intermediate in the old global. The
new regression fails against the previous evaluator and passes here.

### Measured results

Same CE toolchain v15.0 and build settings as the baseline:

| Output | Before | After | Reduction |
| --- | ---: | ---: | ---: |
| Calculator, bin/rpnCE.8xp | 56,342 bytes | 54,431 bytes | 1,911 bytes (3.39%) |
| Linked program image, obj/rpnCE.obj | 122,148 bytes | 114,928 bytes | 7,220 bytes (5.91%) |
| Zero-initialized static RAM (.bss) | 8,674 bytes | 7,052 bytes | 1,622 bytes (18.70%) |
| Resource appvar, bin/rpnCEres.8xv | 19,225 bytes | 19,225 bytes | 0 |

The evaluator's own compiled frame is 154 bytes, compared with 390 before
kernel separation. This is a per-function frame measurement, not the peak
stack of the full call chain. Native tests exercise the resulting calls.

### Verification

- All 18 desktop suites pass, including new operand-boundary/context and
  version-5 precision-footer regressions. The suites also pass with
  AddressSanitizer and UndefinedBehaviorSanitizer.
- All 183 native CE checks and 22 packaged-program keyboard/persistence
  checks pass in CEmu. All 17 hardware-tool checks pass.
- The pre-refactor input, drop, pair, statistics and entry classifications
  match all 164 existing commands.
- A populated current save, including complex/DATE/unit values and
  precision metadata, serializes byte-for-byte identically to the baseline
  host fixture. Version-5 import followed by version-6 save/load is covered.
- All guide screenshots regenerate byte-for-byte unchanged. The resource
  appvar is unchanged. No files in cem/ are modified.

### Next candidates

The larger linked functions now include calc_execute (6,813 bytes),
compute_real (5,648), display_draw (4,957), calc_date_execute (4,235),
tvm_solve (4,117), value_eval (3,996) and compute_complex (3,024).
These figures include inlined helpers and are not estimates of waste.

A next pass can examine repeated DATE conversion/extraction control flow
and command-dispatch structure. Display formatting/caching should be
measured separately, retaining the screenshot and no-reformat regressions.
The mathematical formulas, precision policy and persistence formats remain
regression boundaries for that work.
