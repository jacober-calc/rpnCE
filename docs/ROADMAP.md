# Roadmap

## Milestone 1: working core
- [x] Project skeleton builds with the CE toolchain
- [x] RPN stack (X, Y, Z, T) and LASTX, using TI-OS 14-digit reals
- [x] Number entry line (digits, decimal point, EE, sign, backspace)
- [x] ENTER, stack lift/drop rules, X<>Y, R-down, CLEAR
- [x] + - * / and the dedicated-key math functions
- [x] Display: status line, stack, entry line (menu row is a placeholder)
- [x] Display style: native resolution, 4 stack lines, numbers drawn up to 3x size

## Milestone 2: menus
- [x] Menu tree generated from a menu definition file (menus/menus.txt)
- [x] Menu navigation: softkeys, row up/down, HOME, EXIT
- [x] DEG/RAD mode
- [x] Working menu items: STK, CLR (CLX, CLST, CLD), MODE (RAD, DEG),
      CONV (>DEG, >RAD), PROB (COMB, PERM, N!),
      NUM (%, %CH, GCD, LCM, IP, FP, FLR, CEIL, NEAR, ABS, SIGN, MOD, MIN, MAX),
      MATH (X^3, cube root, XROOTY, 2^X, LOG2, LOGB)
- [ ] Remaining MATH/NUM/PROB/CONV items (ATN2, E^X-, LN1+, PRIM, rounding, RAND)
- [ ] FIX/SCI/ENG display modes
- [ ] Save state to an appvar on exit

## Later
- STO/RCL registers and variables
- BASE, STAT, TVM, complex numbers, units, dates
