# Device acceptance

Use the candidate ZIP produced by the [release workflow](../../docs/RELEASING.md).
Record the calculator model, OS, tester, date, and the tested program and
resource hashes. Copy `release/validation/acceptance.template.json` to
`release/validation/acceptance.json` and record each category's result.
Leave untested categories pending.

Back up saved calculator data and any TVM values needed for other work;
these checks intentionally change stack, register and finance values.
See [Saving, memory and files](../../docs/guide/17-memory.md).

## Installation and persistence

1. Transfer the candidate program and matching resource appvar, with the
   required CE libraries. Verify a fresh launch using the intended launcher.
2. Enter `2 ENTER 3 +`: X should be 5. Store it in an unused register,
   quit with `2ND MODE`, reopen, and recall it.
3. Set stack depth 12 and extra registers, then repeat at their limits.
4. Test an installation with existing saved data. Stack, LASTX, registers,
   units, statistics and settings must remain intact.
5. Verify idle timeout saves and exits. Confirm normal TI-OS operation
   after exiting the program.

## Display, entry and brightness

| Check | Expected |
| --- | --- |
| `2 (−) 2ND x²` | Readable `Err: Domain`; status text and arrows retain their sizes |
| CLEAR twice, CLRG, CLTV, TVM store/solve, clock settings, TINT and SHOW | Consistent message height and spacing |
| Enter `4294967296`, select HEX with a 32-bit word | `Exceeds 32-bit word`; original value retained with `dec` label |
| Type `1234567.89` in DEC | `1,234,567.89_` |
| Type `1234`, then DEL | `1,234_` becomes `123_` |
| MODE row 4, comma decimal; type `1234567.89` | `1.234.567,89_`; physical decimal key still works |
| ENTER, SHOW, STO/RCL, VARS, SCI and ENG in comma mode | Consistent numeric marks; exponents ungrouped |
| DATE entry and SHOW in comma mode | DATE field separators retain their syntax |
| Repeated `2ND UP/DOWN`, including brightness limits | Bounded adjustment with a visible screen |
| Brightness shortcuts during entry, prompts, HELP and VARS | Current entry, prompt or selection preserved |
| Quit/reopen | Decimal preference retained; TI-OS uses the adjusted brightness |
| MODE row 4, point decimal | Point decimal and comma grouping restored |

Cycle all eight tints: LCD, LCD+, GRAY, INVERTED, AYU DARK, MONOKAI, PASTEL
and COMMODORE. Check numbers, units, small labels, menus, messages, HELP,
VARS selection and modifiers on the physical LCD. Verify tint persistence.

## Numbers and units

| Check | Expected |
| --- | --- |
| `1981 PRIM` | X = 7, Y = 1981 |
| Then `÷ PRIM` | X = 1, Y = 283 |
| `6 ENTER 8 LCM`; `21 ENTER 6 LCM` | 24; 42 |
| `0 ENTER 6 LCM`; `6 ENTER 0 LCM` | 0 |
| `2 kWh`, `2000 kcal`, `%CH` | Approximately 16.222222222222%; Y retains 2 kWh |
| `6 ft`, `2 in`, `+`, convert to `in` | Approximately 6.1666666666667 ft, then 74 in |
| `6 ft`, `2 in`, `÷` | 36 without a unit |
| `2 kW`, `2000 kcal`, `%CH` | Unit mismatch; operands preserved |
| `6 ft`, `2`, `+` | Data type error; operands preserved |
| Quantity STO/RCL, STO+/RCL+, X<> | Values and units retained; compatible units converted |
| Store `0 ft`, open VARS | Register appears with ft label |
| Store a quantity, quit/reopen, recall it | Value and unit retained |
| Temperature and fuel conversions and rounding | Correct results with unit retained |
| Arithmetic on temperature or fuel quantities | Refused with operands preserved |

Check each unit grouping used in normal work. Compound-unit construction
is outside the program's supported operations. Statistics and TVM use
plain values.

For register-unit persistence, use a backup before changing appvars:

| Check | Expected |
| --- | --- |
| Store `5 ft` in a register and quit | `RPNCEunt` exists; reopening recalls 5 ft |
| Clear every register holding a unit and quit | `RPNCEunt` is absent |
| Delete `RPNCEunt` after saving a quantity | Numeric register value still loads |

## DATE and the clock

Select DEC. Enter Date records with `2ND (`; commas separate fields.

| Check | Expected |
| --- | --- |
| `D{1987,1,17}` plus 15000 | `D{2028,2,11}` |
| `D{1987,1,17}` minus `D{2026,10,19}` | −14,520 days |
| Reverse those operands | 14,520 days |
| `T{12,47,0}` minus `T{20,17,0}` | −27,000 seconds; SEC→ gives −7 h 30 min |
| Reverse those operands | 27,000 seconds; SEC→ gives 7 h 30 min |

Keep TI-OS set to local time. Set CTZ to that reading's UTC offset and ATZ
to the application's offset. For a reproducible rollover test, a clock
reading of 2026-12-31 23:30 at CTZ = −7 gives:

| ATZ | Application status, NOWD/NOWT/NWDZ | NWUT |
| --- | --- | --- |
| −7 | 2026-12-31 23:30, −07:00 | 2027-01-01 06:30 UTC |
| +9 | 2027-01-01 15:30, +09:00 | Same UTC reading |

Verify that changing ATZ leaves TI-OS time intact, and both offsets
survive quitting/reopening. Restore the intended offsets afterward.
Offsets are fixed; daylight-saving adjustments are manual.

## Keys, HELP, statistics and TVM

Check STAT → STAT, X,T,theta,n → UNIT, APPS → BASE, PRGM → HYP,
`2ND +` → CLR, `2ND 0` → HELP, and `2ND STAT` → STK. Verify ALPHA and
STO/RCL letter selection still take priority over shortcuts. The reserved
complex-entry keys remain unassigned.

HELP should contain 19 readable pages, wrap in both directions, and return
on ENTER, CLEAR, DEL or ON. HOME places TVM on row 1 and HELP on row 2.

Work through the [statistics](../../docs/guide/12-statistics.md) and
[TVM](../../docs/guide/13-tvm.md) examples. In VARS, verify all seven TVM
values, including zeros, with LASTX always last. Recall after ENTER should
replace X without lifting. Confirm finance values agree with TI's solver
and persist after exit.
