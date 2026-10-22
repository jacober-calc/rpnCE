# RPNCE 0.7.0

An HP-style RPN calculator for the TI-84 Plus CE, with TI-OS 14-digit
decimal arithmetic, a 4-12 level stack, registers, units, statistics,
time-value-of-money calculations, BASE operations and DATE objects.

## Changes

- Quantities retain units in ordinary registers through storage, recall,
  exchange and compatible register arithmetic. Compatible linear units
  convert automatically during supported calculations.
- Corrected LCM results and DATE arithmetic, including large day offsets
  and date differences. PRIM preserves the original number in Y for
  successive factorization.
- Separate calculator and application time zones let TI-OS keep local
  time while RPNCE works in another fixed UTC offset.
- TVM values appear in VARS, with LASTX consistently listed last.
- Expanded on-device HELP and a chapter-based user guide with examples,
  screenshots and function references.
- Decimal-point and decimal-comma preferences, grouping during number
  entry, and brightness controls on `2ND UP/DOWN`.
- Taller message text and Ayu Dark, Monokai, Pastel and Commodore tints.
- Consolidated release checks and conservative internal cleanup.

## Installation and existing data

Send `RPNCE.8xp` and `RPNCEres.8xv` from the same package. The CE C libraries
are also required. See `INSTALL.txt` and the
[installation chapter](guide/02-installation.md).

Existing 0.6.6 numeric and DATE state remains readable. Register-unit
metadata is separate in `RPNCEunt`. Back up `RPNCEdat`, `RPNCEcfg` and
`RPNCEunt` together when present. TVM values are shared with TI-OS.

## Current limits

- Complex numbers remain unavailable; reserved CPLX and MODE entries
  do not enable complex calculations.
- Conversions operate within a dimension. Compound units and general
  dimensional algebra are outside this release. Temperature and fuel
  units support conversion and rounding with restricted arithmetic.
- Time zones use fixed offsets; daylight-saving changes are manual.

The [User Guide](USER_GUIDE.md) describes the supported operations.
