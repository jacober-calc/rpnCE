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
- [ ] Menu tree generated from a menu definition file
- [ ] DEG/RAD mode (trig is radians-only for now)
- [ ] MATH, NUM, PROB, CONV, MODE (FIX/SCI/ENG, DEG/RAD)
- [ ] Save state to an appvar on exit

## Later
- STO/RCL registers and variables
- BASE, STAT, TVM, complex numbers, units, dates
