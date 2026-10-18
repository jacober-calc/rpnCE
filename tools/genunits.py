#!/usr/bin/env python3
"""Exact unit definitions, used by tools/genres.py to build the unit tables.

    python3 tools/genunits.py

Writes src/core/units_data.c (labels, dimensions, factors) and
src/ui/unit_map.c (menu item -> unit). Labels come from menus/menus.txt.
Each factor is the size of one unit in its dimension's base unit, computed
exactly here and rounded to the calculator's 14 digits.
"""
import os
import sys
from decimal import Decimal, getcontext
from fractions import Fraction as Fr

import mpmath

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import genmenu  # noqa: E402  (labels and symbol encoding)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_C = os.path.join(ROOT, 'src', 'core', 'units_data.c')
OUT_MAP = os.path.join(ROOT, 'src', 'ui', 'unit_map.c')

# ---- exact definitions ---------------------------------------------------
inch = Fr(254, 10000)                 # international inch
ft, yd = 12 * inch, 36 * inch
mile = 5280 * ft
svft = Fr(1200, 3937)                 # US survey foot
lb = Fr(45359237, 10**8)              # international avoirdupois pound
g0 = Fr(980665, 10**5)                # standard gravity
grain = lb / 7000
lbf = lb * g0
usgal = 231 * inch**3
impgal = Fr(454609, 10**8)
bushel = Fr(215042, 100) * inch**3
mmhg = Fr(133322387415, 10**9)        # conventional millimetre of mercury
btu = Fr(105505585262, 10**8)         # International Table Btu
cal = Fr(4184, 1000)                  # thermochemical calorie
c_light = 299792458
jyear = Fr(36525, 100) * 86400       # Julian year
au = 149597870700
mpmath.mp.dps = 40
parsec = mpmath.mpf(au) * 648000 / mpmath.pi

E = lambda n: Fr(10) ** n

# dimension, base-unit id
DIMS = {
    'LENGTH': 'UNIT_METER', 'AREA': 'UNIT_SQ_METER', 'VOLUME': 'UNIT_CU_METER',
    'TEMP': 'UNIT_KELVIN', 'MASS': 'UNIT_KILO_GRAM', 'FORCE': 'UNIT_NEWTON',
    'PRESSURE': 'UNIT_PASCAL', 'ENERGY': 'UNIT_JOULE', 'POWER': 'UNIT_WATT',
    'TIME': 'UNIT_SECOND', 'SPEED': 'UNIT_METER_PER_SECOND',
    'FUEL': 'UNIT_LITERS_PER_HUNDRED_KILO_METERS',
}

# id: (dimension, factor[, offset])   value_in_base = (value + offset) * factor
U = {}
def L(i, f): U[i] = ('LENGTH', f)
for i, f in [('FERMI', E(-15)), ('ANGSTROM', E(-10)), ('NANO_METER', E(-9)),
             ('MICRO_METER', E(-6)), ('MILLI_METER', E(-3)), ('CENTI_METER', E(-2)),
             ('METER', 1), ('KILO_METER', 1000), ('MIL', inch / 1000), ('INCH', inch),
             ('FOOT', ft), ('YARD', yd), ('MILE', mile),
             ('TWIP', inch / 1440), ('POINT', inch / 72), ('PICA', inch / 6),
             ('SURVEY_FOOT', svft), ('ROD', Fr(33, 2) * svft), ('CHAIN', 66 * svft),
             ('FURLONG', 660 * svft), ('SURVEY_MILE', 5280 * svft),
             ('LEAGUE', 3 * 5280 * svft),
             ('FATHOM', 6 * ft), ('CABLE', 720 * ft), ('NAUTICAL_MILE', 1852),
             ('LIGHT_SECOND', c_light), ('ASTRONOMICAL_UNIT', au),
             ('LIGHT_YEAR', c_light * jyear), ('PARSEC', parsec)]:
    L('UNIT_' + i, f)

def A(i, f): U['UNIT_' + i] = ('AREA', f)
for i, s in [('MILLI_METER', E(-3)), ('CENTI_METER', E(-2)), ('METER', 1),
             ('KILO_METER', 1000), ('INCH', inch), ('FOOT', ft), ('YARD', yd),
             ('MILE', mile), ('NAUTICAL_MILE', 1852), ('ROD', Fr(33, 2) * svft),
             ('CHAIN', 66 * svft), ('FURLONG', 660 * svft)]:
    A('SQ_' + i, s * s)
A('ACRE', 43560 * ft**2)
A('HECTARE', 10000)
A('USFOOTBALL', 360 * ft * 160 * ft)      # including end zones
A('CAFOOTBALL', 150 * yd * 65 * yd)       # including end zones

def V(i, f): U['UNIT_' + i] = ('VOLUME', f)
for i, s in [('MILLI_METER', E(-3)), ('CENTI_METER', E(-2)), ('METER', 1),
             ('KILO_METER', 1000), ('INCH', inch), ('FOOT', ft), ('YARD', yd),
             ('MILE', mile), ('NAUTICAL_MILE', 1852)]:
    V('CU_' + i, s**3)
for i, f in [('MICRO_LITER', E(-9)), ('MILLI_LITER', E(-6)), ('LITER', E(-3)),
             ('METRIC_TEASPOON', 5 * E(-6)), ('METRIC_TABLESPOON', 15 * E(-6))]:
    V(i, f)
usfloz = usgal / 128
for i, f in [('TEASPOON', usfloz / 6), ('TABLESPOON', usfloz / 2),
             ('FLUID_OUNCE', usfloz), ('GILL', usgal / 32), ('CUP', usgal / 16),
             ('PINT', usgal / 8), ('QUART', usgal / 4), ('GALLON', usgal)]:
    V(i, f)
impfloz = impgal / 160
for i, f in [('TEASPOON', impfloz * Fr(5, 24)), ('TABLESPOON', impfloz * Fr(5, 8)),
             ('FLUID_OUNCE', impfloz), ('GILL', impgal / 32), ('CUP', impgal / 16),
             ('PINT', impgal / 8), ('QUART', impgal / 4), ('GALLON', impgal)]:
    V('IMP_' + i, f)
for i, f in [('DRY_PINT', bushel / 64), ('DRY_QUART', bushel / 32),
             ('DRY_GALLON', bushel / 8), ('PECK', bushel / 4), ('BUSHEL', bushel),
             ('DRY_BARREL', 7056 * inch**3), ('BOARD_FOOT', 144 * inch**3),
             ('OIL_BARREL', 42 * usgal), ('OLYMPIC_POOL', 2500),
             ('ACRE_FOOT', 43560 * ft**3)]:
    V(i, f)

U['UNIT_CELSIUS'] = ('TEMP', 1, Fr(27315, 100))
U['UNIT_FAHRENHEIT'] = ('TEMP', Fr(5, 9), Fr(45967, 100))
U['UNIT_RANKINE'] = ('TEMP', Fr(5, 9))
U['UNIT_KELVIN'] = ('TEMP', 1)

def M(i, f): U['UNIT_' + i] = ('MASS', f)
for i, f in [('MICRO_GRAM', E(-9)), ('MILLI_GRAM', E(-6)), ('GRAM', E(-3)),
             ('KILO_GRAM', 1), ('METRIC_TON', 1000),
             ('ATOMIC_MASS_UNIT', Fr(166053906892, 10**38)),   # CODATA 2022
             ('GRAIN', grain), ('DRAM', lb / 256), ('OUNCE', lb / 16), ('POUND', lb),
             ('SLUG', lb * g0 / ft), ('SHORT_HUNDRED_WEIGHT', 100 * lb),
             ('SHORT_TON', 2000 * lb), ('IMP_POUND', lb), ('STONE', 14 * lb),
             ('QUARTER', 28 * lb), ('LONG_HUNDRED_WEIGHT', 112 * lb),
             ('LONG_TON', 2240 * lb), ('TROY_GRAIN', grain),
             ('TROY_PENNY_WEIGHT', 24 * grain), ('TROY_OUNCE', 480 * grain),
             ('TROY_POUND', 5760 * grain)]:
    M(i, f)

def Fo(i, f): U['UNIT_' + i] = ('FORCE', f)
for i, f in [('DYNE', E(-5)), ('NEWTON', 1), ('KILOGRAM_FORCE', g0),
             ('METRIC_TON_FORCE', 1000 * g0), ('POUNDAL', lb * ft),
             ('POUND_FORCE', lbf), ('SHORT_TON_FORCE', 2000 * lbf),
             ('LONG_TON_FORCE', 2240 * lbf)]:
    Fo(i, f)

def P(i, f): U['UNIT_' + i] = ('PRESSURE', f)
for i, f in [('PASCAL', 1), ('HECTO_PASCAL', 100), ('KILO_PASCAL', 1000),
             ('TORR', Fr(101325, 760)), ('ATMOSPHERE', 101325), ('MILLI_BAR', 100),
             ('DECI_BAR', 10000), ('BAR', 100000), ('POUND_SQUARE_INCH', lbf / inch**2),
             ('MILLI_METER_MERCURY', mmhg), ('INCH_MERCURY', Fr(254, 10) * mmhg),
             ('MILLI_METER_WATER', g0), ('INCH_WATER', Fr(254, 10) * g0)]:
    P(i, f)

def En(i, f): U['UNIT_' + i] = ('ENERGY', f)
for i, f in [('ELECTRON_VOLT', Fr(1602176634, 10**28)), ('ERG', E(-7)), ('JOULE', 1),
             ('KILO_JOULE', 1000), ('FOOT_POUND_ENERGY', ft * lbf), ('WATT_HOUR', 3600),
             ('KILO_WATT_HOUR', 3600000), ('CALORIE', cal), ('KILO_CALORIE', 1000 * cal),
             ('BRITISH_THERMAL_UNIT', btu), ('TON_TNT', Fr(4184, 1000) * E(9)),
             ('LITER_ATMOSPHERE', Fr(101325, 1000))]:
    En(i, f)

def Pw(i, f): U['UNIT_' + i] = ('POWER', f)
for i, f in [('WATT', 1), ('KILO_WATT', 1000),
             ('FOOT_POUND_ENERGY_PER_SECOND', ft * lbf), ('BTU_PER_HOUR', btu / 3600),
             ('BTU_PER_MINUTE', btu / 60), ('CALORIE_PER_SECOND', cal),
             ('HORSEPOWER', 550 * ft * lbf)]:
    Pw(i, f)

def T(i, f): U['UNIT_' + i] = ('TIME', f)
for i, f in [('NANO_SECOND', E(-9)), ('MICRO_SECOND', E(-6)), ('MILLI_SECOND', E(-3)),
             ('SECOND', 1), ('MINUTE', 60), ('HOUR', 3600), ('DAY', 86400),
             ('WEEK', 604800), ('JULIAN_MONTH', jyear / 12), ('JULIAN_YEAR', jyear)]:
    T(i, f)

def S(i, f): U['UNIT_' + i] = ('SPEED', f)
for i, f in [('METER_PER_SECOND', 1), ('FOOT_PER_SECOND', ft),
             ('KILO_METER_PER_HOUR', Fr(1000, 3600)), ('MILE_PER_HOUR', mile / 3600),
             ('KNOT', Fr(1852, 3600)), ('LIGHT_SPEED', c_light)]:
    S(i, f)

# Fuel economy: L/100km is the base; mpg is its reciprocal times K.
U['UNIT_LITERS_PER_HUNDRED_KILO_METERS'] = ('FUEL', 1)
MPG_K = 100 * (usgal * 1000) / (mile / 1000)          # 235.2145833...
U['UNIT_MILES_PER_GALLON'] = ('FUEL', MPG_K, 0, 'reciprocal')

# ---- output ----------------------------------------------------------------
def d14(x):
    """Rounds an exact value to 14 significant digits, as a TI number string."""
    getcontext().prec = 40
    if isinstance(x, Fr):
        v = Decimal(x.numerator) / Decimal(x.denominator)
    elif isinstance(x, int):
        v = Decimal(x)
    else:
        v = Decimal(mpmath.nstr(x, 35))
    getcontext().prec = 14
    return '{:.13E}'.format(+v).replace('E+', 'E')


def main():
    labels = {}
    for g in [genmenu.parse()[0]]:
        stack = [g]
        while stack:
            grp = stack.pop()
            for row in grp.rows:
                for n in row:
                    if isinstance(n, genmenu.Group):
                        stack.append(n)
                    elif n[0] == 'item' and n[3] and n[3].startswith('UNIT_'):
                        labels.setdefault(n[3], n[1])
    missing = sorted(set(labels) - set(U) - {'UNIT_VALUE', 'UNIT_BASE'})
    extra = sorted(set(U) - set(labels))
    if missing or extra:
        sys.exit(f'menus/units mismatch: missing {missing}, extra {extra}')
    for dim, base in DIMS.items():
        if base not in U or U[base][0] != dim:
            sys.exit(f'base unit {base} missing for {dim}')

    ids = [i for i in labels if i in U]          # menu order
    dims = list(DIMS)
    with open(OUT_C, 'w') as f:
        f.write('/* Generated by tools/genunits.py. Do not edit. */\n')
        f.write('#include "units.h"\n\n')
        f.write('const unit_t unit_table[UNIT_COUNT] = {\n')
        for i in ids:
            d = U[i]
            dim, fac = d[0], d[1]
            off = d[2] if len(d) > 2 else 0
            recip = len(d) > 3
            off_s = f'"{d14(off)}"' if off else '0'
            f.write(f'    {{ {genmenu.c_label(labels[i])}, DIM_{dim}, '
                    f'"{d14(fac)}", {off_s}, {1 if recip else 0} }},  /* {i} */\n')
        f.write('};\n\n')
        f.write('/* Base unit of each dimension, as an index into unit_table. */\n')
        f.write('const uint8_t unit_base[DIM_COUNT] = { '
                + ', '.join(str(ids.index(DIMS[d])) for d in dims) + ' };\n')

    with open(OUT_MAP, 'w') as f:
        f.write('/* Generated by tools/genunits.py. Do not edit. */\n')
        f.write('#include "unit_map.h"\n\n')
        f.write('int unit_for_menu(menu_id_t id)\n{\n    switch (id) {\n')
        for n, i in enumerate(ids):
            f.write(f'        case MID_{i}: return {n};\n')
        f.write('        default: return -1;\n    }\n}\n')

    with open(os.path.join(ROOT, 'src', 'core', 'units_count.h'), 'w') as f:
        f.write('/* Generated by tools/genunits.py. Do not edit. */\n')
        f.write(f'#define UNIT_COUNT {len(ids)}\n')
        f.write('enum { ' + ', '.join(f'DIM_{d}' for d in dims) + ', DIM_COUNT };\n')
    print(f'{len(ids)} units in {len(dims)} dimensions')


if __name__ == '__main__':
    # The unit tables are now built into the RPNCEres appvar by genres.py,
    # which uses this file as a library.
    import genres
    genres.main()
