# rpnCE 0.8.0

An RPN scientific and financial calculator for the TI-84 Plus CE.

- Real and complex decimal arithmetic, rectangular and polar entry,
  scientific functions, LINK, and independent result/display modes.
- HP-style stack with 4–12 levels, LASTX, ordinary and indirect registers,
  unit quantities, statistics, curve fitting, TVM, BASE, and DATE functions.
- Compact status indicators, DEG as the initial angle mode, decimal-point
  or decimal-comma display, and eight screen tints.
- On-device HELP and an illustrated guide with complex-number examples.

Use matching program and resource files. Source builds produce
`bin/rpnCE.8xp` and `bin/rpnCEres.8xv`; resource format is 4.

Saved calculator data uses format 6. Version-5 saves and their matching
unit metadata can be imported. Back up saved data before upgrading;
older program versions cannot read format 6. Shared TI-OS TVM variables
and the hardware clock retain their normal ownership.

See the [User Guide](USER_GUIDE.md) for operating instructions and
[installation](guide/02-installation.md) for setup and upgrade details.
