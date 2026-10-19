# 14. UNIT: unit conversions

RPNCE 0.7.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 13. TVM: time value of money](13-tvm.md) · [Next: 15. DATE: dates, times and the clock](15-date.md)

**In this chapter**

- [Attaching and converting units](#attaching-and-converting-units)
- [Arithmetic with units](#arithmetic-with-units)
- [Units and the NUM functions](#units-and-the-num-functions)
- [UFCN](#ufcn)
- [Categories](#categories)
- [Example: store and recall a measurement](#example-store-and-recall-a-measurement)
- [Example: compare energy in different units](#example-compare-energy-in-different-units)
- [Choosing and removing a unit](#choosing-and-removing-a-unit)
- [Temperature and fuel conventions](#temperature-and-fuel-conventions)

## Attaching and converting units

Type a value, then press a unit in the **UNIT** menu to attach it: X shows
`12. ft`. Press another unit of the same kind to convert: **m** gives
`3.6576 m`. The unit label moves with the value on the stack, into
registers, and through quitting and restarting.

![A value with a unit, and after converting it](../../assets/guide/units.png)

| Example | Result |
| --- | --- |
| `12` **ft**, then **m** (both in **LENG**) | 3.6576 m |
| `100` **°F**, then **°C** (**TEMP**) | 37.7778 °C |
| `30` **mpg**, then **Lkm** (**FUEL**) | 7.8405 L/100 km |
| `1` **acre**, then **m²** (**AREA**) | 4,046.8564224 m² |

Converting between different kinds, such as metres to kilograms, gives
"Err: Unit Mismatch".

## Arithmetic with units

Values with units can be used in arithmetic, following the same rules as
RPN83P:

| Operation | Result |
| --- | --- |
| unit value `+` or `−` unit value | X is converted to Y's unit; the result keeps Y's unit |
| number `×` unit value (either order) | the unit is kept |
| unit value `÷` number | the unit is kept |
| unit value `÷` unit value (same kind) | a plain number, the ratio |

| Example | Result |
| --- | --- |
| `6` **ft** `2` **in** `+` | 6.1667 ft |
| ...then `3 ÷` | 2.0556 ft |
| ...then **in** | 24.6667 in |
| `3` **kg** `4 ×` | 12 kg |
| `6` **ft** `2` **in** `÷` | 36 (6 ft is 36 times 2 in) |

The result of `+` and `−` keeps **Y's** unit, which suits adding up
measurements: put the first one in place, then add the rest in any unit.

Some combinations aren't supported and give "Err: Data Type":

- adding or subtracting a plain number and a unit value (`6` **ft** `2 +`:
  2 what?);
- multiplying two unit values, or dividing a number by a unit value;
- arithmetic on **temperatures** and **fuel economy**, since their offsets
  and reciprocals make ordinary arithmetic misleading. They can still be
  converted and rounded.

Mixing different kinds, such as `5` **m** `1` **kg** `+`, gives
"Err: Unit Mismatch".

## Units and the NUM functions

| Function | With a unit value |
| --- | --- |
| **%** | X percent of Y, keeping Y's unit: `200` **km** `15` **%** → 30 km |
| **%CH** | the percent change from Y to X, after converting X to Y's unit: a plain number |
| **MIN**, **MAX**, **MOD** | X is converted to Y's unit; the result keeps it |
| **ABS**, **IP**, **FP**, **FLR**, **CEIL**, **NEAR**, rounding | the unit is kept |
| **SIGN** | a plain number |

For example, the percent change from 2 kWh to 2000 kcal (about 2.326 kWh):
`2` **kWh** `2000` **kcal** **%CH** → 16.2222 (%).

Temperatures and fuel economy allow only the rounding-type functions (IP,
FP, FLR, CEIL, NEAR, RNDF, RNDN and RNDG); the rest give "Err: Data Type".

Statistics, TVM and the size and format settings take plain numbers only,
and a value with a unit can't be used as a register index.

## UFCN

**UFCN** has **UVAL** (remove the unit, keeping the number shown) and
**UBAS** (convert to the kind's base unit: m, m², m³, K, kg, N, Pa, J, W,
s, m/s or L/100 km).

## Categories

**LENG** (length), **AREA**, **VOL** (volume), **TEMP** (temperature),
**MASS**, **FORC** (force), **PRES** (pressure), **ENER** (energy), **PWR**
(power), **TIME**, **SPD** (speed) and **FUEL** (fuel economy). Some have
submenus: survey, nautical and astronomical lengths; US, imperial and dry
volumes; US, imperial and troy masses. Every unit is listed in
[Appendix C](19-reference.md#appendix-c-units).

Every conversion factor is computed from the unit's exact definition (1 ft
is exactly 0.3048 m, 1 US gallon is 231 in³). Where definitions differ, RPNCE
uses: the US survey foot for survey units, the thermochemical calorie
(4.184 J), the International Table Btu, football fields including the end
zones, and a cable of 120 fathoms.


## Example: store and recall a measurement

**Setup:** DEC, FIX 4. Open **UNIT** → **LENG**. This exercise replaces A.

| Keys or selection | Result | Explanation |
| --- | --- | --- |
| `12` **ft** | `12.0000 ft` | Attach feet to the numeric X value. |
| `STO→ MATH` | X unchanged | Store the value and its unit in A. |
| `CLEAR` | `0.0000` | Clear X. |
| `2ND RCL MATH` | `12.0000 ft` | Restore the complete measurement. |
| **m** | `3.6576 m` | Convert X; the stored A remains 12 ft. |

VARS also shows the unit on A. Ordinary stored quantities survive a normal
exit and restart. Recalling, exchanging, or rolling a value carries its
unit with it. Selecting **UVAL** explicitly removes a unit while retaining
the displayed numeric magnitude.

## Example: compare energy in different units

**Setup:** DEC, FIX 4; **UNIT** → **ENER**.

| Keys | Y | X | Explanation |
| --- | --- | --- | --- |
| `2` **kWh** | — | 2 kWh | Initial energy. |
| `2000` **kcal** | 2 kWh | 2000 kcal | New entry lifts the first measurement. |
| **HOME** → **NUM** → **%CH** | 2 kWh | approximately 16.2222 | Convert internally, then calculate percentage change. |

Use kWh from **ENER**, not kW from **PWR**: power and energy are different
quantities. A kilowatt is a rate of energy use; a kilowatt-hour is energy.
The result of %CH is a plain percentage, while the original Y is preserved.

## Choosing and removing a unit

Attaching a unit to a plain number gives that number a physical meaning.
Selecting another compatible unit converts the quantity; it does not just
change the label. To reinterpret a magnitude in an unrelated unit, remove
the old unit with UVAL first and then attach the new one. That is an
explicit reinterpretation, rather than a conversion.

Different kinds do not combine into derived units in this version. For
example, length × length does not create area, and energy ÷ time does not
create power. Use plain numeric magnitudes and attach the intended result
unit yourself only when you have handled the dimensional conversion.

## Temperature and fuel conventions

Temperatures are absolute readings, rather than temperature differences.
Offsets make ordinary addition ambiguous, so temperature quantities allow
conversion and rounding rather than addition or subtraction. Fuel economy
uses a reciprocal conversion between mpg and L/100 km and has the same
arithmetic restriction.

The **FUEL** label **mpg** uses the US gallon. **Lkm** means litres per
100 kilometres. The temperature menu displays **°K** for consistency with
its other labels; the SI unit symbol for kelvin is K.

---

[Manual contents](../USER_GUIDE.md) · [Previous: 13. TVM: time value of money](13-tvm.md) · [Next: 15. DATE: dates, times and the clock](15-date.md)
