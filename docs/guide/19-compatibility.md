# 19. Limits and differences

rpnCE 0.8.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 18. Saving, memory and files](18-memory.md) · [Next: Reference tables](20-reference.md)

**In this chapter**

- [Limits](#limits)
- [Compared with HP calculators](#compared-with-hp-calculators)
- [Compared with RPN83P](#compared-with-rpn83p)
- [Complex numbers](#complex-numbers)
- [Supported quantity operations](#supported-quantity-operations)
- [Scope of the documentation](#scope-of-the-documentation)

## Limits

| Item | Limit |
| --- | --- |
| Real components | 14 significant digits each; nonzero magnitudes 1 × 10⁻⁹⁹ to 9.9999999999999 × 10⁹⁹ |
| Typing | 12 mantissa digits per component, and an exponent from −99 to 99 |
| Stack | 4 to 12 levels |
| Registers | 40 to 140 |
| BASE words | 1 to 44 bits, unsigned |
| **PRIM** | 2 to 4,294,967,295 |
| **N!** | 0 to 69 |
| Dates | years 1 to 9999; time zones within ±23:45, in 15-minute steps |

## Compared with HP calculators

rpnCE follows the HP-32SII and DM32 for the stack, LASTX, stack lift,
registers and display formats. Some differences:

- In **RRES**, **roots and powers of negative numbers** follow the HP rule:
  a negative Y needs a whole-number exponent for Y^X, and an odd
  whole-number index for X√Y. CRES permits principal complex results.
- **ALL** is FIX with "as many digits as needed", set with
  **FIX** `. 2` through `. 9`.
- **No equations or programs:** rpnCE is a calculator, not a programmable
  one.

## Compared with RPN83P

rpnCE's menu tree, DATE objects, units and BASE operations follow RPN83P.
Some differences:

- **Registers** are lettered and numbered as on the HP-32SII (A to Z, i,
  and numbered extras) rather than R00 to R99, and all of them, including
  A to Z, can hold DATE objects and values with units.
- **The keyboard** follows the HP-32SII more closely: most keys do what
  their labels say, including their 2ND functions.
- **Roots and powers of negative numbers** follow the HP rule above in
  RRES. CRES allows principal complex results.
- **BASE words** go up to 44 bits.
- **Complex numbers:** follow RPN83P entry, LINK and result/display modes.


## Complex numbers

For entry, arithmetic, CPLX functions, result/display modes and LINK, use
[the illustrated complex-number tutorial](11-complex-numbers.md).

Complex values keep two 14-digit decimal components in one stack level.
They survive stack movement, LASTX, ordinary register arithmetic and a
normal save/reopen. Polar display does not replace the stored rectangular
components. General powers, complex roots and logarithms use principal
values; logarithm and exponential angles are radians in every mode.

Complex trigonometric and hyperbolic functions are unsupported. BASE word
operations, statistics, TVM, unit attachment and numeric settings reject
complex operands, including complex values with zero imaginary component.
REAL explicitly extracts a real component. CRES extends selected results
from real inputs; it does not extend real-only input contracts.

Required arithmetic intermediates and each result component must fit the
decimal range. A calculation can report overflow, underflow or detected
precision loss even when another formula could produce a representable
result. Failed arithmetic preserves operands and LASTX.

## Supported quantity operations

Unit conversion is within a dimension. Compatible linear quantities can
be added or subtracted, multiplied by a plain number, divided by a plain
number, or divided by one another for a ratio. Products of quantities,
inverse units, derived-unit construction, and general dimensional algebra
are not implemented. Temperature and reciprocal fuel units have additional
restrictions described in [Units](15-units.md).

## Scope of the documentation

This guide describes rpnCE 0.8.0. Use matching program/resource files and
record the version and build when
reporting a problem. Reference tables follow the current menu and unit
definitions.

---

[Manual contents](../USER_GUIDE.md) · [Previous: 18. Saving, memory and files](18-memory.md) · [Next: Reference tables](20-reference.md)
