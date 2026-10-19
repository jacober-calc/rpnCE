# 9. Display formats and MODE settings

RPNCE 0.7.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 8. Registers](08-registers.md) · [Next: 10. Math functions](10-mathematics.md)

**In this chapter**

- [FIX, SCI and ENG](#fix-sci-and-eng)
- [Angle mode](#angle-mode)
- [Sizes and tint](#sizes-and-tint)
- [DATE display](#date-display)
- [Example: changing presentation without changing the number](#example-changing-presentation-without-changing-the-number)

`MODE` opens the MODE menu.

| Row | Items |
| --- | --- |
| 1 | **FIX** **SCI** **ENG** **RAD** **DEG** |
| 2 | **RRES** **CRES** **RECT** **PRAD** **PDEG** (complex numbers: not yet available) |
| 3 | **RSIZ** **RSZ?** **TINT** **SSIZ** **SSZ?** |
| 4 | **{..}** **".."** (how DATE objects are shown) |

## FIX, SCI and ENG

Each asks for a number of digits: the prompt shows `FIX _`.

- A digit 0-9 completes it straight away: **FIX** `4` gives four decimal
  places.
- `.` then a digit means 10 to 19, as on the HP-32SII: **FIX** `. 1` is FIX
  11.
- 12 or more (`. 2` to `. 9`) means "as many digits as needed", shown in
  the status bar as `FIX-`. This is the default, and the same as ALL on an
  HP calculator.

| Format | 1234.5678 shows as | 0.000012345 shows as |
| --- | --- | --- |
| FIX- | `1,234.5678` | `0.000012345` |
| FIX 2 | `1,234.57` | `1.23 ᴇ-5` |
| SCI 3 | `1.235 ᴇ3` | `1.235 ᴇ-5` |
| ENG 2 | `1.23 ᴇ3` | `12.3 ᴇ-6` |

FIX switches to scientific notation by itself when a number is too large
for 12 digits, or so small that it would show as zero. ENG is like SCI but
keeps the exponent a multiple of 3, to match prefixes like kilo and micro.
The formats change only the display: the full value is kept, and SHOW
(`2ND ENTRY`) displays it.

## Angle mode

**RAD** and **DEG** set the unit for SIN, COS, TAN, their inverses, ATN2
and the polar conversions. The status bar shows which is on. In DEG mode,
exact quarter turns give exact answers: `180 SIN` is exactly 0.

## Sizes and tint

**RSIZ** and **RSZ?** set and show the number of registers
([chapter 8](08-registers.md#adding-registers)); **SSIZ** and **SSZ?** the stack depth
([chapter 6](06-stack.md#a-deeper-stack)); **TINT** cycles the screen colors
([chapter 4](04-display.md#screen-colors)).

## DATE display

**{..}** shows DATE objects as you'd type them, like `D{2024,3,14}`;
**".."** shows them as readable text, like `2024-03-14`. See
[chapter 15](15-date.md#15-date-dates-times-and-the-clock).


## Example: changing presentation without changing the number

Enter `1 ENTER 7 ÷`, then perform these steps:

| Selection | Display or message | Meaning |
| --- | --- | --- |
| **FIX** `4` | `0.1429` | Four places after the decimal point. |
| **SCI** `4` | approximately `1.4286 E−1` | Four places in the scientific mantissa. |
| **ENG** `4` | approximately `142.86 E−3` | Engineering notation uses an exponent divisible by three. |
| `2ND ENTRY` | approximately `0.14285714285714` | SHOW reveals the stored value. |
| **FIX** `. 2` | approximately `0.142857142857` | Return to all available display digits. |

The E notation in the table represents the display's small exponent
marker. The numerical value remains unchanged throughout. A later RNDF
would round according to the selected format; see the mathematics chapter.

---

[Manual contents](../USER_GUIDE.md) · [Previous: 8. Registers](08-registers.md) · [Next: 10. Math functions](10-mathematics.md)
