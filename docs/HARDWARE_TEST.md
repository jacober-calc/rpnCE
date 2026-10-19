# Refactor hardware testing

This is a development build of 0.6.6 on `refactor`. Send both files from
`bin/` using TI Connect CE, replacing the matching files on the calculator:

- `bin/RPNCE.8xp` — 45,976 bytes
- `bin/RPNCEres.8xv` — 11,545 bytes; resource stamp `37704715`

Use the existing CE libraries. This build does not replace `prod/` or create
a release. Existing v5 `RPNCEdat` state loads without a reset. New register
unit metadata is appended to the existing save format; older builds can
read the numeric and DATE values, but cannot retain register units when
saving again.

## Reported cases

Use an unused ordinary register for the storage checks. Select units from
the UNIT menu; selecting a unit finishes number entry and enables stack lift.

| Check | Expected |
| --- | --- |
| `1981 PRIM` | X = 7, Y = 1981; the factor result was already correct |
| Then `÷ PRIM` | X = 1, Y = 283 (283 is prime) |
| `6 ENTER 8 LCM` | 24 |
| `21 ENTER 6 LCM` | 42 |
| `0 ENTER 6 LCM` and `6 ENTER 0 LCM` | 0 |
| `2 kWh`, `2000 kcal`, `%CH` (units from ENER) | Approximately 16.222222222222%; Y remains 2 kWh |
| `6 ft`, `2 in`, `+` | Approximately 6.1666666666667 ft |
| Convert that result to `in` | 74 in |
| `6 ft`, `2 in`, `÷` | 36, with no unit |
| `2 kW`, `2000 kcal`, `%CH` | Err: Unit Mismatch; original values retained |
| `6 ft`, `2`, `+` | Err: Data Type; original values retained |
| Store a quantity, clear X, recall it | Same value and unit |
| STO+ / RCL+ with compatible units | Result uses the destination's unit |
| X<> with a register holding a different unit | Both values and units exchange |
| Store `0 ft`, open VARS | Register appears with ft label |
| Store a quantity, quit, reopen, recall it | Same value and unit |

Temperature and fuel-economy units support conversion and rounding;
arithmetic on them is refused. Compound units are not supported. Use UVAL
to remove a unit explicitly. Statistics and TVM continue to use plain values.

## Menus and keys

| Key | Opens |
| --- | --- |
| STAT | STAT |
| X,T,theta,n | UNIT |
| APPS | BASE |
| PRGM | HYP |
| 2ND MEM (`2ND +`) | CLR |
| 2ND CATALOG (`2ND 0`) | HELP |
| 2ND LINK (`2ND X,T,theta,n`) | DATE |
| 2ND LIST (`2ND STAT`) | STK |
| 2ND ANGLE (`2ND APPS`) | CONV |

TVM occupies HOME row 1, slot 5; HELP occupies row 2, slot 5. HELP has three
pages: UP / DOWN changes page; ENTER, CLEAR, DEL or ON returns. Check that
ALPHA APPS still enters B, STO APPS still selects B, and 2ND + a TVM softkey
still recalls its variable.

## TVM in VARS

Open VARS and scroll past the ordinary registers. N, I%YR, PV, PMT, FV,
P/YR and C/YR should all appear, followed by LASTX as the final entry.
TVM values and LASTX should appear even when zero. Select one
and press ENTER: it should recall the stored value to X. Repeat immediately
after ENTER on the stack: recalling should replace X, preserving Y. Recall
several TVM entries in succession and confirm their stored values stay intact.

TVM entries show the same values as 2ND + the corresponding TVM menu key,
and share the TI OS TVM Solver's variables. Use the TVM menu to store or
solve values. After CLTV, the catalog should show zeros for the five main
TVM values and 12 for both P/YR and C/YR.

## TINT presets

In MODE, press TINT repeatedly: LCD, LCD+, GRAY, INVERTED, AYU DARK, MONOKAI,
PASTEL, COMMODORE, then back to LCD. INVERTED retains the former DARK colors
and saved index. Check stack values and units, menu labels, the 2ND/ALPHA badge, VARS
selection and HELP in each new preset. Quit and reopen to check persistence.

## Completed checks

All 15 host suites pass, also under AddressSanitizer and
UndefinedBehaviorSanitizer. The old-core comparison matches 22,848 scalar
command/state cases and 8,568 entry steps, with intentional changes covered
separately. Old/new storage readers and writers were checked in both
directions at 0, 5 and 100 extra registers. The CE toolchain v15 build
succeeds; both TI file checksums and the appvar's resource payload validate.

Hardware execution remains to be verified by these calculator tests.
