# RPNCE 0.7.0

An RPN calculator for the TI-84 Plus CE with TI-OS 14-digit decimal
arithmetic and a keyboard-oriented interface.

## Features

- HP-style stack with 4-12 levels, LASTX, ordinary and indirect registers.
- Arithmetic, scientific functions, statistics and curve fitting.
- TVM calculations shared with TI's finance variables, also accessible
  through VARS.
- 169 units in 12 groups, with compatible linear-unit arithmetic and
  quantity storage in ordinary registers.
- BASE operations on unsigned words up to 44 bits.
- Dates, times, durations, epochs and independent clock/application zones.
- Decimal-point or decimal-comma display, grouped number entry,
  brightness shortcuts and eight screen tints.
- On-device HELP and an illustrated user guide.

## Installation and saved data

Send `RPNCE.8xp` and `RPNCEres.8xv` from the same package. The CE C libraries
are required. See `INSTALL.txt` and the
[installation chapter](guide/02-installation.md).

RPNCE saves its state in `RPNCEdat`, display/clock settings in `RPNCEcfg`,
and register-unit metadata in `RPNCEunt` when needed. Back up these appvars
together. TVM values are shared with TI-OS.

## Limits

- Complex numbers are unavailable; reserved CPLX and MODE entries do not
  enable complex calculations.
- Unit conversions stay within a dimension. Compound units and general
  dimensional algebra are unsupported. Temperature and fuel units support
  conversion and rounding with restricted arithmetic.
- Time zones use fixed offsets; daylight-saving adjustments are manual.

The [User Guide](USER_GUIDE.md) describes the supported operations.
