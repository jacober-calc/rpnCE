# Hardware testing: 0.7.0

This is the 0.7.0 development build from `dev`. Send both files from `bin/` using TI Connect CE,
replacing the matching files on the calculator:

- `bin/RPNCE.8xp`: 46,168 bytes
- `bin/RPNCEres.8xv`: 17,019 bytes; resource stamp `5E2832E1` (format 2,
  with 19 HELP pages)

Use the existing CE libraries. An existing `RPNCEdat` from 0.6.6, or from
an earlier `refactor` test build, loads without a reset. Earlier refactor
register-unit footers migrate when no `RPNCEunt` exists; the next save moves
the recovered tags into `RPNCEunt`.

How things are saved:

- `RPNCEdat` keeps exactly the 0.6.6 layout (stack, stack units, LASTX,
  registers, settings), so 0.6.6 can still read a 0.7.0 save.
- Units on registers are saved in a separate appvar, `RPNCEunt`, written
  only when some register holds a unit (and deleted when none do). It has
  a checksum and a fingerprint of the register values it belongs to. If
  it's missing, damaged, or belongs to different values, the registers load
  without units; no value is ever lost.

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

## DATE arithmetic regression

Select DEC before entry. `2ND {` means the second function of `(`; the
commas below separate fields while the record is open. Use the Date object
from three fields, rather than a DateTime or ZonedDateTime, for day counts.

| Keys | Expected |
| --- | --- |
| `2ND { 1987 , 01 , 17 ENTER 15000 +` | `D{2028,2,11}` (or `2028-02-11` in string display) |
| `2ND { 1987 , 01 , 17 ENTER 2ND { 2026 , 10 , 19 −` | −14,520 days |
| `2ND { 2026 , 10 , 19 ENTER 2ND { 1987 , 01 , 17 −` | 14,520 days |
| `2ND { 1987 , 01 , 17 ENTER`, then DATE → CLK → NOWD, then `−` | Birth date minus the clock's current application date, in days |

The earlier build could report Err: Domain on the first case and corrupt
the date difference. The native [CE regression suite](../tests/ce_date/README.md)
reproduces the failure in the old build and verifies the fix, including
wide epoch-second conversions through years 1 and 9999.

## Local clock and application time zone

Keep the TI-OS clock set to local time. In DATE → CLK, set CTZ to the UTC
offset that reading represents, then set ATZ to the desired application
offset. For a reproducible check, use a TI clock of 2026-12-31 23:30 with
CTZ = −7 (or adapt the expected times to your current clock).

| Check | Expected |
| --- | --- |
| Set CTZ = −7, ATZ = −7 | Status bar 2026-12-31 23:30; NOWT 23:30; NOWD 2026-12-31; NWDZ carries −07:00 |
| Select NWUT | 2027-01-01 06:30 UTC |
| Set ATZ = +9, leave CTZ = −7 | Status bar 2027-01-01 15:30; NOWD/NOWT/NWDZ agree; NWUT and NOW epoch seconds unchanged |
| Quit and inspect the TI-OS clock | Still the original local time, allowing for elapsed time |
| Reopen and use CTZ? / ATZ? | −07:00 / +09:00; status bar still uses ATZ |
| Restore your actual CTZ and desired ATZ | Normal clock behavior resumes; no SETC needed |

Offsets are fixed; update them when your intended daylight-saving offset
changes. Only SETC explicitly writes the calculator's hardware clock.

## Register units: RPNCEunt

| Check | Expected |
| --- | --- |
| Store `5 ft` in a register, quit | `RPNCEunt` appears in 2ND MEM > Mem Management > AppVars |
| Reopen, recall it | 5 ft |
| Clear every register holding a unit (CLRG), quit | `RPNCEunt` is gone |
| Store `5 ft` again, quit; delete `RPNCEunt` in Mem Management; reopen, recall it | 5, with no unit: the value is kept |
| Store `5 ft`, quit; reopen; check that `RPNCEdat` loads | All values present |
| Upgrade an earlier refactor save containing a unit footer, without `RPNCEunt`; recall the register | Both value and unit retained; quitting creates `RPNCEunt` |

## Menus and keys

| Key | Opens |
| --- | --- |
| STAT | STAT |
| X,T,theta,n | UNIT |
| APPS | BASE |
| PRGM | HYP |
| 2ND MEM (`2ND +`) | CLR |
| 2ND CATALOG (`2ND 0`) | HELP |
| 2ND LIST (`2ND STAT`) | STK |

`2ND LINK` (`2ND X,T,theta,n`) and `2ND ANGLE` (`2ND APPS`) should do
nothing: they are kept free for complex-number entry.

TVM occupies HOME row 1, slot 5; HELP occupies row 2, slot 5. HELP has 19
pages, now read from the RPNCEres appvar: UP / DOWN changes page, wrapping
from the last page to the first; ENTER, CLEAR, DEL or ON returns. Page 1
should not mention 2ND LINK or 2ND ANGLE. Check that
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
