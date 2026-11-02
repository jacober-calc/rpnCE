# Unreleased

- Unified real, complex, DATE and unit values across stack movement,
  register arithmetic, parsing, formatting and saved state.
- Added RPN83P-style complex entry, LINK, modes and scientific arithmetic.
- Made the status bar compact: format always shows; RAD, CRES, PRAD/PDEG,
  nondecimal bases, carry and modifiers appear only when active.
  DEG, RRES, RECT and DEC are implicit defaults; FIX with all digits shows ALL.
  Crowded status bars show the time without the date.
- Fresh calculators start in DEG; saved angle settings are restored.
- Refactored shared command traits, numerical call state and storage
  workspace lifetimes to reduce code size and static RAM. Persistence
  formats and calculator operations retain their existing contracts.
- Added a dedicated complex-number guide chapter with entry, arithmetic,
  CPLX, all six result/display states, formatting, LINK and storage examples.
  Updated chapter navigation, mode and number-type references and the
  generated function index. HELP now has 26 pages,
  including complex operations, number types and compact status indicators.
- Replaced separate unit metadata and packed DATE runtime slots. Version-5
  state is imported at the file boundary; new version-6 saves carry complete
  values and cannot be read by older program versions.

# rpnCE 0.7.0

These historical notes describe the released binary; the current User
Guide includes the development changes listed above.

The first official pre-release of rpnCE.

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

Send `rpnCE-0.7.0.8xp` and `rpnCEres-0.7.0.8xv` from the same package. The CE C libraries
are required. See `INSTALL.txt` and the
[installation chapter](guide/02-installation.md).

rpnCE saves its state in `RPNCEdat`, display/clock settings in `RPNCEcfg`,
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
