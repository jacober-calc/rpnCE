# Roadmap

## Milestone 1: working core
- [x] Project skeleton builds with the CE toolchain
- [x] RPN stack (X, Y, Z, T) and LASTX, using TI-OS 14-digit reals
- [x] Number entry line (digits, decimal point, EE, sign, backspace)
- [x] ENTER, stack lift/drop rules, X<>Y, R-down, CLEAR
- [x] + - * / and the dedicated-key math functions
- [x] Display: status line, stack, entry line (menu row is a placeholder)
- [x] Display style: native resolution, 4 stack lines, numbers drawn up to 3x size

## Milestone 2: keyboard-first RPN (HP-32SII style)
- [x] Full keyboard mapping, including 2ND functions
- [x] Variables A-Z on the green ALPHA letters, with STO/RCL arithmetic
- [x] CLEAR CLEAR clears the stack
- [x] Menu row only appears when opened, closes after a pick
- [x] State saved and restored between runs; idle timeout

## Milestone 3: lite menus (replaced by milestone 4)
- [x] Menu file supports several small menus, each opened by its own key
- [x] MATH: X^3, cube root, XROOTY, LOGB, 2^X
- [x] PROB (2ND DISTR): COMB, PERM, N!, RAND, SEED
- [x] MODE: DEG, RAD, FIX, SCI, ENG, ALL

## Milestone 4: RPN83P-style menus
- [x] Full RPN83P menu tree, always shown, MATH = HOME, MODE = MODE menu
- [x] Dark menu boxes with inverted labels; T: Z: Y: X: labels; dot-matrix font
- [x] MATH: all ten functions
- [x] TVM: N, I%YR, PV, PMT, FV, P/YR, C/YR, BEG/END, solver settings
- [x] UNIT: all 169 units in 12 kinds, UVAL, UBAS
- [x] STAT: Sigma+/-, all sums, means, deviations, covariance, curve fits
- [x] Registers numbered HP-32SII style: A-Z, i, (i), stat registers, up to
      100 extra (RSIZ); VARS catalog; x<> register; CLRG
- [x] Stack depth 4-12 (SSIZ); annunciators for depth and registers
- [x] NUM (PRIM, RNDF, RNDN, RNDG), HYP, CONV (polar, HMS), BASE
- [x] DATE: seven object types, entry, arithmetic, all D/T/DT/TZ/DZ/DR/DW,
      EPCH and CLK functions, {..} / ".." display
- [ ] Remaining: CPLX (and its MODE settings), HELP
- [x] Menu tree generated from a menu definition file (menus/menus.txt)
- [x] Menu navigation: softkeys, row up/down, HOME, EXIT
- [x] DEG/RAD mode
- [x] Working menu items: STK, CLR (CLX, CLST, CLD), MODE (RAD, DEG),
      CONV (>DEG, >RAD), PROB (COMB, PERM, N!),
      NUM (%, %CH, GCD, LCM, IP, FP, FLR, CEIL, NEAR, ABS, SIGN, MOD, MIN, MAX),
      MATH (X^3, cube root, XROOTY, 2^X, LOG2, LOGB)
- [x] Remaining MATH/NUM/PROB/CONV items (ATN2, E^X-, LN1+, PRIM, rounding, RAND)

## Later
- Lite 32SII-style menus on keys like MATH, STAT, APPS (angle), PRGM, VARS
- BASE, STAT, TVM, complex numbers, units, dates

## Size
- [x] Program stored compressed (COMPRESSED = YES in the makefile); it
      unpacks itself into RAM when started, so the 64 KB file limit no
      longer caps the program's real size
- [x] Menus and unit tables in the RPNCEres appvar, read from archive (no
      RAM); version-stamped, with a clear message if it's missing
- [x] Release packaging: tools/release.sh builds RPNCE-<version>.zip
- [ ] HELP text in an appvar
