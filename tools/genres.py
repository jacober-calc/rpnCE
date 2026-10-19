#!/usr/bin/env python3
"""Builds RPNCE's resources from menus/menus.txt, help/help.txt and the unit
definitions.

    python3 tools/genres.py

Writes:
  data/RPNCEres.bin      the RPNCEres appvar's contents (the makefile turns it
                         into bin/RPNCEres.8xv): menu tree, unit tables, HELP
                         pages, text
  src/ui/menu_data.h     menu IDs, counts, and the record layouts
  src/ui/res_info.h      the resource format and version stamp
  src/core/units_count.h unit and dimension counts
  tests/units_data.c     a C copy of the unit table, for the PC tests only

The program reads the appvar straight from archive, so none of these tables
use RAM. Needs the mpmath package (for the parsec).
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import genmenu   # noqa: E402  (menu file parser and symbol codes)
import genunits  # noqa: E402  (exact unit definitions)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_BIN = os.path.join(ROOT, 'data', 'RPNCEres.bin')
OUT_MENU_H = os.path.join(ROOT, 'src', 'ui', 'menu_data.h')
OUT_INFO_H = os.path.join(ROOT, 'src', 'ui', 'res_info.h')
OUT_COUNT_H = os.path.join(ROOT, 'src', 'core', 'units_count.h')
OUT_UNITS_C = os.path.join(ROOT, 'tests', 'units_data.c')

RES_FORMAT = 2      # bump when the layout below changes (2: HELP pages)
HELP_FILE = os.path.join(ROOT, 'help', 'help.txt')
HELP_MAX_LINES = 8      # a page's lines, title included
HELP_MAX_CHARS = 44     # fits the screen in the system font
# (still format 1: no RPNCEres with 5-byte groups was ever released)
NONE16 = 0xFFFF

# ---------------------------------------------------------------------------
# Appvar layout (little-endian, no padding):
#   header   "RPNR" u16 format, u16 node_count, u8 group_count, u8 unit_count,
#            u16 id_count, u32 hash (FNV-1a of everything after the header)
#   offsets  u16 x 9: nodes, groups, parents, parent_rows, units, unit_base,
#            unit_map, help, text (from the start of the appvar)
#   nodes    8 bytes each: u16 label, u16 alt_label, u16 id, u8 kind, u8 group
#   groups   6 bytes each: u16 label, u16 first node, u8 rows, u8 spare
#   parents  u8 per group (0xFF for a top-level menu), then parent rows
#   units    8 bytes each: u16 label, u8 dim, u8 reciprocal, u16 factor,
#            u16 offset (0xFFFF for none)
#   unit_base u8 per dimension; unit_map u8 per menu ID (0xFF: not a unit)
#   help     u8 page count; then per page u8 line count and u16 per line
#   text     NUL-terminated strings; the u16 fields above are offsets here
# ---------------------------------------------------------------------------
HEADER_FMT = '<4sHHBBHI'
OFFSETS_FMT = '<9H'


def label_bytes(text):
    """A menu label with {symbol} names turned into their character codes."""
    def sym(m):
        name = m.group(1)
        if name not in genmenu.SYM_CODE:
            sys.exit(f'menus.txt: unknown symbol {{{name}}} in {text}')
        return chr(genmenu.SYM_CODE[name])
    return re.sub(r'\{(\w+)\}', sym, text).encode('latin-1')


class Text:
    """The text section: each distinct string stored once."""
    def __init__(self):
        self.data = bytearray()
        self.index = {}

    def add(self, b):
        if b not in self.index:
            self.index[b] = len(self.data)
            self.data += b + b'\0'
        return self.index[b]


def fnv1a(data):
    h = 2166136261
    for byte in data:
        h ^= byte
        h = (h * 16777619) & 0xFFFFFFFF
    return h


def build_menus():
    roots = genmenu.parse()
    groups, order = [], list(roots)
    while order:                    # top-level menus first, then breadth-first
        g = order.pop(0)
        groups.append(g)
        order.extend(n for row in g.rows for n in row if isinstance(n, genmenu.Group))
    gindex = {id(g): i for i, g in enumerate(groups)}

    ids, seen = ['NONE'], {'NONE'}
    for g in groups:
        for ident in [g.ident] + [n.ident if isinstance(n, genmenu.Group) else n[3]
                                  for row in g.rows for n in row]:
            if ident and ident not in seen:
                seen.add(ident)
                ids.append(ident)

    nodes, group_rows = [], []
    for g in groups:
        first = len(nodes)
        for row in g.rows:
            for n in row + [('blank', '', None, None)] * (5 - len(row)):
                if isinstance(n, genmenu.Group):
                    nodes.append(('MENU_GROUP', n.label, None, n.ident, gindex[id(n)]))
                else:
                    kind = {'item': 'MENU_ITEM', 'alt': 'MENU_ALT', 'blank': 'MENU_BLANK'}[n[0]]
                    nodes.append((kind, n[1], n[2], n[3], 0))
        group_rows.append((g.label, first, len(g.rows)))

    parent = [0xFF] * len(groups)
    prow = [0xFF] * len(groups)
    for gi, g in enumerate(groups):
        for ri, row in enumerate(g.rows):
            for n in row:
                if isinstance(n, genmenu.Group):
                    parent[gindex[id(n)]] = gi
                    prow[gindex[id(n)]] = ri
    if len(groups) > 255 or len(nodes) > 0xFFFF:
        sys.exit('menus.txt: too many menus')
    return roots, groups, ids, nodes, group_rows, parent, prow


def build_units():
    """Units in menu order: (id, label, dim, factor, offset, reciprocal)."""
    labels = {}
    stack = [genmenu.parse()[0]]
    while stack:
        grp = stack.pop()
        for row in grp.rows:
            for n in row:
                if isinstance(n, genmenu.Group):
                    stack.append(n)
                elif n[0] == 'item' and n[3] and n[3].startswith('UNIT_'):
                    labels.setdefault(n[3], n[1])
    U, DIMS = genunits.U, genunits.DIMS
    missing = sorted(set(labels) - set(U) - {'UNIT_VALUE', 'UNIT_BASE'})
    extra = sorted(set(U) - set(labels))
    if missing or extra:
        sys.exit(f'menus/units mismatch: missing {missing}, extra {extra}')
    units = []
    for i in [i for i in labels if i in U]:
        d = U[i]
        off = d[2] if len(d) > 2 else 0
        units.append((i, labels[i], d[0], genunits.d14(d[1]),
                      genunits.d14(off) if off else None, len(d) > 3))
    dims = list(DIMS)
    ids = [u[0] for u in units]
    base = [ids.index(DIMS[dname]) for dname in dims]
    return units, dims, base


def build_help():
    """HELP pages from help/help.txt: a list of pages, each a list of lines."""
    pages, page = [], []
    with open(HELP_FILE, encoding='utf-8') as f:
        for n, raw in enumerate(f, 1):
            line = raw.rstrip('\n')
            if line.startswith('#'):
                continue
            if line.strip() == '---':
                pages.append(page)
                page = []
                continue
            if not page and not line.strip():
                continue                    # blank lines before a title
            if len(line) > HELP_MAX_CHARS:
                sys.exit(f'help.txt:{n}: longer than {HELP_MAX_CHARS} characters')
            try:
                line.encode('ascii')
            except UnicodeEncodeError:
                sys.exit(f'help.txt:{n}: use plain ASCII characters')
            page.append(line)
    pages.append(page)
    pages = [[l for l in p] for p in pages]
    for i, p in enumerate(pages, 1):
        while p and not p[-1].strip():
            p.pop()
        if not p:
            sys.exit(f'help.txt: page {i} is empty')
        if len(p) > HELP_MAX_LINES:
            sys.exit(f'help.txt: page {i} has {len(p)} lines (at most {HELP_MAX_LINES})')
    if len(pages) > 255:
        sys.exit('help.txt: too many pages')
    return pages


def main():
    roots, groups, ids, nodes, group_rows, parent, prow = build_menus()
    help_pages = build_help()
    units, dims, base = build_units()
    id_num = {ident: i for i, ident in enumerate(ids)}
    text = Text()

    node_bytes = bytearray()
    for kind, label, alt, ident, grp in nodes:
        kinds = {'MENU_BLANK': 0, 'MENU_ITEM': 1, 'MENU_ALT': 2, 'MENU_GROUP': 3}
        node_bytes += struct.pack('<HHHBB', text.add(label_bytes(label)),
                                  text.add(label_bytes(alt)) if alt else NONE16,
                                  id_num[ident or 'NONE'], kinds[kind], grp)
    group_bytes = bytearray()
    for label, first, rows in group_rows:
        group_bytes += struct.pack('<HHBx', text.add(label_bytes(label)), first, rows)
    unit_bytes = bytearray()
    for ident, label, dim, factor, offset, recip in units:
        unit_bytes += struct.pack('<HBBHH', text.add(label_bytes(label)), dims.index(dim),
                                  1 if recip else 0, text.add(factor.encode()),
                                  text.add(offset.encode()) if offset else NONE16)
    unit_index = {u[0]: k for k, u in enumerate(units)}
    unit_map = bytes(unit_index.get(ident, 0xFF) for ident in ids)
    help_bytes = bytearray([len(help_pages)])
    for page in help_pages:
        help_bytes.append(len(page))
        for line in page:
            help_bytes += struct.pack('<H', text.add(line.encode('ascii')))

    sections = [node_bytes, group_bytes, bytes(parent), bytes(prow), unit_bytes,
                bytes(base), unit_map, bytes(help_bytes), bytes(text.data)]
    start = struct.calcsize(HEADER_FMT) + struct.calcsize(OFFSETS_FMT)
    offsets, pos = [], start
    for sec in sections:
        offsets.append(pos)
        pos += len(sec)
    if pos > 0xFFFF:
        sys.exit('resources are larger than an appvar can hold')
    body = struct.pack(OFFSETS_FMT, *offsets) + b''.join(sections)
    stamp = fnv1a(body)
    header = struct.pack(HEADER_FMT, b'RPNR', RES_FORMAT, len(nodes), len(groups),
                         len(units), len(ids), stamp)
    os.makedirs(os.path.dirname(OUT_BIN), exist_ok=True)
    with open(OUT_BIN, 'wb') as f:
        f.write(header + body)

    # ---- src/ui/menu_data.h ----
    with open(OUT_MENU_H, 'w') as f:
        f.write('/* Generated by tools/genres.py from menus/menus.txt. Do not edit. */\n')
        f.write('#ifndef RPNCE_UI_MENU_DATA_H\n#define RPNCE_UI_MENU_DATA_H\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write('/* Symbol character codes used in labels. */\n')
        for name in genmenu.SYMBOLS:
            f.write(f'#define MSYM_{name.upper()} 0x{genmenu.SYM_CODE[name]:02x}\n')
        f.write(f'#define MSYM_COUNT {len(genmenu.SYMBOLS)}\n\n')
        f.write('typedef enum {\n')
        for ident in ids:
            f.write(f'    MID_{ident},\n')
        f.write('    MID_COUNT\n} menu_id_t;\n\n')
        f.write('typedef enum { MENU_BLANK, MENU_ITEM, MENU_ALT, MENU_GROUP } menu_kind_t;\n\n')
        f.write('/* Records as stored in the RPNCEres appvar (labels are offsets into its\n'
                ' * text; see res.h for reading them). */\n')
        f.write('typedef struct {\n'
                '    uint16_t label;\n'
                '    uint16_t alt_label;      /* MENU_ALT only; 0xFFFF otherwise */\n'
                '    uint16_t id;             /* menu_id_t */\n'
                '    uint8_t kind;            /* menu_kind_t */\n'
                '    uint8_t group;           /* MENU_GROUP: the submenu */\n'
                '} menu_node_t;\n\n')
        f.write('typedef struct {\n'
                '    uint16_t label;\n'
                '    uint16_t first;          /* first node; each row is 5 nodes */\n'
                '    uint8_t rows;\n'
                '    uint8_t spare;           /* keeps the record 6 bytes on any compiler */\n'
                '} menu_group_t;\n\n')
        f.write('/* Top-level menus, opened by keys. */\n')
        for i, r in enumerate(roots):
            f.write(f'#define MENU_ROOT_{r.ident} {i}\n')
        f.write('\n/* Every menu, for jumping straight to one (e.g. MODE). */\n')
        for i, g in enumerate(groups):
            f.write(f'#define MENU_GROUP_{g.ident} {i}\n')
        f.write(f'\n#define MENU_GROUP_COUNT {len(groups)}\n')
        f.write(f'#define MENU_NODE_COUNT {len(nodes)}\n\n#endif\n')

    # ---- src/ui/res_info.h ----
    with open(OUT_INFO_H, 'w') as f:
        f.write('/* Generated by tools/genres.py. Do not edit. */\n')
        f.write('#ifndef RPNCE_UI_RES_INFO_H\n#define RPNCE_UI_RES_INFO_H\n\n')
        f.write('/* The RPNCEres appvar this build of the program expects. */\n')
        f.write('#define RES_APPVAR "RPNCEres"\n')
        f.write(f'#define RES_FORMAT {RES_FORMAT}\n')
        f.write(f'#define RES_STAMP 0x{stamp:08X}UL\n')
        f.write(f'#define RES_SIZE {len(header) + len(body)}\n\n#endif\n')

    # ---- src/core/units_count.h ----
    with open(OUT_COUNT_H, 'w') as f:
        f.write('/* Generated by tools/genres.py. Do not edit. */\n')
        f.write(f'#define UNIT_COUNT {len(units)}\n')
        f.write('enum { ' + ', '.join(f'DIM_{d}' for d in dims) + ', DIM_COUNT };\n')

    # ---- tests/units_data.c (PC tests only) ----
    with open(OUT_UNITS_C, 'w') as f:
        f.write('/* Generated by tools/genres.py. Do not edit.\n'
                ' * The unit table as C, for the PC tests. On the calculator the same\n'
                ' * table is read from the RPNCEres appvar (src/ui/res.c). */\n')
        f.write('#include <stddef.h>\n#include "units.h"\n\n')
        f.write('static const struct { const char *label; uint8_t dim; const char *factor;\n'
                '                      const char *offset; uint8_t recip; } t[UNIT_COUNT] = {\n')
        for ident, label, dim, factor, offset, recip in units:
            off = f'"{offset}"' if offset else 'NULL'
            f.write(f'    {{ {genmenu.c_label(label)}, DIM_{dim}, "{factor}", {off}, '
                    f'{1 if recip else 0} }},  /* {ident} */\n')
        f.write('};\n')
        f.write('static const uint8_t base[DIM_COUNT] = { '
                + ', '.join(str(b) for b in base) + ' };\n\n')
        f.write('const char *unit_label(uint8_t u) { return t[u].label; }\n')
        f.write('uint8_t unit_dim(uint8_t u) { return t[u].dim; }\n')
        f.write('const char *unit_factor(uint8_t u) { return t[u].factor; }\n')
        f.write('const char *unit_offset(uint8_t u) { return t[u].offset; }\n')
        f.write('bool unit_reciprocal(uint8_t u) { return t[u].recip != 0; }\n')
        f.write('uint8_t unit_base_of(uint8_t dim) { return base[dim]; }\n')

    print(f'{len(groups)} menus, {len(nodes)} slots, {len(ids) - 1} ids, '
          f'{len(units)} units, {len(help_pages)} help pages; '
          f'RPNCEres {len(header) + len(body)} bytes, '
          f'stamp {stamp:08X}')


if __name__ == '__main__':
    main()
