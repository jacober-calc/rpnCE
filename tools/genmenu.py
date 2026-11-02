#!/usr/bin/env python3
"""Reads menus/menus.txt (and any files it includes) for tools/genres.py.

The command-line entry point delegates to tools/genres.py.
"""
import os
import re
import sys
from collections import namedtuple

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'menus', 'menus.txt')

MENU_ATTRIBUTION = (
    '/* Menu definitions adapted from RPN83P (https://github.com/bxparks/rpn83p).\n'
    ' * Copyright (c) 2023 Brian T. Park. MIT License; see LICENSE.\n'
    ' */\n'
)

# Symbol names -> character codes drawn by the UI (see menu_glyphs in display.c).
SYMBOLS = ['sq', 'cube', 'cuberoot', 'root', 'sigma', 'to', 'block', 'mu',
           'deg', 'up', 'down', 'left', 'times', 'supx']
SYM_CODE = {name: i + 1 for i, name in enumerate(SYMBOLS)}

Item = namedtuple('Item', 'kind label alt ident command arg', defaults=(None, '0'))


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
        elif kind == 'item' and 3 <= len(parts) <= 5:
            node = Item('item', parts[1], None, parts[2], *parts[3:])
        elif kind == 'alt' and 4 <= len(parts) <= 6:
            node = Item('alt', parts[1], parts[2], parts[3], *parts[4:])
        elif kind == 'blank' and len(parts) == 1:
            node = Item('blank', '', None, None)
        else:
            fail(lineno, f'cannot parse: {line.strip()}')

        if isinstance(node, Item) and node.command:
            if not re.fullmatch(r'CMD_[A-Z0-9_]+', node.command):
                fail(lineno, 'expected a CMD_* command after the menu ID')
            if not re.fullmatch(r'[A-Z][A-Z0-9_]*|0|[1-9][0-9]*', node.arg):
                fail(lineno, 'expected an enum name or decimal byte argument')
            if node.arg.isdecimal() and int(node.arg) > 255:
                fail(lineno, 'command argument exceeds one byte')

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


def c_label(text):
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


if __name__ == '__main__':
    # The menu tables are now built into the RPNCEres appvar by genres.py,
    # which uses this file as a library.
    import genres
    genres.main()
