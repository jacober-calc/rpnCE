#!/usr/bin/env python3
"""Prepare cem/ and the README animation from the current rpnCE build.

Run from an environment with the CE toolchain enabled:
    python3 tools/package_release.py

Screens come from the same real-code renderer as the guide. The arithmetic
demo is one continuous key sequence; captions sit outside the calculator.
Existing guide screens supply the stack, TVM and unit previews. Capture
scripts and intermediate files stay in obj/release-VERSION/.
"""
import importlib.util
import re
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, PngImagePlugin

ROOT = Path(__file__).resolve().parents[1]
VERSION = re.search(r'#define RPNCE_VERSION "([^"]+)"',
                    (ROOT / 'src/core/version.h').read_text()).group(1)
CEM = ROOT / 'cem'
WORK = ROOT / 'obj' / f'release-{VERSION}'
BAR = (82, 85, 82)
INK = (24, 24, 24)
WIDTH, TOP, BOTTOM = 320, 26, 44
HEIGHT = TOP + 240 + BOTTOM

# One continuous calculation: (3 + 4i + 2)^2 = 9 + 40i, magnitude 41.
STEPS = [
    ('3 2ND . 4', 'entry', '3  [2ND .]  4', 'Enter 3 + 4i as one value.', 1800),
    ('ENTER', 'entered', 'ENTER', 'Both components occupy one stack level.', 1600),
    ('2 +', 'add', '2  +', '(3 + 4i) + 2 = 5 + 4i', 2200),
    ('x²', 'square', 'x^2', '(5 + 4i)^2 = 9 + 40i', 2600),
    ('STO MATH', 'stored', 'STO A', 'Store the complete complex value in A.', 1800),
    ('CLEAR', 'cleared', 'CLEAR', 'Clear X before recalling the stored value.', 1100),
    ('2ND STO MATH', 'recalled', 'RCL A', 'Recall both components: 9 + 40i.', 2200),
    ('MODE [PRAD] MATH @HOME>CPLX', 'prad', 'MODE  >  PRAD',
     'The same value in polar radians.', 2400),
    ('MODE [PDEG] MATH @HOME>CPLX', 'pdeg', 'MODE  >  PDEG',
     'The same value in polar degrees.', 2400),
    ('MODE [RECT] MATH @HOME>CPLX [CABS]', 'magnitude', 'RECT  >  CPLX  >  CABS',
     '|9 + 40i| = 41', 2400),
]


def caption(screen, keys, explanation):
    frame = Image.new('RGB', (WIDTH, HEIGHT), 'white')
    frame.paste(screen, (0, TOP))
    draw = ImageDraw.Draw(frame)
    draw.rectangle((0, 0, WIDTH, TOP - 1), fill=BAR)
    draw.text((8, 5), f'rpnCE {VERSION}  |  Complex numbers',
              font=ImageFont.load_default(size=13), fill='white')
    draw.text((8, TOP + 244), keys, font=ImageFont.load_default(size=13), fill=INK)
    draw.text((8, TOP + 263), explanation,
              font=ImageFont.load_default(size=12), fill=INK)
    return frame


def upgrade_card():
    screen = Image.new('RGB', (WIDTH, 240), 'white')
    draw = ImageDraw.Draw(screen)
    title = ImageFont.load_default(size=19)
    body = ImageFont.load_default(size=14)
    draw.text((12, 14), 'New value storage', font=title, fill=INK)
    for y, text in [(49, 'Real, complex, DATE and unit values'),
                    (70, 'stay complete in stacks and registers.'),
                    (108, 'SAVED DATA IS INCOMPATIBLE'),
                    (134, '0.7.0 and earlier saved stacks and'),
                    (155, 'private registers cannot be imported.'),
                    (191, 'Record needed values before upgrading.'),
                    (212, f'Re-enter them in rpnCE {VERSION}.')]:
        draw.text((12, y), text, font=body, fill=INK)
    return caption(screen, 'Read readme.txt before upgrading.',
                   'Display/DATE settings and TI-OS TVM values carry over.')


def save_png(screen, target, description):
    info = PngImagePlugin.PngInfo()
    info.add_text('Title', f'rpnCE {VERSION}')
    info.add_text('Description', description)
    screen.quantize(colors=64, dither=Image.Dither.NONE).save(
        target, optimize=True, pnginfo=info)


def render_demo():
    spec = importlib.util.spec_from_file_location('release_screens',
                                                 ROOT / 'tools/screenshots/run.py')
    renderer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(renderer)
    renderer.build(extra_flags=['-save-temps=obj'])
    menus = renderer.Menus()
    captures = WORK / 'captures'
    captures.mkdir(parents=True, exist_ok=True)
    # TIOS is the ninth preset. Menu lookup follows the actual menu tree.
    # Three decimal places keep the polar separator and angle comfortably visible.
    setup = 'MODE [FIX] 3 [DEG] MATH MODE DOWN DOWN ' + '[TINT] ' * 8
    sequence = setup + ' MATH @HOME>CPLX '
    sequence += ' '.join(f'{keys} >{name}' for keys, name, *_ in STEPS)
    (WORK / 'calculation.txt').write_text(sequence + '\n')
    renderer.run(renderer.compile_script(sequence, menus), str(captures))
    screens = {name: Image.open(captures / f'{name}.ppm').convert('RGB')
               for _, name, *_ in STEPS}
    frames = [caption(screens[name], keys, explanation)
              for _, name, keys, explanation, _ in STEPS]
    frames.append(upgrade_card())
    # A shared palette keeps the LCD colours stable throughout the GIF.
    sheet = Image.new('RGB', (WIDTH, HEIGHT * len(frames)))
    for i, frame in enumerate(frames):
        sheet.paste(frame, (0, HEIGHT * i))
    palette = sheet.quantize(colors=128, dither=Image.Dither.NONE)
    indexed = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]
    animation = ROOT / 'assets/rpnCE-promo1.gif'
    indexed[0].save(animation, save_all=True, append_images=indexed[1:],
                    duration=[s[-1] for s in STEPS] + [6000], loop=0,
                    disposal=2, optimize=False,
                    comment=f'rpnCE {VERSION}: complex arithmetic and new value storage'.encode())
    # Keep a contact sheet with the capture evidence for visual review.
    review = Image.new('RGB', (WIDTH * 3, HEIGHT * 4), (224, 224, 224))
    for i, frame in enumerate(frames):
        review.paste(frame, ((i % 3) * WIDTH, (i // 3) * HEIGHT))
    review.save(WORK / 'animation-review.png')
    save_png(screens['square'], CEM / f'rpnCE-{VERSION}-complex.png',
             '(3 + 4i + 2)^2 = 9 + 40i, in rectangular mode with the CPLX menu.')
    save_png(screens['pdeg'], CEM / f'rpnCE-{VERSION}-complex-polar.png',
             '9 + 40i displayed as magnitude 41 and angle approximately 77.320 degrees (FIX 3).')
    return animation


def main():
    if not shutil.which('cedev-config'):
        sys.exit('Enable the CE development-kit environment before packaging.')
    subprocess.run(['make'], cwd=ROOT, check=True)
    CEM.mkdir(exist_ok=True)
    WORK.mkdir(parents=True, exist_ok=True)
    animation = render_demo()
    copies = [
        (ROOT / 'bin/rpnCE.8xp', f'rpnCE-{VERSION}.8xp'),
        (ROOT / 'bin/rpnCEres.8xv', f'rpnCEres-{VERSION}.8xv'),
        (ROOT / 'docs/INSTALL.txt', 'readme.txt'),
        (ROOT / 'LICENSE', 'LICENSE'),
        (animation, animation.name),
    ]
    for source, name in copies:
        shutil.copyfile(source, CEM / name)
    previews = {'stack': 'stack-levels-5', 'tvm': 'tvm-loan-6', 'units': 'units-convert'}
    for name, source in previews.items():
        with Image.open(ROOT / f'assets/guide/screens/{source}.png') as im:
            screen = im.convert('RGB').resize((320, 240), Image.Resampling.NEAREST)
            save_png(screen, CEM / f'rpnCE-{VERSION}-{name}.png', f'rpnCE {name} example.')
    names = [name for _, name in copies] + [
        f'rpnCE-{VERSION}-{name}.png' for name in ['complex', 'complex-polar', *previews]]
    archive = CEM / f'rpnCE-{VERSION}.zip'
    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name in names:
            z.write(CEM / name, name)
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert sorted(z.namelist()) == sorted(names)
        for name in names:
            assert z.read(name) == (CEM / name).read_bytes(), name
    for source, name in copies:
        assert source.read_bytes() == (CEM / name).read_bytes(), name
    print(f'Prepared {archive.relative_to(ROOT)}: {len(names)} files, {archive.stat().st_size:,} bytes.')
    print(f'Complex animation: {len(STEPS) + 1} frames; captures retained in {WORK.relative_to(ROOT)}.')


if __name__ == '__main__':
    main()
