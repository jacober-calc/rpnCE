# 10. Math functions

RPNCE 0.7.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 9. Display formats and MODE settings](09-settings.md) · [Next: 11. BASE: hexadecimal, octal and binary](11-base.md)

**In this chapter**

- [On the keyboard](#on-the-keyboard)
- [MATH menu](#math-menu)
- [NUM menu](#num-menu)
- [PROB menu](#prob-menu)
- [HYP menu](#hyp-menu)
- [CONV menu](#conv-menu)
- [STK and CLR menus](#stk-and-clr-menus)
- [Operand and result conventions](#operand-and-result-conventions)
- [Percentages and rounding](#percentages-and-rounding)
- [Trigonometry and coordinate conversion](#trigonometry-and-coordinate-conversion)
- [Hyperbolic domains](#hyperbolic-domains)
- [Other domain restrictions](#other-domain-restrictions)

## On the keyboard

| Keys | Function |
| --- | --- |
| `+ − × ÷` | Y + X, Y − X, Y × X, Y ÷ X |
| `^` | Y^X (Y to the power X) |
| `x²`, `2ND √` | X², √X |
| `x⁻¹` | 1/X |
| `LOG`, `2ND 10ˣ` | common log, 10^X |
| `LN`, `2ND eˣ` | natural log, e^X |
| `SIN COS TAN` | trigonometry (in the current angle mode) |
| `2ND SIN⁻¹ COS⁻¹ TAN⁻¹` | inverse trigonometry |
| `2ND π` (on `^`), `2ND e` (on `÷`) | the constants π and e |

**Negative numbers and powers** follow the HP rule (as on the HP-32SII and
DM32): Y^X with a negative Y needs a whole-number X, and a root of a
negative number needs an odd whole-number index. So `8 (−) ENTER 3 ^` is
−512, but (−8)^0.4 is "Err: Domain", even though it has a real value. To
get such values, work with the positive number and apply the sign yourself.

## MATH menu

| Item | Function | Example |
| --- | --- | --- |
| **X^3** | X³ | `3` **X^3** → 27 |
| **³√X** | cube root (works for negatives) | `8 (−)` **³√X** → −2 |
| **X√Y** | the X-th root of Y | `81 ENTER 4` **X√Y** → 3 |
| **ATN2** | the angle of the point (x, y): y in Y, x in X, in −180° to 180° | `1 ENTER 1 (−)` **ATN2** → 135 (DEG) |
| **2^X** | 2 to the power X | `10` **2^X** → 1,024 |
| **LOG2** | log base 2 | `1024` **LOG2** → 10 |
| **LOGB** | log of Y in base X | `8 ENTER 2` **LOGB** → 3 |
| **E^X-** | eˣ − 1, accurate for tiny X | |
| **LN1+** | ln(1 + X), accurate for tiny X | |

**X√Y** follows the negative-number rule above: `8 (−) ENTER 3` **X√Y** is
−2, but an even or fractional index of a negative number is "Err: Domain".
**ATN2** of (0, 0) is 0. **E^X-** and **LN1+** matter in finance and
science where X is very small: for X = 0.00000000000001, ordinary
ln(1 + X) loses every digit, but **LN1+** gives the right answer.

## NUM menu

| Item | Function |
| --- | --- |
| **%** | X% of Y: `200 ENTER 15` **%** → 30 (Y is kept) |
| **%CH** | percent change from Y to X: `80 ENTER 100` **%CH** → 25 |
| **GCD**, **LCM** | greatest common divisor and least common multiple of Y and X |
| **PRIM** | the smallest prime factor of X (2 to 4,294,967,295), or 1 if X is prime, leaving X in Y: `91` **PRIM** → 7, with 91 in Y |
| **IP**, **FP** | integer part and fractional part |
| **FLR**, **CEIL**, **NEAR** | round down, round up, round to the nearest whole number |
| **ABS**, **SIGN** | absolute value; −1, 0 or 1 |
| **MOD** | Y mod X, taking the sign of X |
| **MIN**, **MAX** | the smaller or larger of Y and X |
| **RNDF** | round X using the selected format's digits (in FIX 2, 3.14159 becomes 3.14) |
| **RNDN** | round X to a number of decimal places: **RNDN** `2` |
| **RNDG** | round X to a number of significant digits: **RNDG** `3` |

**RNDN** and **RNDG** ask for their digits the same way as FIX (a digit, or
`.` then a digit for 10-19). Rounding is exact decimal rounding, so 2.675
rounded to 2 places is 2.68, not the 2.67 a computer often gives.

To factor a number completely, divide out each factor that **PRIM** finds.
Because PRIM leaves the original number in Y, `÷` straight after it divides
the factor out: `91` **PRIM** gives 7 (with 91 in Y), `÷` gives 13, and
**PRIM** again gives 1, so 13 is prime and 91 = 7 × 13. The same works for
longer factorizations: `1981` **PRIM** gives 7, `÷` gives 283, and
**PRIM** gives 1.

## PROB menu

| Item | Function |
| --- | --- |
| **COMB** | combinations: Y things taken X at a time, `52 ENTER 5` **COMB** → 2,598,960 |
| **PERM** | permutations: `5 ENTER 2` **PERM** → 20 |
| **N!** | factorial, for whole numbers 0 to 69 |
| **RAND** | a random number from 0 up to (not including) 1 |
| **SEED** | restart the random sequence from X, so it can be repeated |

## HYP menu

**SINH**, **COSH**, **TANH** and their inverses **ASNH**, **ACSH**,
**ATNH**, all accurate near zero. **ACSH** needs X ≥ 1 and **ATNH** needs
−1 < X < 1.

## CONV menu

| Item | Function |
| --- | --- |
| **→DEG**, **→RAD** | convert an angle between radians and degrees |
| **→POL** | rectangular to polar: x in X and y in Y become r in X and the angle in Y |
| **→REC** | polar to rectangular: r in X and the angle in Y become x in X and y in Y |
| **→HR** | H.MMSS to hours: `1.3045` **→HR** → 1.5125 (1 h 30 min 45 s) |
| **→HMS** | hours to H.MMSS |
| **HMS+**, **HMS−** | add or subtract two H.MMSS values: `1.45 ENTER 0.30` **HMS+** → 2.15 |

Example: in DEG mode, `4 ENTER 3` **→POL** gives r = 5 in X and an angle of
53.1301° in Y.

## STK and CLR menus

**STK** holds the stack operations from [chapter 6](06-stack.md#moving-things-around)
and **X<>** ([chapter 8](08-registers.md#exchanging-x-with-a-register)).

| CLR item | Clears |
| --- | --- |
| **CLX** | X |
| **CLST** | every stack level |
| **CLRG** | registers A-Z, i and the extra registers |
| **CLΣ** | the statistics |
| **CLTV** | the TVM values (chapter 13) |
| **CLD** | the message line |


## Operand and result conventions

**Menu locations:** **HOME** → **MATH**, **NUM**, **PROB**; **HOME** row 2
→ **HYP** or **CONV**. The keyboard functions are available without opening
a menu. The [reference tables](19-reference.md) show every row.

Most unary functions replace X and preserve the higher levels. Most binary
functions use Y as the first operand, X as the second, and drop the stack.
There are important exceptions:

| Function | Effect on X and Y |
| --- | --- |
| **%**, **%CH** | Replace X with the result; Y is retained. |
| **PRIM** | Push the factor into X, preserving the original value in Y. |
| **→POL**, **→REC** | Replace both X and Y with the converted pair. |
| **RAND** | Supply a new number to the stack. |

LASTX normally holds the original X operand of a calculation. Review the
[stack chapter](06-stack.md) before combining these exceptions into a long
keystroke sequence. Functions that require ordinary numbers reject DATE
objects or unsupported quantities instead of silently discarding their type.

## Percentages and rounding

### Example: add a percentage to an amount

Find the total of 85 plus 7.5%.

**Setup:** DEC, FIX 4; open **HOME** → **NUM**.

| Keys | Y | X | Explanation |
| --- | --- | --- | --- |
| `85 ENTER 7.5` | 85 | 7.5 | Base amount in Y, percentage in X. |
| **%** | 85 | 6.375 | Calculate the percentage; Y is preserved. |
| `+` | — | 91.375 | Add the percentage amount to the original. |

Use **RNDN** `2` if the next calculation must use 91.38 rather than the
unrounded 91.375. Selecting FIX 2 alone changes only its appearance.

### Example: percentage change

An amount increases from 95 to 114. Enter `95 ENTER 114`, then **%CH**.
X is 20, because `(114 − 95) / 95 × 100 = 20%`; Y remains 95.
The old amount belongs in Y. Reversing the operands answers a different
question. A zero old amount gives division by zero.

### Example: greatest divisor and least multiple

Enter `18 ENTER 24`, then **GCD**, to obtain 6. Enter the pair again and
select **LCM** to obtain 72. Both inputs must be nonnegative whole numbers;
LCM with either input zero is zero. These functions do not accept quantities.

### Integer and fractional parts

| Function applied to −2.7 | Result | Rule |
| --- | --- | --- |
| **IP** | −2 | Truncate toward zero. |
| **FP** | −0.7 | Fractional part retains the sign. |
| **FLR** | −3 | Largest integer no greater than X. |
| **CEIL** | −2 | Smallest integer no less than X. |
| **NEAR** | −3 | Round to the nearest integer. |

Re-enter −2.7 for each row; functions act on the current X. **MOD** is
Y modulo X with the divisor's sign, rather than the signed fractional
part: `7 ENTER 3` **MOD** gives 1.

### Choosing a rounding function

| Requirement | Procedure |
| --- | --- |
| Round to the current FIX/SCI/ENG presentation | **RNDF** |
| Round to two decimal places | **RNDN** `2` |
| Round to three significant digits | **RNDG** `3` |
| Request 10–19 digits in a digit prompt | Press `.`, then a digit 0–9. |

RNDF uses decimal places in FIX and significant digits in SCI/ENG. In the
all-digits setting (FIX-), RNDF leaves the value unchanged. If FIX falls
back to scientific notation for a very small value, RNDF still uses the
requested FIX decimal-place count; it is not a copy of the rendered text.

For example, `12.3456` **RNDN** `2` gives 12.35; re-enter `12.3456` and
use **RNDG** `3` for 12.3. Rounding changes the value used by subsequent
operations; avoid rounding intermediate results unless the problem calls
for it. The decimal-rounding implementation also gives `2.675` **RNDN** `2`
→ 2.68.

## Trigonometry and coordinate conversion

**Setup:** DEC, DEG, FIX 4; open **HOME** row 2 → **CONV**.

| Keys | Y | X | Meaning |
| --- | --- | --- | --- |
| `4 ENTER 3` | 4 | 3 | Point (x, y) = (3, 4). |
| **→POL** | approximately 53.1301 | 5 | Angle in Y, radius in X. |
| **→REC** | approximately 4 | approximately 3 | Convert back to y in Y, x in X. |

For **ATN2**, use the same rectangular operand order: y in Y, x in X.
Its result is the angle in the selected DEG or RAD mode. Selecting RAD
changes the interpretation of angles for trig and coordinate functions;
**→DEG** and **→RAD** explicitly convert a numeric angle value.

For hours/minutes/seconds, 1.30 means 1 hour 30 minutes, not 1.3 hours.
Enter `1.30` **→HR** for 1.5 hours. With **HMS+**, `1.45 ENTER 0.30`
produces 2.15, meaning 2 hours 15 minutes. These numeric H.MMSS functions
are separate from the typed Time and Duration objects in DATE.

## Hyperbolic domains

| Function | Accepted real X |
| --- | --- |
| **SINH**, **COSH**, **TANH**, **ASNH** | Any real number, subject to result range |
| **ACSH** | X ≥ 1 |
| **ATNH** | −1 < X < 1 |

The inverse menu labels are shortened to fit the screen. They mean inverse
hyperbolic sine, cosine, and tangent. Ordinary trigonometric DEG/RAD
settings do not scale the inputs to hyperbolic functions.

## Other domain restrictions

| Function | Required operands |
| --- | --- |
| LOG, LN, LOG2 | X > 0 |
| LOGB | Y > 0; base X > 0 and X ≠ 1 |
| LN1+ | X > −1 |
| ASIN, ACOS | −1 ≤ X ≤ 1 |
| X√Y | X ≠ 0; negative Y requires an odd whole-number X |
| Y^X | Negative Y requires a whole-number X |
| COMB, PERM | Whole numbers with 0 ≤ X ≤ Y |
| GCD, LCM | Nonnegative whole-number X and Y |
| PRIM | Whole X from 2 to 4,294,967,295 |
| N! | Whole X from 0 to 69 |

Any function can also fail if its result exceeds the supported numeric
range. Retain the original operands or use LASTX before applying a
different function; an error is not a prompt to continue a partial formula.

---

[Manual contents](../USER_GUIDE.md) · [Previous: 9. Display formats and MODE settings](09-settings.md) · [Next: 11. BASE: hexadecimal, octal and binary](11-base.md)
