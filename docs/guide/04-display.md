# 4. The screen

RPNCE 0.7.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 3. First calculations](03-first-calculations.md) · [Next: 5. Entering numbers](05-number-entry.md)

**In this chapter**

- [Status bar](#status-bar)
- [Message line](#message-line)
- [Stack](#stack)
- [Menu row](#menu-row)
- [Screen colors](#screen-colors)

![The parts of the screen](../../assets/guide/screen-anatomy.png)

The display has four areas, arranged from top to bottom. The illustration
identifies them; each is described in this chapter.

## Status bar

The dark strip at the top shows the calculator's state.

![Status bar examples](../../assets/guide/status-bar.png)

| Item | Meaning |
| --- | --- |
| `2026-10-18 14:05` | the date and time from the CE's clock, in the clock's own time zone; it updates each minute |
| `FIX4`, `SCI3`, `ENG2` | the display format and its number of digits (see [chapter 9](09-settings.md#9-display-formats-and-mode-settings)) |
| `FIX-` | "as many digits as needed" (all digits, like ALL on an HP) |
| `DEG` / `RAD` | the angle mode for trigonometry and polar conversions |
| `DEC`, `HEX`, `OCT`, `BIN` | the number base (see [chapter 11](11-base.md#11-base-hexadecimal-octal-and-binary)) |
| `C` after the base | the BASE carry flag is set |
| `2ND` badge | `2ND` was pressed; the next key gives its blue function |
| `A` badge | `ALPHA` was pressed; the next key gives its green letter |

## Message line

Under the status bar, the message line shows the result of the last
command when there's something to say: an error such as "Err: Domain", the
full value from SHOW, a notice such as "TVM Stored", or, in BASE modes, a
warning about X. Messages disappear at the next key. A pair of arrows (↓↑)
at the right end means the current menu has more rows; see
[The menus](07-menus.md#7-the-menus).

## Stack

The middle of the screen shows the four stack registers, `T` at the top and
`X` at the bottom, each with its value right-aligned in a large dot-matrix
font. `X` is where numbers are typed and where results appear.

- A value can have a small label after it: a unit such as `ft` or `kg`
  (chapter 14), or `dec` in BASE modes for a value that can't be shown in
  that base (chapter 11).
- A value too long for its line (a 32-bit binary number, or a long date)
  stays on one line: you see its right end, with a small ◂ where digits are
  hidden. Press ◂ and ▸ (the left and right arrow keys) to scroll through X.
- While you type a number, X shows it with a cursor: `3_`.

## Menu row

The bottom row shows five dark boxes, one for each menu key. A short line
across the top of a box marks a submenu (pressing it opens another menu);
other boxes are functions. A ■ after a label marks the active choice in a
setting, such as **DEG■**. An empty slot has no box.

## Screen colors

`MODE`, then row 3, **TINT**, cycles through eight color schemes; the
message line names each one as you go, and the choice is remembered:

| Tint | Look |
| --- | --- |
| **LCD** | gray-green, like a classic calculator LCD (the default) |
| **LCD+** | a stronger green that holds up better on the CE's real screen, which tends to wash colors out |
| **GRAY** | warm gray, like the SwissMicros calculators |
| **INVERTED** | light digits on a near-black panel |
| **AYU DARK** | a deep blue-gray background with amber and blue accents |
| **MONOKAI** | charcoal, with purple, cyan, green and pink stack values |
| **PASTEL** | a light background with distinct row colors and pastel menu keys |
| **COMMODORE** | phosphor greens on near-black, with brighter X digits |

---

[Manual contents](../USER_GUIDE.md) · [Previous: 3. First calculations](03-first-calculations.md) · [Next: 5. Entering numbers](05-number-entry.md)
