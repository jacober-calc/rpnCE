#!/usr/bin/env python3
"""Builds the user guide's screenshots by running rpnCE's real code on a PC.

    python3 tools/screenshots/run.py            (from the project root)

The driver (driver.c) runs src/main.c, the display and the core with a
software version of graphx (graphx_raster.c), keys from a script, and the
CE's system font taken from the CE toolchain's graphx.asm (downloaded once
into tools/screenshots/.cache). Each sequence in shots.py starts from a
fresh calculator, and the images go to assets/guide/:

  screens/NAME.png   full screens, for examples
  menu/NAME.png      menu rows, as in RPN83P's guide: NAME-1, NAME-2, ...
                     for each row, and NAME.png for the parent menu's row
                     with the item that opens this menu outlined in red

Needs a C compiler and the Pillow package.
"""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import genmenu  # noqa: E402

from PIL import Image, ImageDraw  # noqa: E402

CACHE = os.path.join(HERE, '.cache')
GRAPHX_URL = ('https://raw.githubusercontent.com/CE-Programming/toolchain/'
              'v15.0/src/graphx/graphx.asm')
OUT = os.path.join(ROOT, 'assets', 'guide')
DRIVER = os.path.join(CACHE, 'driver')

SOURCES = ['src/ui/display.c', 'src/ui/dotfont.c', 'src/ui/keys.c', 'src/ui/menu.c',
           'src/ui/actions.c', 'src/ui/catalog.c', 'src/ui/res.c',
           'src/core/calc.c', 'src/core/calc_entry.c', 'src/core/calc_date.c',
           'src/core/calc_quantity.c', 'src/core/base.c', 'src/core/datetime.c',
           'src/core/mathx.c', 'src/core/stats.c', 'src/core/tvm.c', 'src/core/units.c',
           'src/core/format.c']

# ---------- building the driver ----------


def font_header():
    asm = os.path.join(CACHE, 'graphx-v15.0.asm')
    if not os.path.exists(asm):
        os.makedirs(CACHE, exist_ok=True)
        print('downloading graphx.asm (CE toolchain v15.0) for the system font')
        urllib.request.urlretrieve(GRAPHX_URL, asm)
    s = open(asm).read()
    a = s.index('_DefaultTextData:')
    rows = re.findall(r'db\s+' + r',\s*'.join([r'\$([0-9A-Fa-f]{2})'] * 8), s[a:a + 40000])[:128]
    sp = [int(v) for line in re.findall(r'db\s+([0-9,]+)', s[s.index('_DefaultCharSpacing:'):])[:8]
          for v in line.split(',')]
    if len(rows) != 128 or len(sp) != 128:
        sys.exit('graphx.asm: font tables not found')
    with open(os.path.join(CACHE, 'font.h'), 'w') as f:
        f.write('/* generated from graphx.asm (CE toolchain v15.0) by run.py */\n#include <stdint.h>\n')
        f.write('static const uint8_t font_data[128][8] = {\n')
        f.write(',\n'.join('{' + ','.join('0x' + b for b in r) + '}' for r in rows) + '};\n')
        f.write('static const uint8_t font_spacing[128] = {' + ','.join(map(str, sp)) + '};\n')


def build():
    font_header()
    inc = ['-I' + os.path.join(HERE, 'hostinc'), '-I' + CACHE, '-Itests/mock',
           '-Itests/mock_fileioc', '-Isrc/core', '-Isrc/ui']
    flags = ['-O1', '-w', '-DRPNCE_HOST_TEST']
    main_o = os.path.join(CACHE, 'main.o')
    subprocess.run(['cc'] + flags + ['-Dmain=rpnce_main'] + inc +
                   ['-c', 'src/main.c', '-o', main_o], check=True, cwd=ROOT)
    subprocess.run(['cc'] + flags + inc + ['-o', DRIVER,
                    os.path.join(HERE, 'driver.c'), os.path.join(HERE, 'graphx_raster.c'),
                    main_o] + SOURCES + ['-lm'], check=True, cwd=ROOT)

# ---------- menus ----------


class Menus:
    """The menu tree from menus.txt, to find where a label is."""

    def __init__(self):
        self.roots = genmenu.parse()

    @staticmethod
    def plain(label):
        return re.sub(r'\{block\}', '', label)

    def find_group(self, path):
        """['HOME', 'UNIT', 'LENG'] -> the Group."""
        g = self.roots[0]
        assert g.label == path[0], path
        for name in path[1:]:
            g = next(n for row in g.rows for n in row
                     if isinstance(n, genmenu.Group) and self.plain(n.label) == name)
        return g

    @staticmethod
    def slot_of(group, label):
        """(row, slot, node) of a label in a group, matching the plain label or
        either label of a mode item."""
        for r, row in enumerate(group.rows):
            for s, n in enumerate(row):
                names = []
                if isinstance(n, genmenu.Group):
                    names = [n.label]
                elif n[0] != 'blank':
                    names = [n[1], n[2] or '']
                if label in [Menus.plain(x) for x in names]:
                    return r, s, n
        raise KeyError(label)


SOFTKEYS = ['sk_Yequ', 'sk_Window', 'sk_Zoom', 'sk_Trace', 'sk_Graph']

KEYS = {
    'ENTER': 'sk_Enter', '+': 'sk_Add', '-': 'sk_Sub', '×': 'sk_Mul', '*': 'sk_Mul',
    '÷': 'sk_Div', '/': 'sk_Div', '^': 'sk_Power', 'CLEAR': 'sk_Clear', '(-)': 'sk_Chs',
    'CHS': 'sk_Chs', '.': 'sk_DecPnt', ',': 'sk_Comma', '(': 'sk_LParen', ')': 'sk_RParen',
    'SIN': 'sk_Sin', 'COS': 'sk_Cos', 'TAN': 'sk_Tan', 'LN': 'sk_Ln', 'LOG': 'sk_Log',
    'x²': 'sk_Square', 'x⁻¹': 'sk_Recip', 'STO': 'sk_Store', 'STO→': 'sk_Store',
    'VARS': 'sk_Vars', 'DEL': 'sk_Del', 'ALPHA': 'sk_Alpha', '2ND': 'sk_2nd',
    'MATH': 'sk_Math', 'MODE': 'sk_Mode', 'STAT': 'sk_Stat', 'APPS': 'sk_Apps',
    'PRGM': 'sk_Prgm', 'XTθn': 'sk_GraphVar', 'UP': 'sk_Up', 'DOWN': 'sk_Down',
    'LEFT': 'sk_Left', 'RIGHT': 'sk_Right', 'ON': 'ON',
    'F1': 'sk_Yequ', 'F2': 'sk_Window', 'F3': 'sk_Zoom', 'F4': 'sk_Trace', 'F5': 'sk_Graph',
}
SHORTCUTS = {'MATH': ['HOME'], 'MODE': ['HOME', 'MODE'], 'STAT': ['HOME', 'STAT'],
             'XTθn': ['HOME', 'UNIT'], 'APPS': ['HOME', 'BASE'], 'PRGM': ['HOME', 'HYP']}


def compile_script(text, menus):
    """Friendly keystrokes -> driver tokens.

    Tokens: numbers like 12.5 (typed digit by digit), key names (KEYS),
    [LABEL] (the menu key under LABEL in the current menu, moving to its
    row first), >NAME (snapshot), !NAME (the prompt flash), and
    @HOME>UNIT>LENG (go to that menu, row 1)."""
    out = []
    path, row = ['HOME'], 0
    second = alpha = False
    prompt = False      # inside a STO / RCL / X<> register prompt
    for t in text.split():
        if t.startswith('>'):
            out += ['SNAP', t[1:]]
            continue
        if t.startswith('!'):
            out += ['FLASH', t[1:]]
            continue
        if t.startswith('@'):
            target = t[1:].split('>')
            out.append('sk_Math')
            path, row = ['HOME'], 0
            for name in target[1:]:
                g = menus.find_group(path)
                r, s, _ = menus.slot_of(g, name)
                out += ['sk_Down'] * r
                out.append(SOFTKEYS[s])
                path, row = path + [name], 0
            continue
        if prompt and not t.startswith('['):
            # a register prompt takes the key as a letter or an operator;
            # it never changes the menu
            if t not in KEYS:
                sys.exit(f'shots: unknown key {t!r}')
            out.append(KEYS[t])
            if t not in ('+', '-', '×', '*', '÷', '/', 'ALPHA', '2ND'):
                prompt = False
            continue
        if t.startswith('[') and t.endswith(']') and len(t) > 2:
            g = menus.find_group(path)
            r, s, n = menus.slot_of(g, t[1:-1])
            if t[1:-1] == 'X<>':
                prompt = True
            nrows = len(g.rows)
            while row != r:                 # rows wrap around
                out.append('sk_Down')
                row = (row + 1) % nrows
            out.append(SOFTKEYS[s])
            if isinstance(n, genmenu.Group) and not second:
                path, row = path + [Menus.plain(n.label)], 0
            second = False
            continue
        if re.fullmatch(r'\d+(\.\d*)?|\.\d+', t):
            for ch in t:
                out.append('sk_DecPnt' if ch == '.' else f'sk_{ch}')
            second = alpha = False
            continue
        if t not in KEYS:
            sys.exit(f'shots: unknown key {t!r}')
        out.append(KEYS[t])
        if t in ('STO', 'STO→'):            # STO, or with 2ND, RCL
            prompt = True
            second = alpha = False
            continue
        if t == '2ND':
            second = not second
            continue
        if t == 'ALPHA':
            alpha = not alpha
            continue
        if not second and not alpha:
            if t in SHORTCUTS:
                path, row = list(SHORTCUTS[t]), 0
            elif t == 'ON' and len(path) > 1:
                parent = menus.find_group(path[:-1])
                row, _, _ = menus.slot_of(parent, path[-1])
                path = path[:-1]
            elif t == 'UP':
                row = (row - 1) % len(menus.find_group(path).rows)
            elif t == 'DOWN':
                row = (row + 1) % len(menus.find_group(path).rows)
        second = alpha = False
    return ' '.join(out)

# ---------- images ----------

SCALE = 2           # images are saved at 2x, pixel for pixel, so they stay
                    # sharp on high-resolution screens; the guide shows them
                    # at their natural size (width 320 for a full screen)
MENU_Y, MENU_H = 213, 24
BOX_W, GAP = (320 - 18) // 5, 3


def menu_strip(img, highlight=None):
    """The menu row of a screen; highlight a slot with a red outline."""
    strip = img.crop((0, MENU_Y - 2, 320, MENU_Y + MENU_H + 2)).convert('RGB')
    if highlight is not None:
        d = ImageDraw.Draw(strip)
        x = GAP + highlight * (BOX_W + GAP)
        d.rectangle([x - 2, 0, x + BOX_W + 1, MENU_H + 3], outline=(220, 30, 30), width=2)
    return strip


def save(img, path):
    """Save at SCALE, pixel for pixel, as a compact palette PNG."""
    big = img.resize((img.width * SCALE, img.height * SCALE), Image.NEAREST)
    big.quantize(colors=64, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE).save(path, optimize=True)


def run(script, tmp, no_res=False):
    path = os.path.join(tmp, 'script.txt')
    with open(path, 'w') as f:
        f.write(script)
    env = dict(os.environ, SHOT_NO_RES='1') if no_res else None
    r = subprocess.run([DRIVER, path, tmp], cwd=ROOT, env=env, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(f'screenshot script failed: {r.stderr.strip()}\n  script: {script}')


def file_name(path):
    """['HOME', 'STAT', '{sigma}'] -> 'home-stat-sigma': plain words for links."""
    return '-'.join(re.sub(r'[^a-z0-9]+', '', re.sub(r'\{(\w+)\}', r'\1', p.lower())) for p in path)


def menu_images(menus, paths, tmp):
    os.makedirs(os.path.join(OUT, 'menu'), exist_ok=True)
    for path in paths:
        g = menus.find_group(path)
        name = file_name(path)
        keys = [f'@{">".join(path)}']
        for r in range(len(g.rows)):
            keys += [f'>row{r + 1}'] + (['DOWN'] if r + 1 < len(g.rows) else [])
        if len(path) > 1:
            keys += ['ON', '>parent']
        run(compile_script(' '.join(keys), menus), tmp)
        for r in range(len(g.rows)):
            img = Image.open(os.path.join(tmp, f'row{r + 1}.ppm'))
            save(menu_strip(img), os.path.join(OUT, 'menu', f'{name}-{r + 1}.png'))
        if len(path) > 1:
            parent = menus.find_group(path[:-1])
            _, slot, _ = menus.slot_of(parent, path[-1])
            img = Image.open(os.path.join(tmp, 'parent.ppm'))
            save(menu_strip(img, slot), os.path.join(OUT, 'menu', f'{name}.png'))


def screen_images(menus, sequences, tmp):
    os.makedirs(os.path.join(OUT, 'screens'), exist_ok=True)
    for seq in sequences:
        print('  ' + seq[:70])
        no_res = seq.startswith('NO_RES ')
        if no_res:
            seq = seq[len('NO_RES '):]
        run(compile_script(seq, menus), tmp, no_res)
        for name in re.findall(r'(?:^|\s)[>!](\S+)', seq):
            img = Image.open(os.path.join(tmp, name + '.ppm')).convert('RGB')
            save(img, os.path.join(OUT, 'screens', name + '.png'))


def referenced_images():
    root = Path(ROOT)
    assets = (root / 'assets').resolve()
    images = set()
    for page in [root / 'README.md', *(root / 'docs').rglob('*.md')]:
        text = page.read_text()
        targets = re.findall(r'!?\[[^\]]*\]\(([^\s)]+)(?:\s+"[^"]*")?\)', text)
        targets += re.findall(r'<img\s[^>]*src="([^"]+)"', text)
        for target in targets:
            if '://' in target or target.startswith(('#', 'mailto:')):
                continue
            path = (page.parent / target.split('#', 1)[0]).resolve()
            if assets in path.parents:
                images.add(path)
    return images


def main():
    global OUT
    os.chdir(ROOT)
    build()
    sys.path.insert(0, HERE)
    import shots
    menus = Menus()
    published = OUT
    wanted = referenced_images()
    with tempfile.TemporaryDirectory() as tmp:
        OUT = os.path.join(tmp, 'images')
        menu_images(menus, shots.MENUS, tmp)
        screen_images(menus, shots.SEQUENCES, tmp)
        shots.compose(os.path.join(OUT, 'screens'))
        for directory, _, names in os.walk(OUT):
            for name in names:
                source = os.path.join(directory, name)
                target = Path(published, os.path.relpath(source, OUT))
                if target.resolve() in wanted:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(source, target)
    OUT = published
    print('Screenshots referenced by the documentation written to assets/guide/.')


if __name__ == '__main__':
    main()
