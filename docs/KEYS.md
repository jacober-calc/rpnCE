# Key layout

RPNCE follows RPN83P's design: the keyboard handles numbers, the stack, and
the math functions printed on the keys; everything else lives in the menu
tree, which is always shown along the bottom of the screen. MATH goes to the
top of the tree (HOME) and MODE goes straight to the MODE menu.

Stack behavior follows standard HP RPN rules: ENTER and CLX disable stack
lift, most other operations enable it, and STO, CHS, SHOW, and mode changes
leave it as it was.

## Number entry and stack

| Key | Primary | 2ND |
| --- | --- | --- |
| 0-9, `.` | number entry | |
| `,` | EE (exponent); inside a DATE record, the field separator | EE |
| `(-)` | +/- (of the exponent after EE) | ANS: LASTX |
| `ENTER` | ENTER | ENTRY: SHOW all 14 digits |
| `DEL` | backspace while typing, else CLX | INS: DUP |
| `CLEAR` | CLX; twice in a row clears the stack | |
| `(` | R-down | `{`: start a DATE record (R-up is in STK) |
| `)` | X<>Y | `}`: close a DATE record; otherwise DROP |

## Math

| Key | Primary | 2ND |
| --- | --- | --- |
| `+ - × ÷` | Y op X | `÷`: e |
| `^` | Y^X | PI |
| `x²` | X^2 | SQRT |
| `x⁻¹` | 1/X | |
| `LOG` / `LN` | LOG / LN | 10^X / e^X |
| `SIN COS TAN` | trig (DEG or RAD) | ASIN ACOS ATAN |

Negative bases follow the HP rule (HP-32SII / DM32): Y^X with a negative Y
needs a whole-number X, and ˣ√Y (MATH menu) with a negative Y needs an odd
whole-number X. Anything else is "Err: Domain", even where a real value
exists: (-8)^3 = -512 and ˣ√ of -8 with X = 3 is -2, but (-8)^0.4 and ˣ√ of
-32 with X = 2.5 are errors. To get those, use |Y| and apply the sign.

## Registers

Registers are numbered as on the HP-32SII:

| Number | Register |
| --- | --- |
| 001-026 | A-Z |
| 027 | i |
| 028-033 | n, Sx, Sy, Sx2, Sy2, Sxy (the 32SII's six stat registers) |
| 034-040 | ln x, (ln x)^2, ln y, (ln y)^2, ln x ln y, x ln y, y ln x sums (for curve fits) |
| 041-140 | extra registers, after MODE > RSIZ raises the total above 40 |

`STO→` then a key stores X: the key's green ALPHA letter (`STO→ MATH` =
STO A), `.` for i, or `2ND .` for (i), the register whose number is in i.
`2ND RCL` recalls the same way, and STK > X<> swaps X with a register.
Press `+ - × ÷` right after STO or RCL for register arithmetic (`STO→ + A`
adds X to A). Since `- × ÷` are also the letters W, R, M, press ALPHA first
to name those registers. CLEAR, DEL, or ON cancels the prompt.

The prompt appears in the X row. RCL works like typing a number: it lifts
the stack, except right after ENTER. STO never moves the stack.

Indirect use: `41 STO→ .` puts 41 in i; then `123 STO→ 2ND .` stores 123
in register 041, and `2ND RCL 2ND .` recalls it. i's integer part is used;
a number that isn't an existing register gives "Err: Index". Storing into
n, Sx, Sy, Sx2, Sy2 or Sxy directly leaves only the linear fit usable until
the stat data is cleared.

`VARS` lists every register holding a nonzero value (and LASTX) as
`A=12.5`, `i=41`, `n=5`, `R041=123` across the four stack lines. Up and
down move through the list; ENTER copies the selected value to X; ON,
CLEAR or VARS closes it.

## TVM (time value of money)

MATH, then TVM (row 2 of HOME). Works like an HP-12C: type a value and
press N, I%YR, PV, PMT, or FV to store it; press one of them right after
another TVM key (without typing a number) to solve for it. 2ND + the key
recalls the stored value. Money received is positive, money paid out is
negative.

Example, a $200,000 mortgage at 6.5% for 30 years: `360 N`, `6.5 I%YR`,
`200000 PV`, `0 FV`, then `PMT` gives -1,264.136...

Row 2: P/YR (payments per year), C/YR (compounding periods per year),
BEG / END (payments at the start or end of each period), CLTV (clear
everything; P/YR = C/YR = 12, END). Row 3: IYR1 and IYR2 (starting guesses
for solving I%YR, default 0% and 100%), TMAX (most iterations, default 50),
RSTV (reset those three).

N, I%YR, PV, PMT, FV, P/YR and C/YR are the calculator's own TVM variables,
so they are shared with TI's TVM Solver (APPS > Finance).

## Units

MATH, then UNIT (row 3 of HOME). Type a value and press a unit to attach
it (`12 ft`); press another unit of the same kind to convert (`m` gives
3.6576 m). The unit shows after the value on the stack and moves with it.
Math on a value removes its unit. UFCN has UVAL (remove the unit, keep the
value) and UBAS (convert to the base unit: m, m², m³, kg, K, N, Pa, J, W,
s, m/s, or L/100km).

Kinds: length, area, volume, temperature, mass, force, pressure, energy,
power, time, speed, and fuel economy (mpg and L/100km convert both ways).
Conventions where definitions differ: US survey units are the US survey
foot; the calorie is the thermochemical calorie (4.184 J); the Btu is the
International Table Btu; football fields include the end zones; the cable
is 120 fathoms.

## Stack depth

The stack has 4 levels (T, Z, Y, X) like the HP-32SII; MODE > SSIZ makes
it up to 12. Only the bottom four are shown; deeper levels sit above T. A
two-number operation drops the whole stack and copies the top level down;
R-down and R-up roll through every level. MODE > SSZ? shows the current
depth, and RSZ? the number of registers.

A value too long for its line (a 32-bit BIN number, say) stays on one line:
the rightmost digits show, with a small arrow where more are hidden. Left
and right arrows scroll X through the rest.

## NUM, HYP and CONV

- NUM: PRIM gives the smallest prime factor of a whole number from 2 to
  4,294,967,295, or 1 if it's prime. RNDF rounds X to what the display
  shows; RNDN _ and RNDG _ round to that many decimals or significant
  digits (a digit, or . then a digit for 10-19). Rounding is exact decimal
  rounding: 2.675 RNDN 2 gives 2.68.
- HYP: SINH, COSH, TANH, ASNH, ACSH, ATNH, accurate near zero.
- CONV: ->POL takes x in X and y in Y and gives r in X and the angle in Y
  (current angle mode); ->REC does the reverse. ->HR and ->HMS convert
  between hours and H.MMSS (1.3045 is 1 h 30 min 45 s); HMS+ and HMS- add
  and subtract H.MMSS values.

## BASE

MATH, then BASE. DEC, HEX, OCT, BIN choose how whole numbers are shown and
typed; the status line shows HEX, OCT or BIN, plus C when the carry flag is
set. In HEX, type A-F with ALPHA plus the key with that green letter (ALPHA
MATH = A, ALPHA APPS = B, ... ALPHA COS = F). Digits the base can't use, and
digits that would overflow the word size, are ignored. Long values scroll
with the left and right arrows.

In HEX, OCT and BIN the `+ - × ÷` keys do word arithmetic (BAS+ BAS- BAS×
BAS/) and `(-)` is NEG. Values are unsigned words of 1 to 44 bits (BCFS >
WSIZ, from X; default 32). The calculator's 14-digit numbers hold every
44-bit value exactly.

A value that isn't a word of the current size (too big, negative, or not a
whole number) stays in decimal with a small `dec` beside it, and when it's
in X the status line says why: "Exceeds 32-bit word", "Negative: not a
word", or "Not a whole number". BASE operations on a value too big for the
word size give "Err: Word Size"; raise WSIZ to use it.

- LOGI: AND, OR, XOR, NOT, NEG (two's complement).
- ROTS: SL, SR, ASR (keeps the sign bit), SLn and SRn (shift Y by X bits);
  RL, RR, RLC, RRC (through carry), and the n versions rotating Y X times.
  Shifts and rotates set the carry to the last bit moved out.
- BITS: CB, SB, B? (clear, set, test bit X of Y), REVB, CNTB.
- BFCN: BAS+, BAS- (carry on overflow or borrow), BAS×, BAS/, BDIV
  (quotient in Y, remainder in X).
- BCFS: CCF, SCF, CF? (clear, set, test carry), WSIZ, WSZ?.

## Statistics

MATH, then STAT (row 2 of HOME). Works like the HP-42S. Enter a data point
as `y ENTER x` and press Sigma+ (for one-variable data, just `x` then
Sigma+). X then shows the number of points, and the next number you type
replaces it. Sigma- removes a point (to fix a mistake, re-enter it and
press Sigma-). CL-Sigma (here or in CLR) clears all data. The data is saved between runs.

- Row 1: Sigma+, Sigma-, ALL-Sigma / LIN-Sigma (keep all 13 sums, needed for
  the LOG, EXP and PWR fits, or just the 6 linear ones), CL-Sigma.
- Row 2: SUM, MEAN, WMN (x weighted by y; y weighted by x), N, and the
  Sigma menu, which recalls any of the 13 sums.
- Row 3: SDEV and PDEV (sample and population standard deviation of x in X
  and of y in Y), SCOV and PCOV (covariance), and CFIT for curve fitting.
- CFIT: Y->X and X->Y forecast one from the other with the current model;
  SLOP, YINT, CORR give the fit's slope, intercept, and correlation. Row 2
  picks the model: LINF (y = a + bx), LOGF (y = a + b ln x), EXPF
  (y = a e^bx), PWRF (y = a x^b), or BEST for the best fit. LOG needs all
  x > 0, EXP all y > 0, PWR both.

## CLR

CLX, CLST (every stack level), CLRG (A-Z, i and the extra registers; stat
data is kept), CL-Sigma (stat data), CLTV (TVM), CLD (clear the message).

## DATE

MATH, then DATE (row 3 of HOME). Seven kinds of DATE objects live in the
stack and registers alongside numbers, on the Gregorian calendar from year
1 to 9999 with fixed UTC offsets:

| Object | Typed as | Shown in ".." mode |
| --- | --- | --- |
| Date | `{2024,3,14` | `2024-03-14` |
| Time | `T{15,36,1` | `15:36:01` |
| DateTime | `{2024,3,14,15,36,1` | `2024-03-14 15:36:01` |
| TimeZone | `{-7,0` | `UTC-07:00` |
| ZonedDateTime | `{2024,3,14,15,36,1,-7,0` | `2024-03-14 15:36:01-07:00` |
| DayOfWeek | `DW{4` | `Thu` |
| Duration | `{1,2,3,4` or `1D2H3M4S` | `1d 2h 3m 4s` |

Typing: `2ND {` starts a record; `,` separates fields while you're inside
one; `(-)` makes the current field negative; the closing `2ND }` is
optional (ENTER or any operation finishes the record). The type comes from
the number of fields (2 TimeZone, 3 Date, 4 Duration, 6 DateTime, 8
ZonedDateTime); Time and DayOfWeek need their tag, typed with ALPHA
(`ALPHA T`, `ALPHA D ALPHA W`) or with the first key of their DATE menu
(`T{}`, `DW{}`). A Duration can also be typed compactly: digits then ALPHA
D, H, M or S, as in `1 ALPHA D 30 ALPHA M`. An impossible record (Feb 30,
23:59:60) shows "Err: Invalid" and stays open for DEL to fix.

Arithmetic: `+` and `-` work as in RPN83P: Dates count in days; Times,
DateTimes and ZonedDateTimes in seconds; TimeZones in hours (multiples of
0.25); DayOfWeeks in days; a Duration works anywhere a number does.
Subtracting two of the same kind gives the difference (two Durations give a
Duration). `×` converts a Date, DateTime or ZonedDateTime to a time zone
(CVTZ). On DATE objects, `1/x`, `√` and `x²` are DCUT, DSHK and DEXD.
Other math on an object shows "Err: Data Type".

Submenus: D, T, DT, TZ, DZ, DR and DW hold each kind's conversions (epoch
days >ED/ED>, epoch seconds >ES/ES>, LEAP, DOW, CVTZ), components (.YR,
.MON, .DAY, .HR, .MIN, .SEC, .D{}, .T{}, .DT{, .TZ{), and casting (DSHK
shrink, DEXD extend, DCUT split in two, DLNK join two). EPCH chooses the
epoch for epoch days and seconds (UNIX 1970-01-01, NTP 1900, GPS 1980-01-06,
TIOS 1997, Y2K 2000, or a custom date set with EPC). CLK reads the CE's
clock (NOW as epoch seconds, NOWD, NOWT, NWDZ in the application time zone,
NWUT in UTC), sets the application and clock time zones (ATZ, CTZ, each
from a TimeZone or hours) and sets the clock (SETC, from a ZonedDateTime).

MODE row 4 chooses how objects are shown: `{..}` as typed (the default) or
`".."` as RFC 3339 strings. Long values scroll with the arrow keys. SHOW
(`2ND ENTRY`) always shows the raw record. DATE settings are saved with the
screen tint in `RPNCEcfg`.

## The screen

- **Status bar:** a dark strip with the calculator's state: the date and
  time from the CE's clock on the left (`2026-10-18 14:05`, in the clock's
  own time zone), and on the right the display format (FIX4, SCI3, FIX- for
  as many digits as needed), DEG or RAD, the base (DEC, HEX, OCT or BIN,
  with C when the carry flag is set), and a 2ND or A (ALPHA) badge when one
  is pending.
- **Message line:** under the status bar: errors, SHOW, notices, and BASE
  warnings about X, in the system font. Up/down arrows at its right end
  mean the current menu has more rows.
- **Stack:** T, Z, Y, X, with units, `dec` tags in BASE modes, and scroll
  arrows on long values.
- **Menu row:** the five menu keys.

## Menus and system

| Key | Action |
| --- | --- |
| `Y= WINDOW ZOOM TRACE GRAPH` | the five menu keys; the menu stays put after a pick |
| up / down arrows | previous / next row of the current menu (when the message line shows arrows at its right end) |
| `MATH` | HOME, the top of the menu tree |
| `MODE` | MODE menu: FIX, SCI, ENG (a digit 0-9, or . then a digit for 10-19, as on the HP-32SII: FIX . 1 is FIX 11; 12 or more means as many digits as needed, shown as FIX-), RAD, DEG. Row 3: RSIZ (X = total registers, 40-140; less than 40 means 40) and RSZ?, TINT (cycle the screen colors: LCD, LCD+ for real screens, GRAY, DARK; saved in the appvar `RPNCEcfg`), SSIZ (X = stack depth, 4-12; less than 4 means 4) and SSZ? |
| `VARS` | the register catalog (see Registers) |
| `ON` | EXIT: back one menu level |
| left / right arrows | scroll a value too long for the X line |
| `2ND MODE` (QUIT) | save and exit |
| `2ND ON` (OFF) | save and exit |

Numbers show up to 12 digits with thousands commas, or up to 10 next to an
exponent; SHOW (2ND ENTRY) shows all 14.

The stack, LASTX, variables, angle mode, and display format are saved on exit
(in the archived appvar `RPNCEdat`) and restored on the next run. After 5
minutes without a key press, RPNCE saves and exits so the calculator can
power down.
