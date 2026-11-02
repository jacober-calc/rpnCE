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


## DATE, modes and value movement

1 November 2026 · Baseline commit `ab99bf5` on `refactor`

### Structural changes

- DATE menu conversions now stage `dt_result_t`, the same object-or-rational
  result used by calendar arithmetic. Arithmetic and menu conversions share
  `value_date_result()` to construct complete values. Conversion temporaries
  have a separate call boundary from clock/settings handling; one stack
  commit follows successful conversion. The former object/whole/push wrappers
  are removed. Existing structural aliases still use the universal evaluator.
- DATE component extraction uses one field/type table, including the signed
  16-bit Duration day field. Fixed epochs use small day-offset constants;
  the custom epoch retains calendar conversion. Whole-number/type guards are
  shared across the scalar conversions.
- `calc_mode.c` describes neutral byte-valued settings once. Assignment and
  menu selection use the same descriptors. Carry actions retain their lack
  of menu highlighting. Modes with additional validation/effects retain
  their handlers. No persistent state layout or command ID changes.
- Twenty-five direct unary real operations share one function lookup/call
  site. The existing functions, quantity preparation, prechecks, precision
  checks and result/underflow guards remain. Complex dispatch is unchanged.
- Stack shifts use `memmove()` of whole values. Initialization, clearing
  and resizing share one typed zero-fill helper that constructs TI decimal
  zero once per fill. Display-cache invalidation compares one padding-free
  format key, retaining all current caching and unit-label behavior.

The first attempt to merge every DATE branch into one function increased
the calculator file by 60 bytes despite passing desktop tests. It was
replaced by the separated conversion/adapter implementation. Smaller source
and fewer branches do not by themselves establish a storage saving.
The lookup tables add searches and indirect calls; execution timing has
not been benchmarked.

### Measured results

Same CE toolchain v15.0 and optimized/compressed settings:

| Output | Before | After | Reduction |
| --- | ---: | ---: | ---: |
| Calculator, bin/rpnCE.8xp | 54,431 bytes | 53,811 bytes | 620 bytes (1.14%) |
| Linked program image, obj/rpnCE.obj | 114,928 bytes | 113,212 bytes | 1,716 bytes (1.49%) |
| Zero-initialized static RAM (.bss) | 7,052 bytes | 7,051 bytes | 1 byte |
| Resource appvar, bin/rpnCEres.8xv | 19,225 bytes | 19,225 bytes | 0 |

Linked text falls from 95,431 to 93,462 bytes; read-only data grows from
6,588 to 6,707 bytes. The compressed-file measurement includes both effects.
Since the `dev` baseline `63dd939`, the two deeper passes together reduce
the calculator file by 2,531 bytes (4.49%) and the linked image by 8,936
bytes (7.32%).

### Verification

- All 18 desktop suites pass, also with AddressSanitizer and
  UndefinedBehaviorSanitizer. Added regressions cover every record's
  supported/unsupported components, epoch/calendar agreement and negative
  epoch-day flooring, neutral modes and invalid entry, and complete mixed
  values moving through all nine stack depths.
- All 194 native CE checks pass in CEmu, including 11 new checks for signed
  fields, the six epochs, overlapping mixed-value shifts and mode/menu
  agreement. All 22 packaged-program keyboard/persistence checks pass.
- A one-off comparison against the baseline matches 39,024 DATE/mode
  scenarios, including complete calculator state, error text, clock writes
  and every menu selection. It varies record/numeric/complex/unit inputs,
  lift, epochs, time zones, entry validity and unused mode arguments.
- Every guide screenshot regenerates byte-for-byte unchanged. Resource
  contents are unchanged; no files in cem/ are modified. Development
  binaries in bin/ are rebuilt, without a new release version.

### Next candidates and storage profile

The largest linked functions are calc_execute (6,384 bytes), display_draw
(4,894), compute_real (4,770), tvm_solve (4,117), value_eval (3,996) and
compute_complex (3,024). DATE conversion is now 2,224 bytes plus the
1,210-byte clock/stack adapter. These include inlined code; compare complete
builds when assessing another dispatch boundary or shared operation family.

The uncompressed resource payload has 11,732 bytes of text, 4,640 of menu
nodes, 1,352 of units and smaller navigation/index sections. Whole-string
deduplication already exists. A read-only experiment with suffix sharing
saves 66 bytes in current insertion order, or 233 with longest strings
first, before any generator changes. This is a bounded opportunity rather
than a reason to add a runtime decoder.

For a larger resource-format experiment, examine whether menu kind/child
group can share a byte, whether unit dimension/reciprocal can share a byte,
and whether the unused group-record padding can be eliminated. The current
580 nodes, 169 units and 56 groups put the gross one-byte-per-record saving
at 805 bytes before decoder/code-size costs. Such a change needs resource
versioning, host/CE layout checks and full navigation/screenshot validation.

Other candidates are operand/result conventions in remaining multi-result
dispatch and duplication in the menu/action description sources. Measure
each independently. Preserve the current precision policy, warnings,
real-time clock interpretation, complete values and saved-file versions.


## Resource records and text pooling

1 November 2026 · Baseline: development artifacts from the preceding pass
(53,811-byte calculator and 19,225-byte resources), continuing on `refactor`.

### Structural changes

- Removed the menu-node kind byte. A submenu has a child-group index;
  other nodes use 0xFF. A blank has MID_NONE, and an alternate label
  distinguishes a selectable mode from an ordinary item. The shared
  `menu_node_kind()` accessor derives this information. This keeps the
  existing group-index capacity, avoiding the proposed 64-group bit limit.
  The generator reserves NONE for blank slots so the encoding is unambiguous.
- Removed group-record padding. Menu nodes/groups are now seven/five bytes.
  Unit records use seven bytes by sharing a dimension byte with the
  reciprocal flag. The dimension index has seven bits (128 dimensions);
  the current data uses 12, and generation checks the limit.
- All on-disk resource structs use explicit packed layouts, with compile-time
  size assertions in the reader. Resource format is 3. The prior format is
  rejected; no compatibility decoder or superseded record structs remain.
  This resource format is separate from saved calculator state/settings.
- The generator collects string-bearing records before encoding them.
  One pool handles menu labels, unit constants/labels and HELP, placing
  longest strings first and reusing suffixes. Consumers still read normal
  C strings directly from archive, with no runtime decompression or buffer.

### Measured results

Same CE toolchain v15.0 and optimized/compressed settings:

| Output | Before | After | Change |
| --- | ---: | ---: | ---: |
| Calculator, bin/rpnCE.8xp | 53,811 bytes | 53,862 bytes | +51 bytes |
| Resource appvar, bin/rpnCEres.8xv | 19,225 bytes | 18,187 bytes | -1,038 bytes |
| Combined calculator + resources | 73,036 bytes | 72,049 bytes | -987 bytes (1.35%) |
| Linked program image, obj/rpnCE.obj | 113,212 bytes | 113,360 bytes | +148 bytes |
| Zero-initialized static RAM (.bss) | 7,051 bytes | 7,051 bytes | unchanged |

The decoder/code change increases the compressed calculator by 51 bytes;
the net storage saving includes that cost. Packed records save 805 resource
bytes: 580 menu-kind bytes, 56 padding bytes and 169 unit-flag bytes.
Suffix sharing saves 233 more, reducing the text pool from 11,732 to 11,499
bytes. Since the `dev` baseline `63dd939`, program-plus-resource storage
falls by 3,518 bytes (4.66%). Resource reads retain direct archive access;
execution timing has not been benchmarked.

### Verification

- All three generator entry points produce identical resource bytes,
  generated headers and host-test unit data. Menu IDs, unit order and
  numerical constants are unchanged.
- An independent format-2/format-3 comparison matches all 580 decoded
  nodes, 56 groups/navigation records, 169 units, 26 HELP pages, 12 base-unit
  indexes and 488 menu-to-unit entries. The baseline file is used only for
  this one-off comparison, not retained as a runtime format or repo fixture.
- A decoded-content fingerprint captured before these changes is checked
  in both desktop and native suites. New desktop checks cover every menu
  slot's navigation/selection behavior and resource-format mismatch handling.
- All 18 desktop suites pass, also with AddressSanitizer and
  UndefinedBehaviorSanitizer. All 195 native CE checks and 22 packaged-program
  keyboard/persistence checks pass in CEmu. All 17 hardware-tool tests and
  2,102 expanded question/reference checks pass.
- Guide screenshots regenerate byte-for-byte unchanged. The development
  program and resources in bin/ are rebuilt together. No files in cem/ are
  modified and the release version is unchanged.

### Next candidates

Resources now contain 11,499 text bytes, 4,060 menu-node bytes, 1,183 unit
bytes and 280 group bytes. Examine repeated HELP text with a separate
storage/latency/RAM measurement before adding any runtime compression.
Whole-string/suffix pooling is already handled without a runtime decoder.

The next code-side candidates remain menu/action descriptions and remaining
multi-result dispatch. In particular, determine whether command/argument
mapping can be generated from one authoritative description without adding
another resident table. The existing commands, numeric guards, saved formats
and screenshot/navigation contracts remain the regression boundaries.
