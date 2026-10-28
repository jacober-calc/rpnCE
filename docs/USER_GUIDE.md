# rpnCE User Guide

**RPN scientific and financial calculator for the TI-84 Plus CE**\
**Software version 0.7.0 · First edition · 19 October 2026**

![rpnCE calculator display](../assets/rpnCE-promo1.png)

This manual explains how to operate rpnCE, from a first calculation to
statistics, time value of money, unit conversions, and calendar arithmetic.
It describes the functions implemented in version 0.7.0. Complex-number
functions remain unavailable in this version.

[Project and source](https://github.com/jacober-calc/rpnCE) ·
[Releases](https://github.com/jacober-calc/rpnCE/releases) ·
[Compact key reference](KEYS.md)

## Using this manual

For your first session, read **Installation**, **First calculations**, and
**The RPN stack**, in that order. Later chapters explain each feature with
its menu location, operand order, examples, and restrictions. Use the
alphabetical function index when you know a function's name but need to
find its instructions.

| To… | Read |
| --- | --- |
| Install, upgrade, or launch the program | [2. Installation](guide/02-installation.md) |
| Learn ENTER, stack lift, and LASTX | [6. The RPN stack](guide/06-stack.md) |
| Store a value or retrieve a TVM variable | [8. Registers](guide/08-registers.md) |
| Change digits, angles, or screen colors | [9. Settings](guide/09-settings.md) |
| Find a function or menu item | [Alphabetical function index](guide/19-reference.md#appendix-d-alphabetical-function-index) |
| Understand an error or recover saved work | [16. Troubleshooting](guide/16-troubleshooting.md) |
| Check which files rpnCE creates | [17. Saving, memory and files](guide/17-memory.md) |

## Contents

| Chapter | Subject |
| --- | --- |
| [1. Introduction](guide/01-introduction.md) | Capabilities and scope of this version |
| [2. Installation](guide/02-installation.md) | Required files, launch methods, upgrades, and exit |
| [3. First calculations](guide/03-first-calculations.md) | A guided first session with expected results |
| [4. The display](guide/04-display.md) | Status bar, stack, messages, menus, and colors |
| [5. Entering numbers](guide/05-number-entry.md) | Signs, exponents, correction, and precision |
| [6. The RPN stack](guide/06-stack.md) | ENTER, lift and drop, rolls, LASTX, and deeper stacks |
| [7. Menus and HELP](guide/07-menus.md) | Navigation, shortcuts, and on-device reference |
| [8. Registers](guide/08-registers.md) | STO, RCL, register arithmetic, indirect addressing, and VARS |
| [9. Settings](guide/09-settings.md) | FIX, SCI, ENG, angle mode, sizes, and DATE display |
| [10. Mathematics](guide/10-mathematics.md) | Keyboard math, MATH, NUM, PROB, HYP, CONV, STK, and CLR |
| [11. BASE](guide/11-base.md) | Number bases, unsigned words, logic, shifts, bits, and carry |
| [12. Statistics](guide/12-statistics.md) | Data entry, summary statistics, and curve fitting |
| [13. Time value of money](guide/13-tvm.md) | Payments, cash-flow signs, periods, and solving |
| [14. Units](guide/14-units.md) | Quantities, conversions, arithmetic, and stored measurements |
| [15. Dates and times](guide/15-date.md) | DATE objects, durations, time zones, epochs, and the clock |
| [16. Troubleshooting](guide/16-troubleshooting.md) | Errors, corrective actions, and startup problems |
| [17. Saving, memory and files](guide/17-memory.md) | Persistence, backups, reset, and TI-OS interaction |
| [18. Limits and compatibility](guide/18-compatibility.md) | Numerical limits, unavailable functions, and differences |
| [Reference tables](guide/19-reference.md) | Keys, complete menu map, units, and alphabetical function index |

## Conventions

| Notation | Meaning |
| --- | --- |
| `ENTER`, `MODE`, `STO→` | A physical key on the calculator |
| `2ND ENTRY` | Press `2ND`, then the key with ENTRY printed above it |
| `ALPHA MATH` | Press `ALPHA`, then `MATH` to enter its green letter A |
| **PMT**, **Σ+**, **kWh** | Press the menu key directly below that screen label |
| **HOME** → **UNIT** → **LENG** | Follow this path through the menus; `MATH` opens HOME |
| `25 ENTER 4 ÷` | Type 25, press ENTER, type 4, then press divide |
| `X`, `Y`, `Z`, `T` | Stack levels; X is the bottom, active level |
| `≈` or an ellipsis | A rounded result; stored digits may extend beyond it |

A displayed menu label takes priority over the physical key's usual
function. For example, **PV** means the softkey below PV, not a letter
entered with ALPHA. `MATH` opens HOME; **MATH** opens the submenu named MATH.
Within a menu, `↑` and `↓` select rows. Menu paths in the examples name the
required row rather than repeating every arrow press. The complete row
layouts are in the [menu map](guide/19-reference.md#appendix-b-menu-map).

## Working through examples

Each worked example states its starting settings. Set those explicitly:
rpnCE remembers settings and values from earlier sessions. A table row
continues from the preceding row unless it says to start a new calculation.

Display examples use FIX 4 when a fixed number of decimal places helps
comparison. Setting FIX changes the display only; use RNDF or RNDN when
you intend to change a stored number. Values are shown as text alongside
keystrokes so you can follow the examples without relying on screenshots.
Clearing a stack, statistics data, or TVM values for an example replaces
those values; save anything you need before following that example.

## Related documentation

The chapter-based organization and emphasis on operating procedures are
informed by Brian T. Park's [RPN83P User Guide](https://github.com/bxparks/rpn83p/blob/develop/USER_GUIDE.md).
The explanations and examples here describe rpnCE's implementation;
keyboard mappings, storage registers, BASE word sizes, and available
functions differ. See [Limits and compatibility](guide/18-compatibility.md)
before applying instructions written for another calculator.
