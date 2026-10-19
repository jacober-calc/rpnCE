#!/usr/bin/env python3
"""Fills in the generated appendices of docs/USER_GUIDE.md:

  Appendix B (the menu map), from menus/menus.txt
  Appendix C (the units), from menus/menus.txt and tools/genunits.py

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
GUIDE = os.path.join(ROOT, 'docs', 'USER_GUIDE.md')

# menu label symbols, as shown on the calculator
SYMBOLS = {'sq': '²', 'cube': '³', 'cuberoot': '³√', 'root': '√', 'sigma': 'Σ',
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

    def walk(g, path):
        lines.append(f'**{" > ".join(path)}**')
        lines.append('')
        lines.append('| Row | Keys |')
        lines.append('| --- | --- |')
        for i, row in enumerate(g.rows, 1):
            cells = [node_label(n) for n in row] + ['·'] * (5 - len(row))
            lines.append(f'| {i} | ' + ' &nbsp; '.join(cells) + ' |')
        lines.append('')
        for row in g.rows:
            for n in row:
                if isinstance(n, genmenu.Group):
                    walk(n, path + [label(n.label)])

    for r in roots:
        walk(r, [label(r.label)])
    return lines


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

    lines = ['Each unit with its value in the base unit of its kind (the unit UBAS',
             'converts to). Values are exact to the digits shown, from each',
             "unit's definition.", '']
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


def replace_block(text, name, new_lines):
    pat = re.compile(r'(<!-- BEGIN ' + name + r'[^>]*-->\n).*?(<!-- END ' + name + r' -->)', re.S)
    if not pat.search(text):
        sys.exit(f'USER_GUIDE.md: no {name} markers')
    return pat.sub(lambda m: m.group(1) + '\n'.join(new_lines) + '\n' + m.group(2), text)


def main():
    roots = genmenu.parse()
    text = open(GUIDE, encoding='utf-8').read()
    text = replace_block(text, 'MENU MAP', menu_map(roots))
    text = replace_block(text, 'UNITS', unit_list(roots))
    open(GUIDE, 'w', encoding='utf-8').write(text)
    print('USER_GUIDE.md appendices updated')


if __name__ == '__main__':
    main()
