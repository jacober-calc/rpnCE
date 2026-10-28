#!/usr/bin/env python3
"""Fills in the generated appendices of docs/guide/19-reference.md:

  Appendix B (the menu map), from menus/menus.txt
  Appendix C (the units), from menus/menus.txt and tools/genunits.py
  Appendix D (the function index), from menus/menus.txt

    python3 tools/guide_appendix.py

The text between each pair of BEGIN/END markers is replaced; everything
else in the guide is left alone. Run it after changing the menus or units.
Needs the mpmath package (via genunits.py).
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import genmenu   # noqa: E402
import genunits  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GUIDE = os.path.join(ROOT, 'docs', 'guide', '19-reference.md')

# menu label symbols, as shown on the calculator
SYMBOLS = {'sq': '²', 'cube': '³', 'cuberoot': '³', 'root': '√', 'sigma': 'Σ',
           'to': '→', 'block': '', 'mu': 'µ', 'deg': '°', 'up': '↑', 'down': '↓',
           'left': '←', 'times': '×', 'supx': 'ˣ'}

DIM_NAMES = {'LENGTH': 'Length', 'AREA': 'Area', 'VOLUME': 'Volume', 'TEMP': 'Temperature',
             'MASS': 'Mass', 'FORCE': 'Force', 'PRESSURE': 'Pressure', 'ENERGY': 'Energy',
             'POWER': 'Power', 'TIME': 'Time', 'SPEED': 'Speed', 'FUEL': 'Fuel economy'}


def label(text):
    out = re.sub(r'\{(\w+)\}', lambda m: SYMBOLS.get(m.group(1), m.group(0)), text)
    return out.replace('|', '\\|')


def node_label(n):
    if isinstance(n, genmenu.Group):
        return f'[{label(n.label)}]'
    kind, text, alt = n[0], n[1], n[2]
    if kind == 'blank':
        return '·'
    if kind == 'alt':
        return label(text)
    return label(text)


def menu_map(roots):
    lines = ['Submenus are shown in brackets, such as [NUM]; · is an empty slot.',
             'Each row lists the five menu keys, from Y= to GRAPH.', '']

    def walk(g, path, raw):
        lines.append(f'**{" > ".join(path)}**')
        lines.append('')
        lines.append('| Row | Keys | Screen |')
        lines.append('| --- | --- | --- |')
        for i, row in enumerate(g.rows, 1):
            cells = [node_label(n) for n in row] + ['·'] * (5 - len(row))
            shot = (f'<img src="../../assets/guide/menu/{menu_file(raw)}-{i}.png" '
                    f'alt="{" > ".join(path)} > Row {i}" width="320">')
            lines.append(f'| {i} | ' + ' &nbsp; '.join(cells) + f' | {shot} |')
        lines.append('')
        for row in g.rows:
            for n in row:
                if isinstance(n, genmenu.Group):
                    walk(n, path + [label(n.label)], raw + [n.label])

    for r in roots:
        walk(r, [label(r.label)], [r.label])
    return lines


def menu_file(raw_path):
    """The menu-row image name made by tools/screenshots/run.py:
    ['HOME', 'STAT', '{sigma}'] -> 'home-stat-sigma'."""
    return '-'.join(re.sub(r'[^a-z0-9]+', '', re.sub(r'\{(\w+)\}', r'\1', p.lower())) for p in raw_path)


def unit_list(roots):
    # unit labels and their menu location, in menu order
    found = []

    def walk(g, path):
        for row in g.rows:
            for n in row:
                if isinstance(n, genmenu.Group):
                    walk(n, path + [label(n.label)])
                elif n[0] == 'item' and n[3] in genunits.U:
                    found.append((n[3], label(n[1]), ' > '.join(path)))

    unit_root = None
    stack = list(roots)
    while stack:
        g = stack.pop()
        for row in g.rows:
            for n in row:
                if isinstance(n, genmenu.Group):
                    if n.label == 'UNIT':
                        unit_root = n
                    stack.append(n)
    walk(unit_root, ['UNIT'])

    lines = ['Each unit is listed in menu order with its value in the base unit',
             'selected by UBAS. Numeric factors are rounded to at most 12 significant',
             'digits for this table; the calculator uses 14-digit conversion factors.',
             'Temperature entries are formulas; reciprocal fuel entries divide by',
             'the quantity shown. See [Units](14-units.md) for usage and conventions.', '']
    by_dim = {}
    for ident, lab, where in found:
        dim = genunits.U[ident][0]
        by_dim.setdefault(dim, []).append((ident, lab, where))
    for dim in genunits.DIMS:
        if dim not in by_dim:
            continue
        base_id = genunits.DIMS[dim]
        base_label = next(l for i, l, w in by_dim[dim] if i == base_id)
        lines.append(f'### {DIM_NAMES.get(dim, dim.title())}')
        lines.append('')
        lines.append(f'| Unit | Menu | Value in {base_label} |')
        lines.append('| --- | --- | --- |')
        for ident, lab, where in by_dim[dim]:
            d = genunits.U[ident]
            factor = genunits.d14(d[1])
            value = number(float(factor))
            if dim == 'TEMP':
                off = genunits.d14(d[2]) if len(d) > 2 and d[2] else None
                tf = temp_factor(float(factor))
                value = (f'({lab} + {number(float(off))}){tf}' if off else f'{lab}{tf}')
            elif len(d) > 3:      # reciprocal (fuel economy)
                value = f'{number(float(factor))} ÷ {lab}'
            lines.append(f'| {lab} | {where} | {value} |')
        lines.append('')
    return lines


SUPERSCRIPT = str.maketrans('-0123456789', '⁻⁰¹²³⁴⁵⁶⁷⁸⁹')


def number(v):
    """12 significant digits, with large and small values as a × 10ⁿ."""
    g = f'{v:.12g}'
    if 'e' not in g:
        return g
    m, e = g.split('e')
    return f'{m} × 10{str(int(e)).translate(SUPERSCRIPT)}'


def temp_factor(f):
    if abs(f - 1) < 1e-15:
        return ''
    if abs(f - 5 / 9) < 1e-13:      # the table is rounded to 14 digits
        return ' × 5/9'
    return f' × {number(f)}'


def function_index(roots):
    """Link each non-unit menu action to its operating instructions."""
    chapters = {
        'MATH': '10-mathematics.md#math-menu',
        'NUM': '10-mathematics.md#num-menu',
        'PROB': '10-mathematics.md#prob-menu',
        'COMPLEX': '18-compatibility.md#complex-number-placeholders',
        'TVM': '13-tvm.md',
        'BASE': '11-base.md',
        'HYPERBOLIC': '10-mathematics.md#hyp-menu',
        'STAT': '12-statistics.md',
        'CONV': '10-mathematics.md#conv-menu',
        'CLEAR': '10-mathematics.md#stk-and-clr-menus',
        'MODE': '09-settings.md',
        'STACK': '06-stack.md#moving-things-around',
        'UNIT': '14-units.md#ufcn',
        'DATE': '15-date.md#the-date-submenus',
        'EPOCH': '15-date.md#epochs',
        'CLK': '15-date.md#the-clock-and-time-zones',
        'CFIT': '12-statistics.md#curve-fitting',
        'HELP': '07-menus.md#help',
    }
    unavailable = {'NUM_RESULT_MODE_REAL', 'NUM_RESULT_MODE_COMPLEX',
                   'COMPLEX_MODE_RECT', 'COMPLEX_MODE_RAD', 'COMPLEX_MODE_DEG'}
    entries = {}

    def walk(g, path, destination=None):
        destination = chapters.get(g.ident, destination)
        for row in g.rows:
            for n in row:
                if isinstance(n, genmenu.Group):
                    walk(n, path + [label(n.label)], destination)
                elif n[0] != 'blank' and n[3] not in genunits.U:
                    ident = n[3]
                    target = chapters.get(ident, destination)
                    if ident in unavailable:
                        target = chapters['COMPLEX']
                    if not target:
                        sys.exit(f'guide index: no chapter for {ident}')
                    entry = entries.setdefault(ident, [label(n[1]), [], target])
                    entry[1].append(' → '.join(path))

    for r in roots:
        walk(r, [label(r.label)])
    lines = ['Find a menu action by its displayed label. Menu paths distinguish',
             'repeated labels such as N, CLTV, or LEAP. Unit selections are listed',
             'separately in [Appendix C](#appendix-c-units); keyboard operations',
             'are in [Appendix A](#appendix-a-key-reference).', '',
             '| Function | Menu location | Instructions |',
             '| --- | --- | --- |']
    for text, paths, target in sorted(entries.values(), key=lambda e: (e[0].casefold(), e[1])):
        lines.append(f'| {text} | {"; ".join(paths)} | [Read]({target}) |')
    lines.append('')
    return lines


def replace_block(text, name, new_lines):
    pat = re.compile(r'(<!-- BEGIN ' + name + r'[^>]*-->\n).*?(<!-- END ' + name + r' -->)', re.S)
    if not pat.search(text):
        sys.exit(f'{GUIDE}: no {name} markers')
    return pat.sub(lambda m: m.group(1) + '\n'.join(new_lines) + '\n' + m.group(2), text)


def main():
    roots = genmenu.parse()
    text = open(GUIDE, encoding='utf-8').read()
    text = replace_block(text, 'MENU MAP', menu_map(roots))
    text = replace_block(text, 'UNITS', unit_list(roots))
    text = replace_block(text, 'FUNCTION INDEX', function_index(roots))
    open(GUIDE, 'w', encoding='utf-8').write(text)
    print('19-reference.md menu map, units, and function index updated')


if __name__ == '__main__':
    main()
