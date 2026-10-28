#!/usr/bin/env python3
"""Reads menus/menus.txt (and any files it includes) for tools/genres.py.

Run from the project root:  python3 tools/genmenu.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'menus', 'menus.txt')
OUT_H = os.path.join(ROOT, 'src', 'ui', 'menu_data.h')
OUT_C = os.path.join(ROOT, 'src', 'ui', 'menu_data.c')

MENU_ATTRIBUTION = (
    '/* Menu definitions adapted from RPN83P (https://github.com/bxparks/rpn83p).\n'
    ' * Copyright (c) 2023 Brian T. Park. MIT License; see LICENSE.\n'
    ' */\n'
)

# Symbol names -> character codes drawn by the UI (see menu_glyphs in display.c).
SYMBOLS = ['sq', 'cube', 'cuberoot', 'root', 'sigma', 'to', 'block', 'mu',
           'deg', 'up', 'down', 'left', 'times', 'supx']
SYM_CODE = {name: i + 1 for i, name in enumerate(SYMBOLS)}


class Group:
    def __init__(self, label, ident):
        self.label, self.ident, self.rows = label, ident, []


def fail(where, msg):
    sys.exit(f'{where}: {msg}')


def read_lines(path, seen=()):
    """Yields (where, line) for a menu file, expanding top-level includes."""
    name = os.path.basename(path)
    if path in seen:
        sys.exit(f'{name}: include loop')
    for lineno, raw in enumerate(open(path, encoding='utf-8'), 1):
        stripped = raw.split('#', 1)[0].rstrip()
        parts = stripped.split()
        if parts and parts[0] == 'include':
            if len(parts) != 2 or raw.startswith(' '):
                fail(f'{name}:{lineno}', 'include FILE must be at the top level')
            inc = os.path.join(os.path.dirname(path), parts[1])
            yield from read_lines(inc, seen + (path,))
        else:
            yield f'{name}:{lineno}', raw


def parse():
    roots = []
    stack = []   # (indent, object)
    for lineno, raw in read_lines(SRC):
        line = raw.split('#', 1)[0].rstrip()
        if not line.strip():
            continue
        indent = len(line) - len(line.lstrip(' '))
        parts = line.split()
        while stack and stack[-1][0] >= indent:
            stack.pop()
        parent = stack[-1][1] if stack else None
        kind = parts[0]

        if kind == 'row':
            if not isinstance(parent, Group):
                fail(lineno, 'row must be inside a group')
            row = []
            parent.rows.append(row)
            stack.append((indent, row))
            continue

        if kind == 'group' and len(parts) == 3:
            node = Group(parts[1], parts[2])
        elif kind == 'item' and len(parts) == 3:
            node = ('item', parts[1], None, parts[2])
        elif kind == 'alt' and len(parts) == 4:
            node = ('alt', parts[1], parts[2], parts[3])
        elif kind == 'blank' and len(parts) == 1:
            node = ('blank', '', None, None)
        else:
            fail(lineno, f'cannot parse: {line.strip()}')

        if parent is None:
            if indent != 0 or not isinstance(node, Group):
                fail(lineno, 'only groups can be at the top level')
            roots.append(node)
        elif isinstance(parent, list):
            if len(parent) >= 5:
                fail(lineno, 'a row holds at most 5 entries')
            parent.append(node)
        else:
            fail(lineno, 'entries must be inside a row')

        if isinstance(node, Group):
            stack.append((indent, node))
    if not roots:
        sys.exit('menus.txt: no menus found')
    return roots


def c_label(text, lineinfo=''):
    def sym(m):
        name = m.group(1)
        if name not in SYM_CODE:
            sys.exit(f'menus.txt: unknown symbol {{{name}}} in {text}')
        return '\\x%02x' % SYM_CODE[name]
    out = text.replace('\\', '\\\\').replace('"', '\\"')
    out = re.sub(r'\{(\w+)\}', sym, out)
    # Keep a hex escape from swallowing following hex digits.
    out = re.sub(r'(\\x[0-9a-f]{2})(?=[0-9A-Fa-f])', r'\1""', out)
    return '"' + out + '"'


def main():
    roots = parse()

    groups = []          # top-level menus first, then submenus breadth-first
    order = list(roots)
    while order:
        g = order.pop(0)
        groups.append(g)
        for row in g.rows:
            for n in row:
                if isinstance(n, Group):
                    order.append(n)
    gindex = {id(g): i for i, g in enumerate(groups)}

    # An ID names a function, so it may appear in several menus.
    ids = ['NONE']
    seen = {'NONE'}
    for g in groups:
        for ident in [g.ident] + [n.ident if isinstance(n, Group) else n[3]
                                  for row in g.rows for n in row]:
            if ident and ident not in seen:
                seen.add(ident)
                ids.append(ident)

    nodes = []
    group_rows = []
    for g in groups:
        first = len(nodes)
        for row in g.rows:
            padded = row + [('blank', '', None, None)] * (5 - len(row))
            for n in padded:
                if isinstance(n, Group):
                    nodes.append(('MENU_GROUP', n.label, None, n.ident, gindex[id(n)]))
                else:
                    kind = {'item': 'MENU_ITEM', 'alt': 'MENU_ALT', 'blank': 'MENU_BLANK'}[n[0]]
                    nodes.append((kind, n[1], n[2], n[3], 0))
        group_rows.append((g.label, first, len(g.rows)))

    with open(OUT_H, 'w') as f:
        f.write('/* Generated by tools/genmenu.py from menus/menus.txt. Do not edit. */\n')
        f.write(MENU_ATTRIBUTION)
        f.write('#ifndef RPNCE_UI_MENU_DATA_H\n#define RPNCE_UI_MENU_DATA_H\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write('/* Symbol character codes used in labels. */\n')
        for name in SYMBOLS:
            f.write(f'#define MSYM_{name.upper()} 0x{SYM_CODE[name]:02x}\n')
        f.write(f'#define MSYM_COUNT {len(SYMBOLS)}\n\n')
        f.write('typedef enum {\n')
        for ident in ids:
            f.write(f'    MID_{ident},\n')
        f.write('    MID_COUNT\n} menu_id_t;\n\n')
        f.write('typedef enum { MENU_BLANK, MENU_ITEM, MENU_ALT, MENU_GROUP } menu_kind_t;\n\n')
        f.write('typedef struct {\n'
                '    const char *label;\n'
                '    const char *alt_label;   /* MENU_ALT only */\n'
                '    uint16_t id;             /* menu_id_t */\n'
                '    uint8_t kind;            /* menu_kind_t */\n'
                '    uint8_t group;           /* MENU_GROUP: index into menu_groups */\n'
                '} menu_node_t;\n\n')
        f.write('typedef struct {\n'
                '    const char *label;\n'
                '    uint16_t first;          /* index of first node in menu_nodes */\n'
                '    uint8_t rows;            /* each row is 5 nodes */\n'
                '} menu_group_t;\n\n')
        f.write('/* Top-level menus, opened by keys. */\n')
        for i, r in enumerate(roots):
            f.write(f'#define MENU_ROOT_{r.ident} {i}\n')
        f.write('\n/* Every menu, for jumping straight to one (e.g. MODE). */\n')
        for i, g in enumerate(groups):
            f.write(f'#define MENU_GROUP_{g.ident} {i}\n')
        f.write('\n')
        f.write(f'#define MENU_GROUP_COUNT {len(groups)}\n')
        f.write(f'#define MENU_NODE_COUNT {len(nodes)}\n\n')
        f.write('extern const menu_node_t menu_nodes[MENU_NODE_COUNT];\n')
        f.write('extern const menu_group_t menu_groups[MENU_GROUP_COUNT];\n\n')
        f.write('/* Where each menu sits: its parent menu and the row it appears on\n'
                ' * (0xFF for a top-level menu). */\n')
        f.write('extern const uint8_t menu_parent[MENU_GROUP_COUNT];\n')
        f.write('extern const uint8_t menu_parent_row[MENU_GROUP_COUNT];\n\n')
        f.write('#endif\n')

    with open(OUT_C, 'w') as f:
        f.write('/* Generated by tools/genmenu.py from menus/menus.txt. Do not edit. */\n')
        f.write(MENU_ATTRIBUTION)
        f.write('#include "menu_data.h"\n\n')
        f.write('const menu_node_t menu_nodes[MENU_NODE_COUNT] = {\n')
        for kind, label, alt, ident, grp in nodes:
            alt_c = c_label(alt) if alt else '0'
            f.write(f'    {{ {c_label(label)}, {alt_c}, MID_{ident or "NONE"}, {kind}, {grp} }},\n')
        f.write('};\n\n')
        f.write('const menu_group_t menu_groups[MENU_GROUP_COUNT] = {\n')
        for label, first, rows in group_rows:
            f.write(f'    {{ {c_label(label)}, {first}, {rows} }},\n')
        f.write('};\n\n')
        parent = [0xFF] * len(groups)
        prow = [0xFF] * len(groups)
        for gi, g in enumerate(groups):
            for ri, row in enumerate(g.rows):
                for n in row:
                    if isinstance(n, Group):
                        parent[gindex[id(n)]] = gi
                        prow[gindex[id(n)]] = ri
        f.write('const uint8_t menu_parent[MENU_GROUP_COUNT] = { '
                + ', '.join(str(v) for v in parent) + ' };\n')
        f.write('const uint8_t menu_parent_row[MENU_GROUP_COUNT] = { '
                + ', '.join(str(v) for v in prow) + ' };\n')

    print(f'{len(groups)} menus, {len(nodes)} slots, {len(ids) - 1} ids')


if __name__ == '__main__':
    # The menu tables are now built into the RPNCEres appvar by genres.py,
    # which uses this file as a library.
    import genres
    genres.main()
