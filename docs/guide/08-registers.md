# 8. Registers

RPNCE 0.7.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 7. The menus](07-menus.md) · [Next: 9. Display formats and MODE settings](09-settings.md)

**In this chapter**

- [The registers](#the-registers)
- [Storing and recalling](#storing-and-recalling)
- [Register arithmetic](#register-arithmetic)
- [Exchanging X with a register](#exchanging-x-with-a-register)
- [Indirect registers: i and (i)](#indirect-registers-i-and-i)
- [Adding registers](#adding-registers)
- [The statistics registers](#the-statistics-registers)
- [The VARS catalog](#the-vars-catalog)
- [Clearing registers](#clearing-registers)
- [Example: accumulating in A](#example-accumulating-in-a)
- [Example: using an extra register](#example-using-an-extra-register)

## The registers

Registers store numbers, numbers with units, and DATE objects. They are
numbered as on the HP-32SII:

| Number | Register |
| --- | --- |
| 001-026 | `A` to `Z` |
| 027 | `i`, the index register |
| 028-033 | `n`, `Σx`, `Σy`, `Σx²`, `Σy²`, `Σxy`: the statistics registers |
| 034-040 | `Σlnx`, `Σlnx²`, `Σlny`, `Σlny²`, `Σlnxlny`, `Σxlny`, `Σylnx`: sums for curve fits |
| 041-140 | extra registers, when you add them (below) |

## Storing and recalling

Press `STO→`, then the key whose green ALPHA letter names the register: the
prompt shows `STO _` in the X row, and the next key completes it. So
`STO→ MATH` stores X in **A**, `STO→ SIN` in **E**, and so on. `2ND RCL`
works the same way.

![The STO prompt, completed](../../assets/guide/sto-prompt.png)

When you complete a prompt, it shows the full command, such as `STO A`, for
a moment before it runs.

| In the prompt | Means |
| --- | --- |
| a letter key | that register, A to Z |
| `.` | register `i` |
| `2ND .` | `(i)`: the register whose number is in `i` |
| `+ − × ÷` first | register arithmetic (below) |
| `ALPHA`, then the key | the letter, for `− × ÷` (W, R, M) |
| `CLEAR`, `DEL` or `ON` | cancel |

- **STO** copies X into the register and never moves the stack.
- **RCL** brings the register's value into X like typing a number: it
  lifts the stack, except right after ENTER, when it replaces the copy.
- A value's **unit** goes with it: store `12 ft` in A, and RCL A gives
  back `12 ft`. Register arithmetic follows the unit rules in
  [chapter 14](14-units.md#14-unit-unit-conversions).

## Register arithmetic

Press `+ − × ÷` straight after `STO→` or `2ND RCL`, then the register:

| Keys | Effect |
| --- | --- |
| `STO→ + MATH` | A = A + X (STO+ A) |
| `STO→ × MATH` | A = A × X |
| `2ND RCL + MATH` | X = X + A (RCL+ A); LASTX keeps the old X |
| `2ND RCL ÷ MATH` | X = X ÷ A |

Because `−`, `×` and `÷` are also the letters W, R and M, press `ALPHA`
before them to name those registers: `STO→ ALPHA ×` stores in R.

## Exchanging X with a register

**STK** row 2, **X<>**, swaps X with a register: **X<>** then a register
key, with the same register choices as STO.

## Indirect registers: i and (i)

`(i)` means "the register whose number is in `i`", using the whole-number
part of `i`. This reaches any register by number, including the extra
ones:

| Keys | Effect |
| --- | --- |
| `1 STO→ .` | i = 1 |
| `2ND RCL 2ND .` | recalls register 001, which is A |
| `28 STO→ .`, `2ND RCL 2ND .` | recalls register 028, n (the number of stat points) |
| `41 STO→ .`, `123 STO→ 2ND .` | stores 123 in register 041 (if it exists) |

If `i` doesn't name an existing register, the result is "Err: Index". A
value with a unit can't be used as an index.

## Adding registers

The standard 40 registers are A to Z, i, and the 13 statistics registers.
`MODE`, row 3, **RSIZ** sets the total from X, from 40 to 140 (a value
below 40 means 40): `60` **RSIZ** adds registers 041 to 060, which you
reach through `(i)`. **RSZ?** puts the total in X. Reducing the size
discards the removed registers' values.

## The statistics registers

The statistics functions keep their running sums in registers 028 to 040,
so you can read them with `(i)` or the **STAT** > **Σ** menu. You can also
store into them. Changing one of the first six by hand (n, Σx, Σy, Σx²,
Σy², Σxy) leaves the other sums out of step, so only the linear curve fit
stays available until you clear the statistics with **CLΣ**.

## The VARS catalog

`VARS` lists, across the four stack lines:

1. every register that holds a value other than zero, or a unit, by name
   (`A`, `i`, `n`, `R041` and so on);
2. the seven TVM values: `N`, `I%YR`, `PV`, `PMT`, `FV`, `P/YR` and
   `C/YR`, always, even when zero ([chapter 13](13-tvm.md#13-tvm-time-value-of-money));
3. `LASTX`, always, as the last entry.

![The VARS catalog](../../assets/guide/vars-catalog.png)

The up and down arrows move through the list; `ENTER` copies the selected
value to X (with the usual stack lift) and closes the catalog; `ON`,
`CLEAR` or `VARS` closes it without copying.

## Clearing registers

**CLR** > **CLRG** clears A to Z, i and the extra registers. The
statistics are kept; clear them with **CLΣ**.


## Example: accumulating in A

**Starting settings:** DEC, FIX 4. This exercise replaces A.

| Keys | A | X | Explanation |
| --- | --- | --- | --- |
| `100 STO→ MATH` | 100 | 100 | Store the starting amount in A. |
| `25 STO→ + MATH` | 125 | 25 | Add X to A without moving the stack. |
| `2ND RCL MATH` | 125 | 125 | Recall the updated register. |
| `5` | 125 | 5 | Start a second operand; 125 moves into Y. |
| `2ND RCL ÷ MATH` | 125 | 0.04 | RCL÷ divides X by A; A remains 125. |

STO arithmetic writes to the named register. RCL arithmetic replaces X.
For subtraction and division, the order is important:

| Operation | Assignment |
| --- | --- |
| STO+ / STO− / STO× / STO÷ A | A ← A + X / A − X / A × X / A ÷ X |
| RCL+ / RCL− / RCL× / RCL÷ A | X ← X + A / X − A / X × A / X ÷ A |

A register can contain a plain number, a supported quantity, or a DATE
object. Arithmetic requires compatible operands. Statistics registers
accept plain numbers; TVM values are accessed through the TVM menu and
VARS rather than through numbered-register addressing.

## Example: using an extra register

**Starting settings:** DEC; at least 41 total registers. Select **MODE**
row 3, enter `41`, and press **RSIZ** if you need to expand the register
set. A larger existing register set need not be reduced.

1. Enter `41 STO→ .` to put the destination number in i.
2. Enter `123 STO→ 2ND .` to store 123 in register 041.
3. Clear X and press `2ND RCL 2ND .` to recall 123.
4. Open VARS to find `R041=123`.

Selecting **RSIZ** replaces the size setting; reducing the size discards
registers above the new limit. **CLRG** clears ordinary register values
without reducing the register count.

---

[Manual contents](../USER_GUIDE.md) · [Previous: 7. The menus](07-menus.md) · [Next: 9. Display formats and MODE settings](09-settings.md)
