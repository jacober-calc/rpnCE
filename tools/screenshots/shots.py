"""The user guide's screenshots: keystroke sequences run on rpnCE's real code.

Each sequence starts from a fresh calculator: DEC, RAD, FIX- (all digits),
a four-level stack, the clock at 2026-10-19 09:30, and the TI-OS TVM values
at their defaults (P/YR = C/YR = 12).

Tokens (see run.py):
  12.5          typed digit by digit
  ENTER + - × ÷ ^ CHS , . ( ) x² x⁻¹ SIN COS TAN LN LOG STO 2ND ALPHA
  DEL CLEAR VARS MATH MODE STAT APPS PRGM XTθn UP DOWN LEFT RIGHT ON
  [LABEL]       the menu key under LABEL (as spelled in menus/menus.txt)
  @HOME>A>B     go to that menu (row 1)
  >name         snapshot to assets/guide/screens/name.png
  !name         the frame shown while a completed prompt flashes
A sequence starting with NO_RES runs without the RPNCEres appvar.
"""
import os

FIX4 = 'MODE [FIX] 4 MATH'          # FIX 4, back to HOME
FIX4_DEG = 'MODE [FIX] 4 [DEG] MATH'
PI = '2ND ^'
RCL = '2ND STO'
SQRT = '2ND x²'
LASTX = '2ND CHS'
HELP_KEY = '2ND 0'
DOTS_MODE = 'MODE DOWN DOWN DOWN [".."]'    # DATE objects as readable text

# ---------------------------------------------------------------------------
# Menu rows (RPN83P style): NAME-1.png ... for each row, and NAME.png for the
# parent's row with this menu's key outlined in red.
# ---------------------------------------------------------------------------
def _all_menus():
    """Every menu in menus/menus.txt, as a path of labels."""
    import sys
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
    import genmenu
    out = []

    def walk(g, path):
        out.append(path)
        for row in g.rows:
            for n in row:
                if isinstance(n, genmenu.Group):
                    walk(n, path + [n.label])
    for r in genmenu.parse():
        walk(r, [r.label])
    return out


MENUS = _all_menus()

# ---------------------------------------------------------------------------
# Screens
# ---------------------------------------------------------------------------
SEQUENCES = [
    # 2. Installation: the first run
    '>install-first-run',

    # 3. First calculations
    f'{FIX4_DEG} CLEAR CLEAR >first-prepare',
    f'{FIX4_DEG} 2 ENTER >first-add-1 3 + >first-add-2 4 × >first-add-3',
    f'{FIX4_DEG} 3 ENTER 4 + >first-sums-1 5 ENTER 6 + >first-sums-2 × >first-sums-3',
    f'{FIX4_DEG} 30 SIN >first-sin',
    f'{FIX4_DEG} 9 {SQRT} >first-sqrt',
    f'{FIX4_DEG} [MATH] 3 [X^3] >first-cube',
    f'{FIX4_DEG} 5 STO MATH !first-sto-flash CLEAR >first-sto-2 {RCL} MATH >first-rcl',
    f'{FIX4_DEG} {PI} 2ND ENTER >first-show',
    f'{FIX4_DEG} {HELP_KEY} >first-help',

    # 4. The display
    f'{FIX4} 1264.136 ENTER 2 CHS ENTER 3 ÷ {PI} {RCL} ÷ MATH >display-anatomy',
    f'{FIX4_DEG} >display-status-1',
    f'{FIX4} 2ND >display-status-2',
    'MODE [SCI] 3 [DEG] APPS [HEX] [BCFS] [SCF] ALPHA >display-status-3',
    '4294967295 APPS [BIN] >display-scroll',
    f'{FIX4} 1264.136 ENTER 2 CHS ENTER 3 ÷ {PI} MODE DOWN DOWN >tint-1 [TINT] >tint-2 [TINT] >tint-3 '
    '[TINT] >tint-4 [TINT] >tint-5 [TINT] >tint-6 [TINT] >tint-7 [TINT] >tint-8',

    # 5. Entering numbers
    f'{FIX4} 6.02 >entry-exp-1 , >entry-exp-2 2 CHS >entry-exp-3 ENTER >entry-exp-4',

    # 6. The RPN stack
    '2 ENTER 3 >stack-ex-1 + >stack-ex-2 4 × >stack-ex-3',
    '3 ENTER 4 + >stack-sums-1 5 ENTER 6 + >stack-sums-2 × >stack-sums-3',
    f'{FIX4} CLEAR CLEAR 2 ENTER >stack-levels-1 3 >stack-levels-2 + >stack-levels-3 '
    '4 >stack-levels-4 × >stack-levels-5',
    f'{FIX4} 2 ENTER ENTER ENTER >stack-const-1 × >stack-const-2 × >stack-const-3 × >stack-const-4',
    f'{FIX4} 12 ENTER 3 ÷ >stack-lastx-1 {LASTX} × >stack-lastx-2 {LASTX} × >stack-lastx-3',

    # 7. Menus and HELP
    f'{FIX4_DEG} MODE >menus-mode',
    f'{HELP_KEY} >menus-help-1',
    f'{HELP_KEY} UP >menus-help-about',

    # 8. Registers
    f'{FIX4} 12.5 STO >reg-sto-prompt MATH !reg-sto-flash',
    f'{FIX4} 100 STO MATH >reg-acc-1 25 STO + MATH >reg-acc-2 {RCL} MATH >reg-acc-3 '
    f'5 >reg-acc-4 {RCL} ÷ MATH >reg-acc-5',
    f'{FIX4} MODE DOWN DOWN 41 [RSIZ] MATH 41 STO . 123 STO 2ND . CLEAR {RCL} 2ND . >reg-extra-1 '
    'VARS >reg-extra-2',
    f'{FIX4} XTθn [LENG] 12 [ft] STO MATH CLEAR 41 STO . CLEAR @HOME>TVM 360 [N] 6.5 [I%YR] '
    'MATH CLEAR VARS >reg-vars',

    # 9. Settings
    '.000012345 ENTER 1234.5678 ) >settings-fmt-all MODE [FIX] 2 >settings-fmt-fix2 [SCI] 3 '
    '>settings-fmt-sci3 [ENG] 2 >settings-fmt-eng2',
    '1 ENTER 7 ÷ MODE [FIX] 4 >settings-pres-1 [SCI] 4 >settings-pres-2 [ENG] 4 >settings-pres-3 '
    '2ND ENTER >settings-pres-4 [FIX] . 2 >settings-pres-5',

    # 10. Mathematics
    f'{FIX4} @HOME>NUM 85 ENTER 7.5 >math-pct-1 [%] >math-pct-2 + >math-pct-3',
    f'{FIX4} @HOME>NUM 95 ENTER 114 [%CH] >math-pctch',
    f'{FIX4} @HOME>NUM 18 ENTER 24 [GCD] >math-gcd 18 ENTER 24 [LCM] >math-lcm',
    '@HOME>NUM 91 [PRIM] >math-prim-1 ÷ >math-prim-2 [PRIM] >math-prim-3',
    f'{FIX4_DEG} @HOME>CONV 4 ENTER 3 >math-pol-1 [{{to}}POL] >math-pol-2 [{{to}}REC] >math-pol-3',
    f'{FIX4} @HOME>CONV 1.3 [{{to}}HR] >math-hms-1 1.45 ENTER .3 [HMS+] >math-hms-2',

    # 11. BASE
    '255 ENTER 4895 ENTER 5133136320 APPS [HEX] >base-hex',
    'APPS [DEC] 8 [BCFS] [WSIZ] ON [HEX] ALPHA COS ALPHA COS ENTER >base-ovf-1 2 + >base-ovf-2 '
    '[DEC] >base-ovf-3',
    'APPS [DEC] 8 [BCFS] [WSIZ] ON [HEX] ALPHA APPS 6 ENTER 6 5 >base-mask-1 [LOGI] [AND] '
    '>base-mask-2 ON [BIN] >base-mask-3 [DEC] >base-mask-4',
    '17 ENTER 5 APPS [BFCN] [BDIV] >base-bdiv',

    # 12. Statistics
    f'{FIX4} STAT [CL{{sigma}}] [ALL{{sigma}}] [CFIT] [LINF] ON 3.1 ENTER 1 [{{sigma}}+] '
    '>stat-entry-1 4.9 ENTER 2 [{sigma}+] 7.2 ENTER 3 [{sigma}+] 8.8 ENTER 4 [{sigma}+] '
    '11.1 ENTER 5 [{sigma}+] >stat-entry-5 [MEAN] >stat-mean [CFIT] [SLOP] >stat-slop '
    '[YINT] >stat-yint 6 [X{to}Y] >stat-forecast',

    # 13. TVM
    f'{FIX4} @HOME>TVM [CLTV] [RSTV] 360 [N] >tvm-loan-1 6.5 [I%YR] >tvm-loan-2 200000 [PV] '
    '>tvm-loan-3 0 [FV] >tvm-loan-4 [PMT] >tvm-loan-5 6 [I%YR] [PMT] >tvm-loan-6',
    f'{FIX4} @HOME>TVM [CLTV] [RSTV] 1 [P/YR] 1 [C/YR] 10 [N] 5 [I%YR] 1000 CHS [PV] 0 [PMT] '
    '[FV] >tvm-save',
    f'{FIX4} @HOME>TVM [CLTV] [RSTV] 60 [N] 25000 [PV] 480 CHS [PMT] 0 [FV] [I%YR] >tvm-rate',

    # 14. Units
    f'{FIX4} XTθn [LENG] 12 [ft] ENTER [m] >units-convert',
    f'{FIX4} XTθn [LENG] 6 [ft] 2 [in] + >units-arith-1 3 ÷ >units-arith-2 [in] >units-arith-3',
    f'{FIX4} XTθn [LENG] 12 [ft] >units-store-1 STO MATH >units-store-2 CLEAR >units-store-3 '
    f'{RCL} MATH >units-store-4 [m] >units-store-5',
    f'{FIX4} @HOME>UNIT>ENER 2 [kWh] >units-energy-1 2000 [kcal] >units-energy-2 @HOME>NUM [%CH] '
    '>units-energy-3',

    # 15. Dates and times (Time, DayOfWeek and Duration started with their
    # menu keys)
    f'{DOTS_MODE} @HOME>DATE>D [D{{}}] 2024 , 3 , 14 ENTER @HOME>DATE>T [T{{}}] 15 , 36 , 1 ENTER '
    '@HOME>DATE>DW [DW{}] 4 ENTER @HOME>DATE>DR [DR{}] 285 , 8 , 23 , 59 '
    f'{DOTS_MODE} >date-strings',       # the mode key completes the entry without moving the stack
    f'{DOTS_MODE} MATH 2ND ( 2024 , 3 , 14 ENTER >date-days-1 100 + >date-days-2 '
    '@HOME>DATE>D [DOW] >date-days-3',
    '@HOME>DATE>T [T{}] 12 , 47 , 0 ENTER [T{}] 20 , 17 , 0 - >date-tdiff-1 @HOME>DATE>DR [SEC{to}] '
    '>date-tdiff-2',
    '2ND ( 2024 , 3 , 14 , 15 , 36 , 1 , 7 CHS , 0 ENTER 13 × >date-cvtz-1 x⁻¹ >date-cvtz-2 '
    '@HOME>DATE>DT [DLNK] >date-cvtz-3',

    # 16. Troubleshooting
    f'{FIX4} 2 CHS {SQRT} >trouble-domain',
    '2ND ( 2024 , 2 , 30 ENTER >trouble-invalid',
    'NO_RES >trouble-no-res',

    # 18. Limits and compatibility
    '@HOME>CPLX [REAL] >compat-cplx',
]


# ---------------------------------------------------------------------------
# Combined images
# ---------------------------------------------------------------------------
def _font(size, bold=False):
    from PIL import ImageFont
    for name in (['DejaVuSans-Bold.ttf', 'Arial Bold.ttf', 'arialbd.ttf'] if bold else
                 ['DejaVuSans.ttf', 'Arial.ttf', 'arial.ttf']):
        for d in ('/usr/share/fonts/truetype/dejavu', '/Library/Fonts', 'C:/Windows/Fonts', ''):
            try:
                return ImageFont.truetype(os.path.join(d, name), size)
            except OSError:
                pass
    return ImageFont.load_default()


def compose(screens):
    from PIL import Image, ImageDraw

    # the parts of the screen, labelled (chapter 4)
    shot = Image.open(os.path.join(screens, 'display-anatomy.png')).convert('RGB')
    img = Image.new('RGB', (shot.width + 500, shot.height + 20), (255, 255, 255))
    img.paste(shot, (0, 0))
    d = ImageDraw.Draw(img)
    title, body = _font(26, True), _font(22)
    x = shot.width + 60
    # (y of the screen area, y of the text block, title, lines)
    for y, top, name, lines in [(18, 0, 'Status bar', ['clock, display format, angle, base']),
                                (66, 62, 'Message line', ['errors, SHOW, warnings;',
                                                          '↓↑: the menu has more rows']),
                                (250, 226, 'Stack', ['T, Z, Y and X', '(X is the bottom row)']),
                                (450, 432, 'Menu row', ['the five keys Y= … GRAPH'])]:
        d.line([shot.width + 8, y, x - 10, top + 14], fill=(90, 90, 90), width=3)
        d.text((x, top), name, fill=(20, 20, 20), font=title)
        for k, line in enumerate(lines):
            d.text((x, top + 32 + k * 28), line, fill=(70, 70, 70), font=body)
    img.quantize(colors=128, dither=Image.Dither.NONE).save(
        os.path.join(screens, 'display-anatomy-labelled.png'), optimize=True)

    def load(name):
        return Image.open(os.path.join(screens, name + '.png')).convert('RGB')

    # the status bar in three states, stacked
    bars = [load(f'display-status-{i}').crop((0, 0, 640, 36)) for i in (1, 2, 3)]
    img = Image.new('RGB', (640, 3 * 36 + 2 * 8), (255, 255, 255))
    for i, b in enumerate(bars):
        img.paste(b, (0, i * 44))
    img.quantize(colors=64, dither=Image.Dither.NONE).save(
        os.path.join(screens, 'display-status.png'), optimize=True)
