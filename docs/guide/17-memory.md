# 17. Saving, memory and files

rpnCE 0.7.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 16. Messages and errors](16-troubleshooting.md) · [Next: 18. Limits and differences](18-compatibility.md)

**In this chapter**

- [Files on the calculator](#files-on-the-calculator)
- [The TVM values](#the-tvm-values)
- [Memory](#memory)
- [Backing up your work](#backing-up-your-work)
- [Interaction with TI-OS](#interaction-with-ti-os)
- [Removing rpnCE](#removing-rpnce)

rpnCE saves automatically when you quit (`2ND QUIT`, `2ND OFF`, or after
five minutes without a key press), and restores completed stack values,
LASTX, registers, statistics, and settings when it starts. It always
starts at the HOME menu. An unfinished DATE record is not a saved editor
session; complete it before quitting.

## Files on the calculator

| Appvar | Holds | Made by |
| --- | --- | --- |
| `RPNCEres` | the menus, unit tables and HELP pages | sent by you, from each release |
| `RPNCEdat` | the stack (with units), LASTX, registers, statistics and settings | rpnCE, when you quit |
| `RPNCEcfg` | the screen tint, DATE settings and decimal separator preference | rpnCE, when you quit |
| `RPNCEunt` | units on registers | rpnCE, only when a register holds a unit |

All of them are kept in archive memory, so they survive a RAM clear. You
can see them in `2ND MEM` > Mem Management > AppVars.

- **To reset rpnCE's private saved state**, exit and delete `RPNCEdat`,
  `RPNCEcfg`, and `RPNCEunt` there. Keep `RPNCEres`: rpnCE needs it to
  start. This does not clear shared TI-OS TVM values or reset the clock.
- **Upgrading** keeps all your data.
- **Register units are stored separately on purpose.** `RPNCEunt` carries
  a checksum and a fingerprint of the register values its units belong
  to. If it's ever missing or damaged, or doesn't match, the registers load
  without their units; your values are never lost.

## The TVM values

The TVM values (N, I%YR, PV, PMT, FV, P/YR and C/YR) are the calculator's
own finance variables, shared with TI's TVM Solver, so they're kept by
TI-OS rather than in rpnCE's appvars.

## Memory

rpnCE is stored compressed and unpacks into RAM when it starts. The 0.7.0
program image and static variables occupy about 90 KB before runtime stack
and library allocations; this is not a complete measurement of peak RAM.
Menus, unit tables, and HELP text are read from archive. Available memory
therefore needs to accommodate more than just the compressed program file.
Archive other files or free memory if the OS cannot load the program.


## Backing up your work

1. Finish current entry and exit with `2ND QUIT` to save the latest state.
2. In TI Connect CE, copy `RPNCEdat`, `RPNCEcfg`, and `RPNCEunt` if present
   to one backup folder on your computer.
3. Keep the matching rpnCE program and resource files, or note their build.
4. Record the seven shared TVM values separately if you need to preserve
   the finance problem; rpnCE's saved appvars do not contain those values.

Restore the saved-data files as a set from the same backup. In particular,
RPNCEunt belongs to the register values in RPNCEdat. If a mismatched unit
file is restored, rpnCE keeps the numeric values but declines to attach
unverified unit labels. A missing RPNCEunt is normal when no ordinary
register holds a unit.

## Interaction with TI-OS

| Data | Ownership and effect |
| --- | --- |
| rpnCE A–Z, i, stack, LASTX, extra registers | Private rpnCE values; TI-OS scalar A–Z and matrix [A] are not used for these registers. |
| rpnCE statistics | Private accumulated sums; the program does not use TI-OS lists as its observation store. |
| N, I%YR, PV, PMT, FV, P/YR, C/YR | Shared TI-OS finance variables; stores, solves, and CLTV affect the values visible to TI's solver. |
| Hardware clock | Shared device clock; SETC changes it. ATZ and CTZ are rpnCE interpretation settings. |
| RPNCEres | Read-only program resources supplied with a build. |
| RPNCEdat, RPNCEcfg, RPNCEunt | rpnCE's own saved state, created on normal exit as needed. |

Archive protects saved appvars from a RAM clear, not from deleting files
or clearing archive memory. A normal exit is the point at which rpnCE
writes its private state; a forced interruption before saving cannot
preserve work that has not yet been written.

`2ND ↑` and `2ND ↓` adjust the calculator's shared TI-OS brightness
preference and LCD backlight. They do not create an additional appvar.

## Removing rpnCE

After backing up anything you need, delete the rpnCE program, RPNCEres,
and its three private saved-data appvars. The CE libraries may also be
used by other programs; keep them if those programs need them. Removing
rpnCE does not itself erase shared finance values or alter the clock.

---

[Manual contents](../USER_GUIDE.md) · [Previous: 16. Messages and errors](16-troubleshooting.md) · [Next: 18. Limits and differences](18-compatibility.md)
