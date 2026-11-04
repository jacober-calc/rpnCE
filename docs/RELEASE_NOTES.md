# rpnCE 0.8.0

An RPN scientific and financial calculator for the TI-84 Plus CE.

- Real and complex decimal arithmetic, rectangular and polar entry,
  scientific functions, LINK, and independent result/display modes.
- HP-style stack with 4–12 levels, LASTX, ordinary and indirect registers,
  unit quantities, statistics, curve fitting, TVM, BASE, and DATE functions.
- Mathematical complex separators and an always-visible a𝑖b/r∠θ/r∠°
  presentation indicator; RRES/CRES are marked only in MODE.
- Compact status indicators, DEG as the initial angle mode, decimal-point
  or decimal-comma display, and nine screen tints including TIOS.
- On-device HELP and an illustrated guide with complex-number examples.
- HOME has HELP on row 1 and TVM on row 2. All menu rows have five buttons,
  with unused slots blank. UNIT, BASE and HYP are reached through HOME.
- MODE row 4 separates DATE display from decimal point/comma on TRACE/GRAPH.
  Brightness uses 16-unit steps between the brightest and visible minimum.

The [0.8.0 package](../cem/rpnCE-0.8.0.zip) includes matching program and
resource files, installation notes, the license, screenshots and
`rpnCE-promo1.gif`. Source builds produce `bin/rpnCE.8xp` and
`bin/rpnCEres.8xv`; resource format is 4.

## Not compatible with 0.7.0 saved data

Complex-number support changed how every stack and register value is
stored, so 0.8.0 does not read the calculator data (`RPNCEdat`, format 5)
saved by 0.7.0 and earlier. The first start after upgrading has an empty
stack, LASTX, registers and statistics, with the default stack depth,
angle, display, complex, BASE and TVM solver modes. The old `RPNCEdat` is
replaced when you quit.

- **Before upgrading,** note any stack or register values you need, then
  re-enter them in 0.8.0. A copy of the old `RPNCEdat` can only be used
  with 0.7.0.
- **Carried over:** the screen tint, DATE settings and decimal separator in
  `RPNCEcfg`, and the shared TI-OS TVM values and hardware clock.
- **Delete `RPNCEunt`** if a 0.7.0 installation left one; 0.8.0 does not
  use it.

0.8.0 saves calculator data in format 6, which earlier versions cannot read.

See the [User Guide](USER_GUIDE.md) for operating instructions and
[installation](guide/02-installation.md) for setup and upgrade details.
