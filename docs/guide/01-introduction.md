# 1. Introduction

RPNCE 0.7.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Next: 2. Installing RPNCE](02-installation.md)

**In this chapter**

- [What this version provides](#what-this-version-provides)

RPNCE turns the TI-84 Plus CE into an RPN (Reverse Polish Notation)
calculator in the tradition of Hewlett-Packard's scientific and financial
calculators. Instead of typing an expression and pressing `=`, you put
numbers on a **stack** and then apply operations to them: to add 2 and 3,
press `2 ENTER 3 +`. There are no parentheses to balance, every intermediate
result is visible, and long calculations become a sequence of simple steps.

RPNCE is keyboard-first, like the HP-32SII: the keys do what their labels
say, including their `2ND` functions. Everything else lives in a menu tree
shown along the bottom of the screen, a design borrowed from RPN83P and the
HP-42S.

**Highlights**

- A four-level HP-style stack (`T`, `Z`, `Y`, `X`) with `LASTX`, expandable
  to 12 levels.
- The calculator's own 14-digit decimal arithmetic, shown with up to 12
  digits, thousands separators, and FIX, SCI, ENG and ALL formats.
- Registers numbered as on the HP-32SII: `A` to `Z`, `i` and indirect `(i)`,
  the statistics registers, and up to 100 more, with register arithmetic, a
  register exchange, and a VARS catalog of everything stored.
- Math, number, probability, hyperbolic and conversion functions.
- BASE: hexadecimal, octal and binary integers of up to 44 bits, with logic,
  shift, rotate and bit operations.
- STAT: one- and two-variable statistics with linear, logarithmic,
  exponential and power curve fits.
- TVM: loans, savings and annuities, sharing values with TI's own TVM
  Solver.
- UNIT: 169 units in 12 categories, with unit-aware arithmetic: 6 ft plus
  2 in is 6.1667 ft.
- DATE: dates, times, time zones, durations, epoch seconds, and the CE's
  real-time clock.
- HELP: quick-reference pages on the calculator (`2ND CATALOG`).
- Completed stack values, registers, statistics, and settings are saved
  when you quit and restored when you start again.

**Not yet available:** complex numbers. The **CPLX** menu opens, but its
items, and the complex settings in **MODE**, show "Not available yet".


## What this version provides

RPNCE is an interactive calculator. It evaluates an operation as soon as
you press its key and keeps intermediate values on the stack. It does not
store a history of expressions or record keystroke programs.

The menus are grouped by task. **MATH**, **NUM**, **PROB**, **HYP**, and
**CONV** provide numerical operations; **STAT**, **TVM**, **BASE**, **UNIT**,
and **DATE** provide specialized workflows. **STK**, **CLR**, and **MODE**
manage the working environment. **HELP** supplies a short on-device
reference; this manual supplies the fuller procedures and examples.

RPNCE's lettered registers are private to the program. Storing a value in
RPNCE's A does not change TI-OS variable A or matrix [A]. The seven TVM
values are deliberately shared with TI's Finance solver. See
[Saving, memory and files](17-memory.md) for that distinction.

---

[Manual contents](../USER_GUIDE.md) · [Next: 2. Installing RPNCE](02-installation.md)
