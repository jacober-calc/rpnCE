#!/usr/bin/env python3
"""Build the browser question data with independent high-precision references.

Run from the repository root with mpmath installed:
    python3 tests/hardware/questions.py [--check]
The calculator and browser need only questions.js, not Python.
"""
from datetime import date, datetime, timedelta, timezone
from decimal import Decimal, localcontext, ROUND_HALF_UP
from fractions import Fraction
import argparse
import hashlib
import json
from pathlib import Path
import sys

import mpmath as mp

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import genmenu
import genunits

mp.mp.dps = 180
SYMBOLS = dict(zip(genmenu.SYMBOLS, ['²', '³', '³', '√', 'Σ', '→', '■', 'µ', '°', '↑', '↓', '←', '×', 'ˣ']))
COMMANDS = {}

def label(value):
    for name, symbol in SYMBOLS.items():
        value = value.replace('{' + name + '}', symbol)
    return value

def walk(group, path, section):
    for row in group.rows:
        for item in row:
            if isinstance(item, genmenu.Group):
                walk(item, path + [label(item.label)], item.label if group.ident == 'HOME' else section)
            elif item[3]:
                COMMANDS.setdefault(item[3], {'path': path + [label(item[1])], 'section': section, 'label': label(item[1])})

walk(genmenu.parse()[0], ['HOME'], 'HOME')
QUESTIONS = []
REFERENCES = {
    'math': 'mpmath at 180 decimal digits, rounded to 55 significant digits for the reference. Input decimals are exact. Relative tolerance 5E-11 unless the case states otherwise; zero references use an explicit absolute tolerance only when needed.',
    'integer': 'Exact integer / rational arithmetic. No floating-point tolerance.',
    'rounding': 'Python Decimal ROUND_HALF_UP: decimal halves away from zero. No binary floating-point arithmetic.',
    'units': 'Exact factors and offsets from tools/genunits.py, evaluated separately at 180 digits. This shares unit definitions with the program; it checks conversions, not an independent audit of the definitions.',
    'date': 'Python Gregorian date/datetime and exact integer second arithmetic. Epochs and fixed offsets are stated in the procedure.',
    'tvm': 'Independent annuity equations evaluated at 180 digits. Positive values are receipts; negative values are payments. The reference is not computed by the calculator solver.',
    'contract': 'Current rpnCE 0.7.0 user guide and documented domains. Expected errors are checked separately from numerical results.'
}

def decimal(value):
    if isinstance(value, Fraction):
        return mp.mpf(value.numerator) / value.denominator
    return mp.mpf(str(value))

def number(value, exact=False, relative='5E-11', absolute='0'):
    v = decimal(value)
    stored = abs(mp.mpf(mp.nstr(v, 14)))
    if not mp.isfinite(v) or (v and (stored < mp.mpf('1E-99') or stored > mp.mpf('9.9999999999999E99'))):
        raise ValueError(f'Use an explicit error expectation for an unsupported result: {value}')
    return {'kind': 'number', 'value': mp.nstr(v, 55), 'relative': '0' if exact else relative, 'absolute': absolute}

def error(value='Err: Domain'):
    return {'kind': 'error', 'values': [value]}

def text(value):
    return {'kind': 'text', 'value': value}

def select(ident):
    return 'Select ' + ' → '.join(COMMANDS[ident]['path']) + '.'

def keys(value):
    s = str(value).upper()
    negative = s.startswith('-')
    s = s.lstrip('+-')
    parts = s.split('E')
    mantissa = parts[0]
    digits = sum(c.isdigit() for c in mantissa)
    if mantissa.startswith('.'):
        digits += 1
    if digits > 12:
        raise ValueError(f'Input exceeds 12 typed mantissa digits: {value}')
    output = mantissa + (' (−)' if negative else '')
    if len(parts) == 2:
        exponent = int(parts[1])
        if abs(exponent) > 99:
            raise ValueError(f'Exponent cannot be entered directly: {value}')
        output += ' EE ' + ('(−) ' if exponent < 0 else '') + str(abs(exponent))
    return output

def q(ident, section, title, steps, expected=None, covers=(), focus='boundary', checks=(), reference='math', **extra):
    QUESTIONS.append({'id': 'expand-' + ident, 'section': section, 'title': title, 'steps': steps, 'expected': expected,
                      'covers': list(covers), 'kind': 'result' if expected else 'observation', 'focus': focus,
                      'checks': list(checks), 'reference': REFERENCES.get(reference, reference), **extra})

def fn(tag, ident, inputs, expected, focus='boundary', setup=(), suffix=(), checks=(), reference='math', **extra):
    inputs = [str(x) for x in inputs]
    if expected and expected['kind']=='error':
        checks=list(checks)+['The rejected operation left X at '+inputs[-1]+'.']
        if len(inputs)==2: checks+=['Y still contains '+inputs[0]+'.']
    q(tag, COMMANDS[ident]['section'], f"{COMMANDS[ident]['label']}: {', '.join(inputs)}",
      list(setup) + ['Enter ' + ' ENTER '.join(keys(x) for x in inputs) + '.'] + [select(ident)] + list(suffix),
      expected, [ident], focus, checks, reference, operands=inputs, operation=ident, **extra)

KEYS = {'ADD': '+', 'SUB': '−', 'MUL': '×', 'DIV': '÷', 'POW': '^', 'SQUARE': 'x²', 'SQRT': '2ND x²', 'RECIP': 'x⁻¹',
        'LOG': 'LOG', 'ALOG': '2ND LOG', 'LN': 'LN', 'EXP': '2ND LN', 'SIN': 'SIN', 'COS': 'COS', 'TAN': 'TAN',
        'ASIN': '2ND SIN', 'ACOS': '2ND COS', 'ATAN': '2ND TAN'}

def keyfn(tag, ident, inputs, expected, focus='boundary', setup=(), suffix=(), **extra):
    inputs = [str(x) for x in inputs]
    if expected and expected['kind']=='error':
        extra['checks']=['X still contains '+inputs[-1]+'.']+(['Y still contains '+inputs[0]+'.'] if len(inputs)==2 else [])
    q(tag, 'Keyboard', f"{ident}: {', '.join(inputs)}", list(setup) + ['Enter ' + ' ENTER '.join(keys(x) for x in inputs) + f'; press {KEYS[ident]}.'] + list(suffix),
      expected, focus=focus, operands=inputs, keyFunction=ident, **extra)

# Ordinary arithmetic, cancellation, guard digits, and scale separation.
for op, pairs in {
    'ADD': [('0.1','0.2'),('-8','3'),('1E-99','1E-99'),('4E98','5E98'),('1E12','1'),('1E-40','-1E-40')],
    'SUB': [('0.3','0.2'),('-8','3'),('1E-99','1E-99'),('1.000000001','1'),('1E12','1'),('1E-40','-1E-40')],
    'MUL': [('-8','3'),('0','1E99'),('1E-50','1E-49'),('1E50','1E49'),('1E99','1E-99'),('1.23456789','9.87654321')],
    'DIV': [('1','7'),('-8','3'),('1E-99','1'),('1E99','1E99'),('1E-50','1E49'),('1E50','1E-49')]
}.items():
    for i,(a,b) in enumerate(pairs):
        x,y=decimal(a),decimal(b)
        value={'ADD':lambda:x+y,'SUB':lambda:x-y,'MUL':lambda:x*y,'DIV':lambda:x/y}[op]()
        keyfn(f'arithmetic-{op.lower()}-{i}',op,[a,b],number(value, exact=op!='DIV' and len(Decimal(mp.nstr(value,55)).normalize().as_tuple().digits)<=14),focus='normal' if i==0 else 'boundary',reference='integer' if op!='DIV' else 'math')

for op, values, function in [
 ('SQUARE',['0','-3','1E-49','1E49'],lambda x:x*x),
 ('SQRT',['0','1E-99','1E98','2'],mp.sqrt),
 ('RECIP',['-4','1E-99','1E99','7'],lambda x:1/x),
 ('LOG',['1','1E-99','1E99','1.000000001'],mp.log10),
 ('LN',['1','1E-99','1E99','1.000000001'],mp.log),
 ('ALOG',['0','-99','99','.5'],lambda x:mp.power(10,x)),
 ('EXP',['0','-1','-200','230'],mp.exp)
]:
    for i,v in enumerate(values):
        keyfn(f'{op.lower()}-{i}',op,[v],number(function(decimal(v))),focus='normal' if i==0 else 'boundary')
for i,(a,b) in enumerate([('2','-3'),('16','.5'),('-2','4'),('-2','-3'),('1E-90','.5'),('1.000000001','1E9'),('0','3')]):
    keyfn(f'power-{i}','POW',[a,b],number(mp.power(decimal(a),decimal(b))))
for op, inputs, message in [('DIV',['1','0'],'Err: Division By 0'),('RECIP',['0'],'Err: Division By 0'),('SQRT',['-1'],'Err: Domain'),('LN',['-1'],'Err: Domain'),('LOG',['0'],'Err: Domain'),('POW',['-8','.4'],'Err: Domain'),('ASIN',['1.000000001'],'Err: Domain'),('ACOS',['-1.000000001'],'Err: Domain')]:
    keyfn('domain-'+op.lower(),op,inputs,error(message),focus='domain',reference='contract')

# Trig quadrants, axes, domain endpoints, near-asymptote inputs, and reduction.
for ident,function in [('SIN',mp.sin),('COS',mp.cos),('TAN',mp.tan)]:
    for i,v in enumerate(['-1','1E-12','1E-99','1000']):
        keyfn(f'radians-{ident.lower()}-{i}',ident,[v],number(function(decimal(v))))
    for i,v in enumerate(['0','30','90','180','270','-90','360000000030']):
        if ident=='TAN' and int(v)%180==90:
            expected=error()
        else:
            angle=decimal(v)%360
            if int(angle)%90==0:
                exact_value={'SIN':[0,1,0,-1],'COS':[1,0,-1,0],'TAN':[0,0,0,0]}[ident][int(angle)//90]
                expected=number(exact_value,exact=True)
            else:
                expected=number(function(angle*mp.pi/180))
        keyfn(f'degrees-{ident.lower()}-{i}',ident,[v],expected,focus='domain' if expected['kind']=='error' else 'boundary',setup=[select('DEG')],reference='math')
for ident,function in [('ASIN',mp.asin),('ACOS',mp.acos),('ATAN',mp.atan)]:
    for i,v in enumerate(['-1','0','.99999999999','1'] if ident!='ATAN' else ['-1E99','-1E-99','1E-99','1E99']):
        for deg in [False,True]:
            value=function(decimal(v))*(180/mp.pi if deg else 1)
            keyfn(f'inverse-{ident.lower()}-{i}-{int(deg)}',ident,[v],number(value),setup=[select('DEG' if deg else 'RAD')])
for i,v in enumerate(['89.999999','90.000001','-89.999999']):
    keyfn(f'tangent-pole-{i}','TAN',[v],number(mp.tan(decimal(v)*mp.pi/180),relative='5E-6'),setup=[select('DEG')],reference='mpmath, 180 digits. At one millionth of a degree from 90, a 5E-14 radian conversion error can amplify to about 3E-6 relative error in TAN. This case allows 5E-6; exactly 90 degrees has a separate domain test.')

# Every MATH function: signs, axes, scale, small arguments, and restrictions.
for ident,values,function in [
 ('CUBE',['0','-2','1E-30','1E30'],lambda x:x**3),
 ('CUBE_ROOT',['0','-27','1E-90','1E90'],lambda x:mp.sign(x)*mp.root(abs(x),3)),
 ('TWO_POW',['0','-10','-300','300','.5'],lambda x:mp.power(2,x)),
 ('LOG2',['1','.5','1E-99','1E99'],lambda x:mp.log(x,2)),
 ('EXP_MINUS_ONE',['0','-1E-99','1E-14','-.9999999999','-1','1','-.1','.9999999999','1.000000001'],mp.expm1),
 ('LN_ONE_PLUS',['0','-1E-99','1E-14','-.9999999999','-.1','-.0999999999','.0999999999','.1','1E99'],mp.log1p)
]:
    for i,v in enumerate(values): fn(f'{ident.lower()}-{i}',ident,[v],number(function(decimal(v))))
for i,(a,b) in enumerate([('81','4'),('-32','5'),('-8','-3'),('1E-90','3'),('1E90','3'),('0','3')]):
    value=mp.sign(decimal(a))*mp.power(abs(decimal(a)),1/decimal(b)) if decimal(a)<0 else mp.power(decimal(a),1/decimal(b))
    fn(f'xroot-{i}','XROOT_Y',[a,b],number(value))
for i,(a,b) in enumerate([('1','2'),('8','.5'),('1E99','10'),('1E-99','10')]): fn(f'log-base-{i}','LOG_BASE',[a,b],number(mp.log(decimal(a))/mp.log(decimal(b))))
for i,(y,x) in enumerate([('0','0'),('1','0'),('0','-1'),('-1','0'),('1','-1'),('-1','-1'),('-1','1'),('1E-40','1E40'),('1E40','1E-40')]):
    for deg in [False,True]:
        angle=mp.atan2(decimal(y),decimal(x))*(180/mp.pi if deg else 1)
        fn(f'atan2-{i}-{int(deg)}','ATAN2',[y,x],number(angle),setup=[select('DEG' if deg else 'RAD')])
for ident,inputs,message in [('XROOT_Y',['8','0'],'Err: Division By 0'),('XROOT_Y',['-8','2'],'Err: Domain'),('XROOT_Y',['-32','2.5'],'Err: Domain'),('XROOT_Y',['0','-3'],'Err: Domain'),('LOG2',['0'],'Err: Domain'),('LOG_BASE',['8','1'],'Err: Division By 0'),('LOG_BASE',['0','2'],'Err: Domain'),('LOG_BASE',['8','-2'],'Err: Domain'),('LN_ONE_PLUS',['-1'],'Err: Domain'),('LN_ONE_PLUS',['-1.000000001'],'Err: Domain')]:
    fn('invalid-'+ident.lower()+'-'+str(len(QUESTIONS)),ident,inputs,error(message),focus='domain',reference='contract')

# NUM: signed remainders, integer boundaries, percentages, exact decimal rounding.
for ident,values,function in [
 ('INT_PART',['0','2.999999999','-2.999999999','1E-99'],lambda x:mp.sign(x)*mp.floor(abs(x))),
 ('FRAC_PART',['0','2.999999999','-2.999999999','1E-99'],lambda x:x-mp.sign(x)*mp.floor(abs(x))),
 ('FLOOR',['0','-.000000001','2','-2.000000001'],mp.floor),
 ('CEIL',['0','.000000001','2','-2.000000001'],mp.ceil),
 ('NEAR',['0','2.5','-2.5','2.499999999','-2.499999999'],lambda x:mp.sign(x)*mp.floor(abs(x)+mp.mpf('.5'))),
 ('ABS',['0','-1E-99','-1E99','1E99'],abs),
 ('SIGN',['0','-1E-99','1E-99','-1E99','1E99'],mp.sign)
]:
    for i,v in enumerate(values): fn(f'num-{ident.lower()}-{i}',ident,[v],number(function(decimal(v)),exact=True),reference='integer')
for ident,pairs,function in [
 ('MOD',[('-17','5'),('17','-5'),('-17','-5'),('10','5'),('1E-20','3E-21')],lambda a,b:a-b*mp.floor(a/b)),
 ('MIN',[('-8','3'),('3','3'),('1E-99','2E-99')],min),
 ('MAX',[('-8','3'),('3','3'),('1E-99','2E-99')],max),
 ('PERCENT',[('200','0'),('200','-15'),('1E90','1E-90'),('-200','15')],lambda a,b:a*b/100),
 ('PERCENT_CHANGE',[('100','100'),('100','0'),('100','80'),('-100','-80'),('1E-50','2E-50')],lambda a,b:(b-a)/a*100)
]:
    for i,(a,b) in enumerate(pairs): fn(f'num-{ident.lower()}-{i}',ident,[a,b],number(function(decimal(a),decimal(b)),exact=True),checks=['Y retains the original first operand.'] if ident in ['PERCENT','PERCENT_CHANGE'] else [],reference='integer')
for ident in ['GCD','LCM']:
    import math
    for i,(a,b) in enumerate([(0,0),(0,7),(7,0),(17,19),(123456,7890),(4294967295,65535)]):
        fn(f'{ident.lower()}-{i}',ident,[a,b],number(math.gcd(a,b) if ident=='GCD' else math.lcm(a,b),exact=True),reference='integer')
    for i,pair in enumerate([[-1,2],[2,-1],[2.5,4],[4,2.5]]): fn(f'domain-{ident.lower()}-{i}',ident,pair,error(),focus='domain',reference='contract')
for i,(v,answer) in enumerate([('2',1),('3',1),('4',2),('9',3),('65521',1),('4294967295',3),('4294967291',1),('4293001441',65521)]):
    fn(f'prime-{i}','PRIME',[v],number(answer,exact=True),reference='Exact trial division with Python integer arithmetic.',checks=[f'Y retains {v}.'])
for i,v in enumerate(['0','1','-2','2.5','4294967296']): fn(f'prime-domain-{i}','PRIME',[v],error(),focus='domain',reference='contract')
for ident,inputs,message in [('MOD',[1,0],'Err: Division By 0'),('PERCENT_CHANGE',[0,100],'Err: Division By 0')]: fn('domain-'+ident.lower(),ident,inputs,error(message),focus='domain',reference='contract')
for i,(v,digits) in enumerate([('1.005',2),('-1.005',2),('9.995',2),('-.005',2),('2.5',0),('-2.5',0),('1.23456789',6),('1E-99',9)]):
    with localcontext() as ctx:
        ctx.prec=180
        want=Decimal(v).quantize(Decimal(1).scaleb(-digits),rounding=ROUND_HALF_UP)
    fn(f'round-decimal-{i}','ROUND_TO_N',[v],number(str(want),exact=True),suffix=[f'At RNDN press {digits}.'],reference='rounding')
for i,(v,digits) in enumerate([('999.5',3),('-999.5',3),('.009995',3),('1.23456789',1),('1E-99',3),('1E99',3)]):
    with localcontext() as ctx:
        ctx.prec=180; d=Decimal(v); quantum=Decimal(1).scaleb(d.adjusted()-digits+1); want=d.quantize(quantum,rounding=ROUND_HALF_UP)
    fn(f'round-significant-{i}','ROUND_TO_GUARD',[v],number(str(want),exact=True),suffix=[f'At RNDG press {digits}.'],reference='rounding')
for style in ['FIX','SCI','ENG']:
    with localcontext() as ctx:
        ctx.prec=180
        want=Decimal('1234.567').quantize(Decimal('0.01') if style=='FIX' else Decimal('1E1'),rounding=ROUND_HALF_UP)
    fn('round-display-'+style,'ROUND_TO_FIX',['1234.567'],number(str(want),exact=True),setup=[select(style),'At the digit prompt press 2.'],reference='rounding')

# Probability: endpoints, combinatorial symmetry, large exact answers, invalid domains.
for ident in ['COMB','PERM']:
    import math
    for i,(n,r) in enumerate([(0,0),(10,0),(10,10),(52,5),(20,10),(100,2)]):
        value=math.comb(n,r) if ident=='COMB' else math.perm(n,r)
        fn(f'prob-{ident.lower()}-{i}',ident,[n,r],number(value,exact=len(str(value))<=14),reference='integer' if len(str(value))<=14 else 'math')
    for i,pair in enumerate([[3,4],[-3,2],[3,-1],[3.5,2],[3,1.5]]): fn(f'prob-domain-{ident.lower()}-{i}',ident,pair,error(),focus='domain',reference='contract')
for i,v in enumerate([1,2,10,20,68]): fn(f'factorial-{i}','FACTORIAL',[v],number(mp.factorial(v),exact=v<=10))
for i,v in enumerate([-1,'.5',70]): fn(f'factorial-domain-{i}','FACTORIAL',[v],error(),focus='domain',reference='contract')
for seed in ['0','1','4294967295']:
    q('random-repeat-'+seed,'PROB','SEED '+seed+': repeat a short sequence',[
      'Enter '+keys(seed)+'.',select('RANDOM_SEED'),select('RANDOM'),'Write the first value in Notes.',select('RANDOM'),'Write the second value in Notes.',
      'Enter '+keys(seed)+'.',select('RANDOM_SEED'),select('RANDOM'),'Compare the first value.',select('RANDOM'),'Compare the second value.'
    ],covers=['RANDOM','RANDOM_SEED'],focus='boundary',checks=['Both repeated values exactly match the values from the first sequence.','All four draws are greater than or equal to 0 and strictly less than 1.'],reference='contract')

# HYP: even/odd symmetry, smallest arguments, series branches, domain edges.
for ident,values,function in [
 ('SINH',['0','-1','1E-99','-1E-99','.9999999999','1.000000001','-230.5'],mp.sinh),
 ('COSH',['0','-1','1E-99','.9999999999','1.000000001','230.5'],mp.cosh),
 ('TANH',['0','-1','1E-99','-1E-99','19.99999999','20','20.00000001','-1E99'],mp.tanh),
 ('ASINH',['0','-1','1E-99','-1E-99','9999999999','1E10','10000000001','1E99','-1E99'],mp.asinh),
 ('ACOSH',['1','1.000000001','9999999999','1E10','10000000001','1E99'],mp.acosh),
 ('ATANH',['0','-.5','1E-99','-1E-99','.99999999999','-.99999999999'],mp.atanh)
]:
    for i,v in enumerate(values):
        expected=number(function(decimal(v)))
        if ident in ['COSH','SINH'] and abs(decimal(v))>230: expected['errors']=['Err: Overflow']
        fn(f'hyp-{ident.lower()}-{i}',ident,[v],expected,knownIssue=(ident in ['COSH','SINH'] and abs(decimal(v))>230))
for ident,values in [('ACOSH',['0','-1','.99999999999']),('ATANH',['1','-1','1.000000001','-1.000000001'])]:
    for i,v in enumerate(values): fn(f'hyp-domain-{ident.lower()}-{i}',ident,[v],error(),focus='domain',reference='contract')

# CONV: both outputs are graded numerically, not only acknowledged visually.
for i,(y,x) in enumerate([('0','0'),('1','0'),('0','-1'),('-4','-3'),('-4','3'),('4','-3'),('4E90','3E90'),('4E-90','3E-90')]):
    for angle in [False,True]:
        value=mp.atan2(decimal(y),decimal(x)) if angle else mp.hypot(decimal(x),decimal(y))
        expected=number(value)
        if i>=6: expected['errors']=['Err: Overflow' if i==6 else 'Err: Underflow']
        fn(f'polar-{i}-{int(angle)}','RECT_TO_POLAR',[y,x],expected,suffix=['If conversion succeeded, press ) (X↔Y) to put the angle in X. If it failed, record its error immediately.'] if angle else [],knownIssue=i>=6,focus='regression' if i>=6 else 'boundary',output='angle' if angle else 'radius')
for i,(theta,radius) in enumerate([('0','0'),('90','5'),('180','5'),('270','5'),('-30','2'),('45','1E90'),('45','1E-90')]):
    for y in [False,True]:
        value=decimal(radius)*(mp.sin(decimal(theta)*mp.pi/180) if y else mp.cos(decimal(theta)*mp.pi/180))
        if int(theta)%90==0: value=decimal(radius)*({'x':[1,0,-1,0],'y':[0,1,0,-1]}['y' if y else 'x'][int(decimal(theta)%360)//90])
        fn(f'rectangular-{i}-{int(y)}','POLAR_TO_RECT',[theta,radius],number(value),setup=[select('DEG')],suffix=['Press ) (X↔Y) to put the y coordinate in X.'] if y else [],output='y coordinate' if y else 'x coordinate')
for ident,values,function in [('RAD_TO_DEG',['-1','1E-99','1'],lambda x:x*180/mp.pi),('DEG_TO_RAD',['-180','1E-90','360'],lambda x:x*mp.pi/180)]:
    for i,v in enumerate(values): fn(f'angle-convert-{ident.lower()}-{i}',ident,[v],number(function(decimal(v))))
for i,(hms,want) in enumerate([('0','0'),('-1.3045','-1.5125'),('.0059',mp.mpf(59)/3600),('23.5959',mp.mpf(86399)/3600)]): fn(f'hms-hours-{i}','HMS_TO_HR',[hms],number(want))
for i,(hours,want) in enumerate([('0','0'),('-1.5125','-1.3045'),('23.5','23.3'),('1.999999999','1.59599999964')]): fn(f'hours-hms-{i}','HR_TO_HMS',[hours],number(want))
for ident,pairs in [('HMS_PLUS',[('0.5959','.0001','1'),('-1.30','.30','-1')]),('HMS_MINUS',[('1','.0001','.5959'),('0','.30','-.30')])]:
    for i,(a,b,want) in enumerate(pairs): fn(f'hms-arithmetic-{ident.lower()}-{i}',ident,[a,b],number(want))

# BASE references use Python integers, independently of the C bit routines.
def enter_word(value):
    s=str(value)
    if len(s)<=12:
        return ['Enter '+keys(s)+'.']
    prefix,tail=s[:12],s[12:]
    return [f'Enter {prefix} ENTER {10**len(tail)} × {int(tail)} +. This constructs {s} without exceeding the entry limit.']

BASE_OPS = ['BITWISE_AND','BITWISE_OR','BITWISE_XOR','BITWISE_NOT','BITWISE_NEG','SHIFT_LEFT_LOGICAL','SHIFT_RIGHT_LOGICAL','SHIFT_RIGHT_ARITHMETIC',
 'SHIFT_LEFT_LOGICAL_N','SHIFT_RIGHT_LOGICAL_N','ROTATE_LEFT_CIRCULAR','ROTATE_RIGHT_CIRCULAR','ROTATE_LEFT_CARRY','ROTATE_RIGHT_CARRY',
 'ROTATE_LEFT_CIRCULAR_N','ROTATE_RIGHT_CIRCULAR_N','ROTATE_LEFT_CARRY_N','ROTATE_RIGHT_CARRY_N','CLEAR_BIT','SET_BIT','GET_BIT','REVERSE_BIT','COUNT_BIT','BASE_ADD','BASE_SUBT','BASE_MULT','BASE_DIV','BASE_DIV2']

def base_case(tag,ident,bits,inputs,want,carry=0,final_carry=None,focus='boundary',output='result'):
    steps=['Enter '+str(bits)+'.',select('SET_WORD_SIZE'),select('SET_CARRY_FLAG' if carry else 'CLEAR_CARRY_FLAG')]
    for i,value in enumerate(inputs):
        steps+=enter_word(value)
        if i<len(inputs)-1: steps+=['Press ENTER to separate the operands.']
    steps+=[select(ident)]
    covers=[ident]
    if output=='carry': steps+=[select('GET_CARRY_FLAG')]; covers+=['GET_CARRY_FLAG']
    checks=[]
    if final_carry is not None and output!='carry': checks=[f'The carry flag is {final_carry}: '+('CF is shown in the status bar.' if final_carry else 'CF is absent from the status bar.')]
    q(tag,'BASE',f'{COMMANDS[ident]["label"]}, {bits}-bit word: '+('carry output' if output=='carry' else ', '.join(str(x) for x in inputs)),steps,
      number(want,exact=True),covers,focus,checks,'integer',operation=ident,operands=[str(x) for x in inputs],wordSize=bits,initialCarry=carry,output=output)

for bits in [1,8,32,44]:
    mask=(1<<bits)-1; top=1<<(bits-1)
    matrix=[
      ('BITWISE_AND',[mask,0],0,None),('BITWISE_OR',[0,mask],mask,None),('BITWISE_XOR',[mask,mask],0,None),('BITWISE_NOT',[0],mask,None),('BITWISE_NEG',[1],mask,None),
      ('SHIFT_LEFT_LOGICAL',[top],0,1),('SHIFT_RIGHT_LOGICAL',[1],0,1),('SHIFT_RIGHT_ARITHMETIC',[top],top|(top>>1),top&1),
      ('SHIFT_LEFT_LOGICAL_N',[mask,bits],0,1),('SHIFT_RIGHT_LOGICAL_N',[mask,bits],0,1),
      ('ROTATE_LEFT_CIRCULAR',[top],1,1),('ROTATE_RIGHT_CIRCULAR',[1],top,1),('ROTATE_LEFT_CARRY',[top],1,1),('ROTATE_RIGHT_CARRY',[1],top,1),
      ('ROTATE_LEFT_CIRCULAR_N',[1,0],1,1),('ROTATE_RIGHT_CIRCULAR_N',[1,bits],1,1 if bits==1 else 0),
      ('ROTATE_LEFT_CARRY_N',[mask,bits],mask,1),('ROTATE_RIGHT_CARRY_N',[mask,bits],mask,1),
      ('CLEAR_BIT',[mask,bits-1],top-1,None),('SET_BIT',[0,bits-1],top,None),('GET_BIT',[top,bits-1],1,None),('REVERSE_BIT',[1],top,None),('COUNT_BIT',[mask],bits,None),
      ('BASE_ADD',[mask,1],0,1),('BASE_SUBT',[0,1],mask,1),('BASE_MULT',[mask,1 if bits==1 else 2],1 if bits==1 else mask-1,0 if bits==1 else 1),
      ('BASE_DIV',[mask,1],mask,None),('BASE_DIV2',[mask,1],0,None)]
    for ident,inputs,want,cf in matrix:
        initial=1 if ident.startswith('ROTATE') else 0
        base_case(f'base-{bits}-{ident.lower()}',ident,bits,inputs,want,initial,cf)
        if cf is not None:
            base_case(f'base-carry-{bits}-{ident.lower()}',ident,bits,inputs,cf,initial,output='carry')
for ident in BASE_OPS:
    binary=ident in ['BITWISE_AND','BITWISE_OR','BITWISE_XOR','SHIFT_LEFT_LOGICAL_N','SHIFT_RIGHT_LOGICAL_N','ROTATE_LEFT_CIRCULAR_N','ROTATE_RIGHT_CIRCULAR_N','ROTATE_LEFT_CARRY_N','ROTATE_RIGHT_CARRY_N','CLEAR_BIT','SET_BIT','GET_BIT','BASE_ADD','BASE_SUBT','BASE_MULT','BASE_DIV','BASE_DIV2']
    fn('base-domain-'+ident.lower(),ident,['1','-1'] if binary else ['-1'],error(),focus='domain',setup=['Enter 8.',select('SET_WORD_SIZE')],reference='contract')
for ident in ['SHIFT_LEFT_LOGICAL_N','SHIFT_RIGHT_LOGICAL_N','ROTATE_LEFT_CIRCULAR_N','ROTATE_RIGHT_CIRCULAR_N','ROTATE_LEFT_CARRY_N','ROTATE_RIGHT_CARRY_N']:
    base_case('base-zero-count-'+ident.lower(),ident,8,[129,0],129,carry=1,final_carry=1)
    fn('base-excess-count-'+ident.lower(),ident,[1,9],error(),focus='domain',setup=['Enter 8.',select('SET_WORD_SIZE')],reference='contract')
for ident in ['CLEAR_BIT','SET_BIT','GET_BIT']:
    fn('base-index-'+ident.lower(),ident,[255,8],error(),focus='domain',setup=['Enter 8.',select('SET_WORD_SIZE')],reference='contract')
for ident in ['BASE_DIV','BASE_DIV2']:
    fn('base-zero-divisor-'+ident.lower(),ident,[1,0],error('Err: Division By 0'),focus='domain',setup=['Enter 8.',select('SET_WORD_SIZE')],reference='contract')
fn('base-word-overflow','BITWISE_NOT',[256],error('Err: Word Size'),focus='domain',setup=['Enter 8.',select('SET_WORD_SIZE')],reference='contract')
fn('base-fraction','BITWISE_NOT',['.5'],error(),focus='domain',setup=['Enter 8.',select('SET_WORD_SIZE')],reference='contract')
for i,v in enumerate([0,45,'1.5']): fn('word-size-domain-'+str(i),'SET_WORD_SIZE',[v],error(),focus='domain',reference='contract')
for bits in [1,44]:
    fn('word-size-query-'+str(bits),'GET_WORD_SIZE',[0],number(bits,exact=True),setup=['Enter '+str(bits)+'.',select('SET_WORD_SIZE')],reference='integer')

# UNIT: zero, sign, tiny/large scales, reverse conversions, and affine/reciprocal rules.
for ident,definition in genunits.U.items():
    dimension,factor,*rest=definition
    base=genunits.DIMS[dimension]
    # Parsec is recomputed at the reference precision, not genunits' output precision.
    f=mp.mpf(149597870700)*648000/mp.pi if ident=='UNIT_PARSEC' else decimal(factor)
    offset=decimal(rest[0]) if rest else mp.mpf(0)
    reciprocal=len(rest)>1 and rest[1]=='reciprocal'
    values=['1E-30','1E30']
    if dimension not in ['TEMP','FUEL']: values+=['0','-2.5']
    elif dimension=='TEMP': values+=['0','-40'] if ident in ['UNIT_CELSIUS','UNIT_FAHRENHEIT'] else ['0','300']
    else: values+=['30']
    for i,v in enumerate(values):
        expected=f/decimal(v) if reciprocal else (decimal(v)+offset)*f
        if ident==base: expected=decimal(v)
        q('unit-scale-'+ident.lower()+'-'+str(i),'UNIT',f'{v} {COMMANDS[ident]["label"]} → {COMMANDS[base]["label"]}',
          ['Enter '+keys(v)+'.',select(ident),select(base)],number(expected),[ident],checks=['The destination unit is '+COMMANDS[base]['label']+'.'],reference='units',unit=ident,input=v,conversion='to-base')
    back='300' if dimension=='TEMP' else '123.456'
    target=base
    # Exercise a second unit even when the source already is the base.
    if ident==base:
        target=next(k for k,v in genunits.U.items() if v[0]==dimension and k!=ident)
    q('unit-roundtrip-'+ident.lower(),'UNIT',f'{COMMANDS[ident]["label"]}: convert away and back',
      ['Enter '+keys(back)+'.',select(ident),select(target),select(ident)],number(back),[ident],focus='interaction',checks=['X again carries the original unit '+COMMANDS[ident]['label']+'.'],reference='units',unit=ident,input=back,conversion='roundtrip')
for source,target,operand in [('UNIT_MILES_PER_GALLON','UNIT_LITERS_PER_HUNDRED_KILO_METERS','0'),('UNIT_LITERS_PER_HUNDRED_KILO_METERS','UNIT_MILES_PER_GALLON','0')]:
    q('fuel-zero-'+source.lower(),'UNIT','Zero fuel value: reciprocal conversion', ['Enter '+operand+'.',select(source),select(target)],error('Err: Division By 0'),[source,target],focus='domain',reference='contract')
for source,target,operand,want in [('UNIT_CELSIUS','UNIT_FAHRENHEIT','-40',-40),('UNIT_FAHRENHEIT','UNIT_CELSIUS','212',100),('UNIT_KELVIN','UNIT_CELSIUS','0','-273.15'),('UNIT_RANKINE','UNIT_FAHRENHEIT','0','-459.67')]:
    q('temperature-'+source.lower(),'UNIT','Temperature offset: '+operand+' '+COMMANDS[source]['label'],['Enter '+keys(operand)+'.',select(source),select(target)],number(want),[source,target],reference='units')
for ident in ['UNIT_VALUE','UNIT_BASE']:
    q('unit-utility-zero-'+ident.lower(),'UNIT',COMMANDS[ident]['label']+' on a zero-valued quantity', ['Enter 0.',select('UNIT_FOOT'),select(ident)],number(0,exact=True),[ident],checks=['X has no unit.' if ident=='UNIT_VALUE' else 'X has unit m.'],reference='integer')
for op,key,want in [('subtract','−','5.8333333333333333333333333333333333333333333333333'),('scale','×',18),('divide','÷',2)]:
    steps=['Enter 6.',select('UNIT_FOOT')]
    if op=='subtract': steps+=['Enter 2.',select('UNIT_INCH')]
    else: steps+=['Enter 3.']
    steps+=['Press '+key+'.']
    q('quantity-'+op,'UNIT','Quantity '+op,steps,number(want),focus='interaction',checks=['The result is labelled ft.'],reference='units')
for ident,operand,want,keep in [('INT_PART','-2.75',-2,True),('FRAC_PART','-2.75','-.75',True),('FLOOR','-2.75',-3,True),('CEIL','-2.75',-2,True),('NEAR','-2.5',-3,True),('ABS','-2.75','2.75',True),('SIGN','-2.75',-1,False)]:
    q('quantity-num-'+ident.lower(),'NUM',COMMANDS[ident]['label']+' on a length',['Enter '+keys(operand)+'.',select('UNIT_FOOT'),select(ident)],number(want,exact=True),[ident],focus='interaction',checks=['The result retains ft.' if keep else 'The result has no unit.'],reference='integer')
for ident in ['MIN','MAX','MOD']:
    result={'MIN':mp.mpf(2)/12,'MAX':mp.mpf(6),'MOD':mp.mpf(0)}[ident]
    expected=number(result,absolute='1E-11' if ident=='MOD' else '0')
    if ident=='MOD': expected['errors']=['Err: Precision']
    q('quantity-binary-'+ident.lower(),'NUM',COMMANDS[ident]['label']+' converts inches to feet',['Enter 6.',select('UNIT_FOOT'),'Enter 2.',select('UNIT_INCH'),select(ident)],expected,[ident],focus='regression' if ident=='MOD' else 'interaction',knownIssue=ident=='MOD',
      checks=['On success the result retains ft. On an error, X remains 2 in and Y remains 6 ft.'] if ident=='MOD' else ['The result retains ft.'],
      reference='Exactly 6 ft is 72 in, so MOD 2 in is zero. Converting 2 in to feet introduces a repeating decimal; Err: Precision is acceptable if the integer quotient cannot be resolved safely. A remainder near 2 in is incorrect.' if ident=='MOD' else 'units')
for kind in ['UNIT_CELSIUS','UNIT_MILES_PER_GALLON']:
    for ident in ['ABS','SIGN','PERCENT_CHANGE']:
        steps=['Enter 10.',select(kind)]
        if ident=='PERCENT_CHANGE': steps+=['Enter 20.',select(kind)]
        steps+=[select(ident)]
        q('quantity-invalid-'+kind.lower()+'-'+ident.lower(),'UNIT',COMMANDS[ident]['label']+' rejects '+COMMANDS[kind]['label'],steps,error('Err: Data Type'),[ident],focus='domain',reference='contract')

# STAT: independent datasets, both coordinates, degeneracy and cancellation.
def stat_data(points, linear=False):
    steps=[select('STAT_CLEAR'),select('STAT_LINEAR_MODE' if linear else 'STAT_ALL_MODE')]
    for x,y in points:
        steps+=['Enter '+keys(y)+' ENTER '+keys(x)+'.',select('STAT_PLUS')]
    return steps

REGS=['STAT_REG_N','STAT_REG_X','STAT_REG_X2','STAT_REG_Y','STAT_REG_Y2','STAT_REG_XY',
 'STAT_REG_LN_X','STAT_REG_LN_X2','STAT_REG_LN_Y','STAT_REG_LN_Y2','STAT_REG_LN_XLN_Y','STAT_REG_XLN_Y','STAT_REG_YLN_X']
for ident in REGS+['STAT_SUM','STAT_N']:
    q('stat-empty-'+ident.lower(),'STAT',COMMANDS[ident]['label']+' after clearing', [select('STAT_CLEAR'),select(ident)],number(0,exact=True),[ident],reference='integer')
for name,points in [
 ('varied',[('1','2'),('3','1'),('4','5'),('7','3')]),
 ('negative',[('-3','6'),('-1','2'),('1','-2'),('3','-6')]),
 ('tiny',[('1E-40','2E-40'),('2E-40','4E-40'),('3E-40','6E-40')]),
 ('large',[('1E45','2E45'),('2E45','4E45'),('3E45','6E45')]),
 ('constant',[('2','4'),('2','4'),('2','4')]),
 ('offset',[('100000001','2'),('100000002','4'),('100000003','6')])
]:
    xs=[decimal(p[0]) for p in points];ys=[decimal(p[1]) for p in points];n=len(points)
    sx=sum(xs);sy=sum(ys);sxy=sum(x*y for x,y in zip(xs,ys));mx=sx/n;my=sy/n
    vx=sum((x-mx)**2 for x in xs);vy=sum((y-my)**2 for y in ys);cv=sum((x-mx)*(y-my) for x,y in zip(xs,ys))
    setup=stat_data(points)
    targets=[('STAT_MEAN',mx,my),('STAT_SUM',sx,sy),('STAT_SAMPLE_SDEV',mp.sqrt(vx/(n-1)),mp.sqrt(vy/(n-1))),('STAT_POP_SDEV',mp.sqrt(vx/n),mp.sqrt(vy/n)),('STAT_SAMPLE_COV',cv/(n-1),None),('STAT_POP_COV',cv/n,None)]
    if sx and sy: targets+=[('STAT_WEIGHTED_MEAN',sxy/sy,sxy/sx)]
    for ident,xwant,ywant in targets:
        for second in [False,True] if ywant is not None else [False]:
            want=ywant if second else xwant
            expected=number(want,absolute='1E-11' if name=='varied' and not want else '0')
            if name=='offset' and ident in ['STAT_SAMPLE_SDEV','STAT_POP_SDEV']: expected['errors']=['Err: Precision']
            q('stat-'+name+'-'+ident.lower()+'-'+str(int(second)),'STAT',COMMANDS[ident]['label']+' on '+name+' data: '+('Y' if second else 'X')+' result',
              setup+[select(ident)]+(['If the operation succeeded, press ) (X↔Y) to move the second result into X. Otherwise record its error immediately.'] if second else []),
              expected,[ident],focus='regression' if name=='offset' and ident in ['STAT_SAMPLE_SDEV','STAT_POP_SDEV'] else 'boundary',
              knownIssue=name=='offset' and ident=='STAT_SAMPLE_SDEV',reference='Independent centered sums of exact inputs, evaluated at 180 digits; variances use deviations from the mean rather than subtraction of nearly equal large sums.',dataset=points,output='Y' if second else 'X')
    if name=='varied':
        logsx=[mp.log(x) for x in xs];logsy=[mp.log(y) for y in ys]
        values=[n,sx,sum(x*x for x in xs),sy,sum(y*y for y in ys),sxy,sum(logsx),sum(x*x for x in logsx),sum(logsy),sum(y*y for y in logsy),sum(x*y for x,y in zip(logsx,logsy)),sum(x*y for x,y in zip(xs,logsy)),sum(x*y for x,y in zip(ys,logsx))]
        for ident,want in zip(REGS,values): q('stat-register-varied-'+ident.lower(),'STAT',COMMANDS[ident]['label']+' on varied data',setup+[select(ident)],number(want),[ident],dataset=points)
for ident in ['STAT_MEAN','STAT_WEIGHTED_MEAN','STAT_SAMPLE_SDEV','STAT_POP_SDEV','STAT_SAMPLE_COV','STAT_POP_COV']:
    q('stat-no-data-'+ident.lower(),'STAT',COMMANDS[ident]['label']+' rejects an empty dataset',[select('STAT_CLEAR'),select(ident)],error('Err: Stat'),[ident],focus='domain',reference='contract')
for ident in ['STAT_SAMPLE_SDEV','STAT_SAMPLE_COV']:
    q('stat-one-point-'+ident.lower(),'STAT',COMMANDS[ident]['label']+' needs two points',stat_data([('2','4')])+[select(ident)],error('Err: Stat'),[ident],focus='domain',reference='contract')
q('stat-pop-one','STAT','Population deviation of one point',stat_data([('2','4')])+[select('STAT_POP_SDEV')],number(0,exact=True),['STAT_POP_SDEV'],reference='integer',checks=['Y is also zero.'])
q('stat-weights-zero','STAT','WMN with zero total weights',stat_data([('-1','-2'),('1','2')])+[select('STAT_WEIGHTED_MEAN')],error('Err: Stat'),['STAT_WEIGHTED_MEAN'],focus='domain',reference='contract')
q('stat-remove-last','STAT','Remove the only point',stat_data([('2','4')])+['Enter 4 ENTER 2.',select('STAT_MINUS'),select('STAT_N')],number(0,exact=True),['STAT_MINUS','STAT_N'],reference='integer')


models=[
 ('CFIT_LINEAR',[('-1','5'),('0','3'),('1','1')],-2,3,-1,4,-5),
 ('CFIT_LOG',[('1','2'),('10','5'),('100','8')],3/mp.log(10),2,1,1000,11),
 ('CFIT_EXP',[('0','3'),('1','6'),('2','12')],mp.log(2),3,1,3,24),
 ('CFIT_POWER',[('1','3'),('2','12'),('3','27')],2,3,1,4,48)
]
for model,points,slope,intercept,corr,x,y in models:
    setup=stat_data(points)+[select(model)]
    for ident,want,operand in [('CFIT_SLOPE',slope,None),('CFIT_INTERCEPT',intercept,None),('CFIT_CORRELATION',corr,None),('CFIT_FORECAST_Y',y,x),('CFIT_FORECAST_X',x,y)]:
        q('fit-model-'+model.lower()+'-'+ident.lower(),'STAT',COMMANDS[model]['label']+': '+COMMANDS[ident]['label'],setup+(['Enter '+keys(operand)+'.'] if operand is not None else [])+[select(ident)],number(want),[model,ident],dataset=points,reference='Independent transformed linear-regression equations; the model has an exact defining curve and all typed data are exact.')
for model in ['CFIT_LINEAR','CFIT_LOG','CFIT_EXP','CFIT_POWER']:
    q('fit-degenerate-'+model.lower(),'STAT',COMMANDS[model]['label']+': equal x values',stat_data([('2','3'),('2','5')])+[select(model),select('CFIT_SLOPE')],error('Err: Stat'),[model,'CFIT_SLOPE'],focus='domain',reference='contract')
for model,points in [('CFIT_LOG',[('0','1'),('1','2')]),('CFIT_EXP',[('1','0'),('2','1')]),('CFIT_POWER',[('-1','1'),('1','2')])]:
    q('fit-invalid-log-'+model.lower(),'STAT',COMMANDS[model]['label']+' rejects nonpositive log data',stat_data(points)+[select(model)],error('Err: Stat'),[model],focus='domain',reference='contract')
for model,ident,value in [('CFIT_LOG','CFIT_FORECAST_Y','0'),('CFIT_POWER','CFIT_FORECAST_Y','-1'),('CFIT_EXP','CFIT_FORECAST_X','0'),('CFIT_POWER','CFIT_FORECAST_X','-1')]:
    points=next(m[1] for m in models if m[0]==model)
    q('fit-invalid-forecast-'+model.lower()+'-'+ident.lower(),'STAT',COMMANDS[model]['label']+': invalid forecast argument',stat_data(points)+[select(model),'Enter '+keys(value)+'.',select(ident)],error(),[ident],focus='domain',reference='contract')
q('fit-flat-inverse','STAT','A flat line has no unique inverse',stat_data([('1','2'),('2','2'),('3','2')])+[select('CFIT_LINEAR'),'Enter 2.',select('CFIT_FORECAST_X')],error('Err: Stat'),['CFIT_FORECAST_X'],focus='domain',reference='contract')
q('fit-flat-correlation','STAT','Correlation is undefined for constant y',stat_data([('1','2'),('2','2'),('3','2')])+[select('CFIT_LINEAR'),select('CFIT_CORRELATION')],error('Err: Stat'),['CFIT_CORRELATION'],focus='domain',reference='contract')
q('fit-best-degenerate','STAT','BEST rejects identical data points',stat_data([('2','3'),('2','3')])+[select('CFIT_BEST')],error('Err: Stat'),['CFIT_BEST'],focus='domain',reference='contract')
for model in ['CFIT_LOG','CFIT_EXP','CFIT_POWER']:
    q('fit-linear-only-'+model.lower(),'STAT','LINΣ excludes '+COMMANDS[model]['label'],stat_data([('1','2'),('2','4')],True)+[select(model)],error('Err: Stat'),[model,'STAT_LINEAR_MODE'],focus='domain',reference='contract')

# TVM: solve every unknown, rate and timing boundaries, compounding, and failures.
def tvm_steps(values,target,py='1',cy='1',begin=False,guesses=None,iterations=None):
    steps=[select('TVM_CLEAR'),select('TVM_SOLVER_RESET'),'Enter '+keys(py)+'.',select('TVM_PYR'),'Enter '+keys(cy)+'.',select('TVM_CYR'),select('TVM_BEGIN' if begin else 'TVM_END')]
    if guesses:
        for ident,value in zip(['TVM_IYR0','TVM_IYR1'],guesses): steps+=['Enter '+keys(value)+'.',select(ident)]
    if iterations is not None: steps+=['Enter '+str(iterations)+'.',select('TVM_ITER_MAX')]
    for ident in ['TVM_N','TVM_IYR','TVM_PV','TVM_PMT','TVM_FV']:
        if ident in values: steps+=['Enter '+keys(values[ident])+'.',select(ident)]
    return steps+[select(target)]

def tvm_pmt(n,annual,pv,fv,py=1,cy=1,begin=False):
    n,annual,pv,fv,py,cy=map(decimal,[n,annual,pv,fv,py,cy])
    i=mp.expm1((cy/py)*mp.log1p(annual/(100*cy)))
    v=mp.power(1+i,-n); a=n if not i else -mp.expm1(-n*mp.log1p(i))/i
    b=1+i if begin else 1
    return -(pv+fv*v)/(b*a)

for i,(n,rate,pv,fv,py,cy,begin) in enumerate([
 ('1','5','1000','0','1','1',False),('360','6.5','200000','0','12','12',False),
 ('360','6.5','200000','0','12','12',True),('10','-5','1000','0','1','1',False),
 ('10','1E-10','1000','0','1','1',False),('10','-1E-10','1000','0','1','1',False),
 ('24','6','10000','2000','12','4',False),('24','6','10000','2000','12','4',True),
 ('1000','1','1000','0','1','1',False),('10','5','1E80','0','1','1',False),('10','5','1E-80','0','1','1',False)
]):
    values={'TVM_N':n,'TVM_IYR':rate,'TVM_PV':pv,'TVM_FV':fv}
    q('tvm-payment-'+str(i),'TVM','PMT: '+n+' periods, '+rate+'% annual'+(', BEG' if begin else ', END'),tvm_steps(values,'TVM_PMT',py,cy,begin),number(tvm_pmt(n,rate,pv,fv,py,cy,begin),relative='2E-13' if abs(decimal(rate))<mp.mpf('1E-8') else '5E-11'),['TVM_PMT','TVM_BEGIN' if begin else 'TVM_END','TVM_PYR','TVM_CYR'],reference='tvm',cashFlows=values)
for i,(target,values,want) in enumerate([
 ('TVM_N',{'TVM_IYR':'0','TVM_PV':'1000','TVM_PMT':'-100','TVM_FV':'0'},10),
 ('TVM_N',{'TVM_IYR':'100','TVM_PV':'-1000','TVM_PMT':'0','TVM_FV':'2048000'},11),
 ('TVM_N',{'TVM_IYR':'300','TVM_PV':'-100','TVM_PMT':'0','TVM_FV':'200'},'.5'),
 ('TVM_PV',{'TVM_N':'10','TVM_IYR':'0','TVM_PMT':'-100','TVM_FV':'0'},1000),
 ('TVM_PV',{'TVM_N':'1','TVM_IYR':'10','TVM_PMT':'0','TVM_FV':'1100'},-1000),
 ('TVM_FV',{'TVM_N':'0','TVM_IYR':'5','TVM_PV':'-1000','TVM_PMT':'100'},1000),
 ('TVM_FV',{'TVM_N':'10','TVM_IYR':'0','TVM_PV':'-1000','TVM_PMT':'-100'},2000),
 ('TVM_FV',{'TVM_N':'10','TVM_IYR':'-5','TVM_PV':'-1000','TVM_PMT':'0'},1000*mp.mpf('.95')**10),
 ('TVM_FV',{'TVM_N':'10','TVM_IYR':'5','TVM_PV':'-1000','TVM_PMT':'0'},1000*mp.mpf('1.05')**10)
]):
    q('tvm-unknown-'+str(i),'TVM',COMMANDS[target]['label']+': solve the unknown',tvm_steps(values,target),number(want),[target],reference='tvm',cashFlows=values)
for i,(n,pv,fv,want,guesses) in enumerate([
 ('1','-1000','1100',10,None),('1','-1000','950',-5,('-20','0')),
 ('1','-100','1',-99,('-99.5','-90')),('1','-1E80','1.05E80',5,None),('1','-1E-80','1.05E-80',5,None),
 ('2','-1000','1210',10,None)
]):
    values={'TVM_N':n,'TVM_PV':pv,'TVM_PMT':'0','TVM_FV':fv}
    q('tvm-rate-'+str(i),'TVM','I%YR: '+str(want)+'% defining cash flow',tvm_steps(values,'TVM_IYR',guesses=guesses),number(want,relative='5E-9'),['TVM_IYR'],reference='Independent relation FV = -PV (1+I/100)^N with no payments; unique admissible real rate. A 5E-9 relative allowance is used for iterative rate solving.',cashFlows=values)
for ident in ['TVM_N','TVM_IYR','TVM_PV','TVM_PMT','TVM_FV','TVM_PYR','TVM_CYR']:
    value='123.45' if ident not in ['TVM_PYR','TVM_CYR'] else '4'
    q('tvm-recall-'+ident.lower(),'TVM',COMMANDS[ident]['label']+': independent store and recall',
      [select('TVM_CLEAR'),'Enter '+value+'.',select(ident),'Press CLEAR to remove X.', 'Reopen the same TVM menu row; press 2ND and the softkey for '+COMMANDS[ident]['label']+'.'],number(value,exact=True),[ident],focus='interaction',reference='integer')
for ident in ['TVM_PYR','TVM_CYR']:
    for i,value in enumerate(['0','-1']): fn('tvm-period-domain-'+ident.lower()+str(i),ident,[value],error(),focus='domain',reference='contract')
for i,(values,target,expect) in enumerate([
 ({'TVM_N':'0','TVM_IYR':'5','TVM_PV':'1000','TVM_FV':'0'},'TVM_PMT','Err: Division By 0'),
 ({'TVM_IYR':'0','TVM_PV':'100','TVM_PMT':'10','TVM_FV':'0'},'TVM_N','TVM Not Found'),
 ({'TVM_N':'10','TVM_IYR':'-100','TVM_PV':'1000','TVM_FV':'0'},'TVM_PMT','Err: Domain'),
 ({'TVM_N':'10','TVM_IYR':'-100.1','TVM_PV':'1000','TVM_FV':'0'},'TVM_PMT','Err: Domain')
]):
    q('tvm-failure-'+str(i),'TVM','TVM rejects an undefined or unsolvable setup',tvm_steps(values,target),error(expect),[target],focus='domain',reference='contract',cashFlows=values)
q('tvm-iteration-limit','TVM','I%YR reports failure when iterations are exhausted',tvm_steps({'TVM_N':'360','TVM_PV':'200000','TVM_PMT':'-1264.13604699','TVM_FV':'0'},'TVM_IYR',py='12',cy='12',iterations=1),error('TVM Not Found'),['TVM_IYR','TVM_ITER_MAX'],focus='domain',reference='contract')
for ident in ['TVM_N','TVM_IYR','TVM_PV','TVM_PMT','TVM_FV','TVM_PYR','TVM_CYR','TVM_IYR0','TVM_IYR1','TVM_ITER_MAX']:
    q('tvm-unit-'+ident.lower(),'TVM',COMMANDS[ident]['label']+' rejects a quantity',[select('TVM_CLEAR'),'Enter 1.',select('UNIT_METER'),select(ident)],error('Err: Data Type'),[ident],focus='domain',reference='contract')

# DATE: Gregorian boundaries, negative epochs, both cast outputs, and bad types.
def record(ident,fields,enter=True):
    rendered=', '.join(str(abs(int(v)))+(' (−)' if int(v)<0 else '') for v in fields)
    return select(ident)+f' Type {rendered}'+(' ENTER.' if enter else '. Do not press ENTER; the following operation finishes the record.')+' Commas separate the fields.'

def raw(kind,fields):
    return kind+'{'+','.join(str(v) for v in fields)+'}'

creators={
 'DATE_CREATE':('D',[[1,1,1],[9999,12,31],[2000,2,29]]),
 'TIME_CREATE':('T',[[0,0,0],[23,59,59]]),
 'DATE_TIME_CREATE':('DT',[[1,1,1,0,0,0],[9999,12,31,23,59,59],[2000,2,29,12,0,0]]),
 'OFFSET_CREATE':('TZ',[[-23,-45],[23,45],[0,-15]]),
 'OFFSET_DATE_TIME_CREATE':('DZ',[[2024,2,29,23,59,59,23,45],[2024,2,29,0,0,0,-23,-45]]),
 'DURATION_CREATE':('DR',[[0,0,0,0],[9999,23,59,59],[-9999,-23,-59,-59]]),
 'DAY_OF_WEEK_CREATE':('DW',[[1],[7]])
}
for ident,(kind,examples) in creators.items():
    for i,fields in enumerate(examples): q('record-limit-'+ident.lower()+str(i),'DATE','Create '+raw(kind,fields),[record(ident,fields)],text(raw(kind,fields)),[ident],reference='date')
for ident,examples in {
 'DATE_CREATE':[[0,1,1],[10000,1,1],[1900,2,29],[2024,4,31],[2024,0,1],[2024,13,1],[2024,1,0]],
 'TIME_CREATE':[[24,0,0],[23,60,0],[23,59,60],[-1,0,0]],
 'DATE_TIME_CREATE':[[2023,2,29,0,0,0],[2024,1,1,24,0,0]],
 'OFFSET_CREATE':[[24,0],[0,14],[-7,30]],
 'OFFSET_DATE_TIME_CREATE':[[2024,2,30,0,0,0,0,0],[2024,1,1,0,0,0,-7,30]],
 'DURATION_CREATE':[[10000,0,0,0],[0,24,0,0],[1,-1,0,0],[0,0,60,0]],
 'DAY_OF_WEEK_CREATE':[[0],[8]]
}.items():
    for i,fields in enumerate(examples): q('record-domain-'+ident.lower()+str(i),'DATE',COMMANDS[ident]['label']+': invalid fields',
      [record(ident,fields),'Note the message before CLEAR cancels the invalid record.'],error('Err: Invalid'),[ident],focus='domain',reference='contract')
for leap in ['IS_DATE_LEAP','IS_DATE_TIME_LEAP','IS_OFFSET_DATE_TIME_LEAP']:
    for year in [1,1900,2000,2100,2400,9999]: fn('leap-'+leap.lower()+str(year),leap,[year],number(int(year%4==0 and (year%100!=0 or year%400==0)),exact=True),reference='date')
    for i,value in enumerate([0,10000,'2024.5']): fn('leap-domain-'+leap.lower()+str(i),leap,[value],error(),focus='domain',reference='contract')
for fields,amount in [([2024,2,28],1),([2023,2,28],1),([2000,2,28],1),([1900,2,28],1),([2024,12,31],1),([2024,3,1],-1),([1987,1,17],15000)]:
    result=date(*fields)+timedelta(days=amount)
    q('calendar-add-'+str(fields[0])+'-'+str(fields[1])+'-'+str(amount),'DATE',raw('D',fields)+f' + {amount} days',
      [record('DATE_CREATE',fields),'Enter '+keys(amount)+' +.'],text(raw('D',[result.year,result.month,result.day])),focus='boundary',reference='date')
for fields,seconds in [([2024,2,28,23,59,59],1),([2024,12,31,23,59,59],1),([2024,3,1,0,0,0],-1)]:
    result=datetime(*fields)+timedelta(seconds=seconds)
    q('datetime-roll-'+str(fields[1])+'-'+str(seconds),'DATE','DateTime crosses midnight',
      [record('DATE_TIME_CREATE',fields),'Enter '+keys(seconds)+' +.'],text(raw('DT',[result.year,result.month,result.day,result.hour,result.minute,result.second])),reference='date')
for i,(fields,seconds,want) in enumerate([([0,0,0],-1,[23,59,59]),([23,59,59],1,[0,0,0]),([12,0,0],86400,[12,0,0])]):
    q('time-wrap-'+str(i),'DATE','Time wrap: '+str(seconds)+' seconds',[record('TIME_CREATE',fields),'Enter '+keys(seconds)+' +.'],text(raw('T',want)),reference='date')
for ident in ['DATE_TO_EPOCH_DAYS','DATE_TO_EPOCH_SECONDS']:
    for i,fields in enumerate([[1,1,1],[1969,12,31],[1970,1,1],[9999,12,31]]):
        delta=(date(*fields)-date(1970,1,1)).days
        q('date-epoch-boundary-'+ident.lower()+str(i),'DATE',COMMANDS[ident]['label']+' at '+raw('D',fields),[select('EPOCH_UNIX'),record('DATE_CREATE',fields),select(ident)],number(delta*(86400 if ident.endswith('SECONDS') else 1),exact=True),[ident],reference='date')
for ident,kind,create in [('DATE_TIME_TO_EPOCH_SECONDS','DT','DATE_TIME_CREATE'),('OFFSET_DATE_TIME_TO_EPOCH_SECONDS','DZ','OFFSET_DATE_TIME_CREATE')]:
    for i,fields in enumerate([[1,1,1,0,0,0],[1969,12,31,23,59,59],[9999,12,31,23,59,59]]):
        delta=datetime(*fields)-datetime(1970,1,1); seconds=delta.days*86400+delta.seconds
        actual=fields+([0,0] if kind=='DZ' else [])
        q('datetime-epoch-boundary-'+ident.lower()+str(i),'DATE',COMMANDS[ident]['label']+' at '+raw(kind,actual),[select('EPOCH_UNIX'),record(create,actual),select(ident)],number(seconds,exact=True),[ident],reference='date')
for ident,kind,unit in [('EPOCH_DAYS_TO_DATE','D','day'),('EPOCH_DAYS_TO_DATE_TIME','DT','day'),('EPOCH_DAYS_TO_OFFSET_DATE_TIME','DZ','day'),
                       ('EPOCH_SECONDS_TO_DATE','D','second'),('EPOCH_SECONDS_TO_DATE_TIME','DT','second'),('EPOCH_SECONDS_TO_OFFSET_DATE_TIME_UTC','DZ','second')]:
    for i,value in enumerate([-1,0,1]):
        dt=datetime(1970,1,1)+(timedelta(days=value) if unit=='day' else timedelta(seconds=value))
        fields=[dt.year,dt.month,dt.day]+([dt.hour,dt.minute,dt.second] if kind!='D' else [])+([0,0] if kind=='DZ' else [])
        fn('epoch-inverse-'+ident.lower()+str(i),ident,[value],text(raw(kind,fields)),setup=[select('EPOCH_UNIX')],reference='date')
    fn('epoch-fraction-'+ident.lower(),ident,['1.5'],error(),focus='domain',setup=[select('EPOCH_UNIX')],reference='contract')
for ident in ['DATE_TO_DAY_OF_WEEK','DATE_TIME_TO_DAY_OF_WEEK','OFFSET_DATE_TIME_TO_DAY_OF_WEEK']:
    create='DATE_CREATE' if ident.startswith('DATE_TO') else 'DATE_TIME_CREATE' if ident.startswith('DATE_TIME') else 'OFFSET_DATE_TIME_CREATE'
    fields=[1970,1,1]+([] if create=='DATE_CREATE' else [0,0,0])+([0,0] if create=='OFFSET_DATE_TIME_CREATE' else [])
    q('weekday-epoch-'+ident.lower(),'DATE','Unix epoch is Thursday', [record(create,fields),select(ident)],text('DW{4}'),[ident],reference='date')
for ident,field_index,create,fields in [
 ('DATE_EXTRACT_YEAR',0,'DATE_CREATE',[9999,12,31]),('DATE_EXTRACT_MONTH',1,'DATE_CREATE',[9999,12,31]),('DATE_EXTRACT_DAY',2,'DATE_CREATE',[9999,12,31]),
 ('TIME_EXTRACT_HOUR',0,'TIME_CREATE',[23,59,59]),('TIME_EXTRACT_MINUTE',1,'TIME_CREATE',[23,59,59]),('TIME_EXTRACT_SECOND',2,'TIME_CREATE',[23,59,59]),
 ('DURATION_EXTRACT_DAY',0,'DURATION_CREATE',[-9999,-23,-59,-59]),('DURATION_EXTRACT_HOUR',1,'DURATION_CREATE',[-9999,-23,-59,-59]),
 ('DURATION_EXTRACT_MINUTE',2,'DURATION_CREATE',[-9999,-23,-59,-59]),('DURATION_EXTRACT_SECOND',3,'DURATION_CREATE',[-9999,-23,-59,-59]),
 ('OFFSET_EXTRACT_HOUR',0,'OFFSET_CREATE',[-23,-45]),('OFFSET_EXTRACT_MINUTE',1,'OFFSET_CREATE',[-23,-45])]:
    q('component-boundary-'+ident.lower(),'DATE',COMMANDS[ident]['label']+' at the signed/range boundary', [record(create,fields),select(ident)],number(fields[field_index],exact=True),[ident],reference='integer')
for ident,amount in [('SECONDS_TO_TIME',86399),('DAYS_TO_DURATION',9999),('HOURS_TO_DURATION',239999),('MINUTES_TO_DURATION',14399999),('SECONDS_TO_DURATION',863999999)]:
    if ident=='SECONDS_TO_TIME': expected=text('T{23,59,59}')
    else:
        sec=amount*({'DAYS_TO_DURATION':86400,'HOURS_TO_DURATION':3600,'MINUTES_TO_DURATION':60,'SECONDS_TO_DURATION':1}[ident])
        d,rem=divmod(sec,86400);h,rem=divmod(rem,3600);m,s=divmod(rem,60);expected=text(raw('DR',[d,h,m,s]))
    fn('duration-limit-'+ident.lower(),ident,[amount],expected,reference='date')
    fn('duration-fraction-'+ident.lower(),ident,['1.5'],error(),focus='domain',reference='contract')
for ident,value in [('SECONDS_TO_TIME',-1),('SECONDS_TO_TIME',86400),('DAYS_TO_DURATION',10000),('SECONDS_TO_DURATION',864000000)]:
    fn('duration-domain-'+ident.lower()+str(value),ident,[value],error(),focus='domain',reference='contract')
q('duration-negative-seconds','DATE','Convert negative seconds to a Duration',['Enter 93784 (−).',select('SECONDS_TO_DURATION')],text('DR{-1,-2,-3,-4}'),['SECONDS_TO_DURATION'],reference='date')
q('duration-signed-components','DATE','Negative Duration to seconds',[record('DURATION_CREATE',[-1,-2,-3,-4]),select('DURATION_TO_SECONDS')],number(-93784,exact=True),['DURATION_TO_SECONDS'],reference='date')
for ident in ['TIME_TO_SECONDS','DURATION_TO_SECONDS','OFFSET_TO_HOURS','DAY_OF_WEEK_TO_ISO_DOW_NUMBER','DAY_OF_WEEK_TO_UNIX_DOW_NUMBER']:
    fn('date-type-'+ident.lower(),ident,[0],error('Err: Data Type'),focus='domain',reference='contract')
for ident,value,want in [('HOURS_TO_OFFSET','-23.75','TZ{-23,-45}'),('HOURS_TO_OFFSET','23.75','TZ{23,45}'),('HOURS_TO_OFFSET','-.25','TZ{0,-15}'),('ISO_DOW_NUMBER_TO_DAY_OF_WEEK','1','DW{1}'),('UNIX_DOW_NUMBER_TO_DAY_OF_WEEK','6','DW{6}'),('UNIX_DOW_NUMBER_TO_DAY_OF_WEEK','0','DW{7}')]:
    fn('date-number-boundary-'+ident.lower()+value,ident,[value],text(want),reference='date')
for ident,values in [('HOURS_TO_OFFSET',['24','-24','.1']),('ISO_DOW_NUMBER_TO_DAY_OF_WEEK',['0','8','1.5']),('UNIX_DOW_NUMBER_TO_DAY_OF_WEEK',['-1','7','.5'])]:
    for i,v in enumerate(values): fn('date-number-domain-'+ident.lower()+str(i),ident,[v],error(),focus='domain',reference='contract')
for ident in list(COMMANDS):
    if COMMANDS[ident]['section']!='DATE': continue
    if ident in ['ISO_DOW_NUMBER_TO_DAY_OF_WEEK','UNIX_DOW_NUMBER_TO_DAY_OF_WEEK']:
        q('date-wrong-type-'+ident.lower(),'DATE',COMMANDS[ident]['label']+' rejects a Date object',
          [record('DATE_CREATE',[2024,3,14]),select(ident)],error('Err: Data Type'),[ident],focus='domain',reference='contract')
    elif ('EXTRACT_' in ident or '_SHRINK_' in ident or '_EXTEND_' in ident or '_CUT_' in ident or '_LINK_' in ident or ident.endswith('TO_EPOCH_DAYS') or ident.endswith('TO_EPOCH_SECONDS') or ident.endswith('TO_DAY_OF_WEEK') or ident.endswith('CONVERT_TO_TIME_ZONE') or ident=='EPOCH_SET_CUSTOM'):
        fn('date-wrong-type-'+ident.lower(),ident,[0],error('Err: Data Type'),focus='domain',reference='contract')
for i,(fields,offset) in enumerate([([2024,12,31,23,30,0,-7,0],9),([2024,1,1,0,30,0,9,0],-7)]):
    tz=timezone(timedelta(hours=fields[6],minutes=fields[7])); dt=datetime(*fields[:6],tzinfo=tz).astimezone(timezone(timedelta(hours=offset)))
    q('zone-new-year-'+str(i),'DATE','CVTZ crosses the year boundary',[record('OFFSET_DATE_TIME_CREATE',fields),'Enter '+keys(offset)+'.',select('OFFSET_DATE_TIME_CONVERT_TO_TIME_ZONE')],text(raw('DZ',[dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second,offset,0])),['OFFSET_DATE_TIME_CONVERT_TO_TIME_ZONE'],reference='date')
q('date-cut-second-output','DATE','DCUT: verify the Date output',[record('DATE_TIME_CREATE',[9999,12,31,23,59,59]),select('DATE_TIME_CUT_TO_DATE'),'Press ) (X↔Y).'],text('D{9999,12,31}'),['DATE_TIME_CUT_TO_DATE'],reference='date')
q('zoned-cut-second-output','DATE','DCUT: verify the DateTime output',[record('OFFSET_DATE_TIME_CREATE',[2024,12,31,23,59,59,-7,0]),select('OFFSET_DATE_TIME_CUT_TO_DATE_TIME'),'Press ) (X↔Y).'],text('DT{2024,12,31,23,59,59}'),['OFFSET_DATE_TIME_CUT_TO_DATE_TIME'],reference='date')
for ident in ['SET_APP_TIME_ZONE','SET_CLOCK_TIME_ZONE']:
    fn('clock-zone-quarter-'+ident.lower(),ident,['.1'],error(),focus='domain',reference='contract')
    fn('clock-zone-range-'+ident.lower(),ident,[24],error(),focus='domain',reference='contract')
    getter='GET_APP_TIME_ZONE' if ident=='SET_APP_TIME_ZONE' else 'GET_CLOCK_TIME_ZONE'
    q('clock-zone-min-'+ident.lower(),'DATE',COMMANDS[ident]['label']+': negative quarter-hour offset',
      ['Write your original '+COMMANDS[getter]['label']+' value in Notes before changing it.','Enter 5.75 (−).',select(ident),select(getter),'After recording, restore your original zone setting.'],text('TZ{-5,-45}'),[ident,getter],reference='date')

# Registers and stack bounds, prompt priority, retained metadata and persistence.
for ident,getter,values in [('SET_STACK_SIZE','GET_STACK_SIZE',[(-1,4),(3,4),(4,4),(12,12)]),('SET_REG_SIZE','GET_REG_SIZE',[(-1,40),(39,40),(40,40),(140,140)])]:
    for value,want in values:
        q('size-limit-'+ident.lower()+str(value),'MODE',COMMANDS[ident]['label']+': '+str(value),['Enter '+keys(value)+'.',select(ident),select(getter)],number(want,exact=True),[ident,getter],reference='contract')
    for i,value in enumerate([13,'4.5'] if ident=='SET_STACK_SIZE' else [141,'40.5']): fn('size-domain-'+ident.lower()+str(i),ident,[value],error(),focus='domain',reference='contract')
    q('size-unit-'+ident.lower(),'MODE',COMMANDS[ident]['label']+' rejects a quantity',['Enter 4.',select('UNIT_METER'),select(ident)],error('Err: Data Type'),[ident],focus='domain',reference='contract')
for ident,answer in [('STACK_DUP',12),('STACK_ROLL_UP',1),('STACK_ROLL_DOWN',11),('STACK_DROP',11),('STACK_EXCHANGE_XY',11)]:
    q('stack-12-'+ident.lower(),'STK',COMMANDS[ident]['label']+' with a full 12-level stack',['Enter 12.',select('SET_STACK_SIZE'),'Press CLEAR twice. Enter '+' ENTER '.join(map(str,range(1,13)))+'.',select(ident)],number(answer,exact=True),[ident],reference='integer')
q('stack-12-top-copy','STK','Repeated arithmetic drops across all 12 levels',['Enter 12.',select('SET_STACK_SIZE'),'Press CLEAR twice. Enter '+' ENTER '.join(map(str,range(1,13)))+'.','Press + eleven times to sum the stack, then press + once more to use the repeated top level.'],number(79,exact=True),reference='integer')
q('clear-stack-12','CLR','CLST clears hidden stack levels',['Enter 12.',select('SET_STACK_SIZE'),'Enter '+' ENTER '.join(map(str,range(1,13)))+'.',select('CLEAR_STACK'),'Press ( twelve times, inspecting X after every roll.'],number(0,exact=True),['CLEAR_STACK'],checks=['Every roll showed zero; none of the hidden values returned.'],reference='integer')
for name,key,want in [('add','+',125),('subtract','−',75),('multiply','×',2500),('divide','÷',4)]:
    q('store-arithmetic-'+name,'Registers','STO '+key+' A', ['This replaces A. Enter 100 STO→ MATH. Enter 25 STO→ '+key+' MATH. Press CLEAR, then 2ND STO→ MATH.'],number(want,exact=True),focus='interaction',reference='integer')
    want={'add':125,'subtract':-75,'multiply':2500,'divide':mp.mpf('.25')}[name]
    q('recall-arithmetic-'+name,'Registers','RCL '+key+' A', ['This replaces A. Enter 100 STO→ MATH. Enter 25; press 2ND STO→ '+key+' MATH.'],number(want,exact=True),focus='interaction',reference='integer')
q('register-max-index','Registers','Highest extra register 140', ['This replaces i and R140. Enter 140.',select('SET_REG_SIZE'),'Enter 140 STO→ .; enter 456 STO→ 2ND .; press CLEAR; press 2ND STO→ 2ND .'],number(456,exact=True),reference='integer')
q('register-invalid-index','Registers','Indirect index beyond register 140', ['Enter 140.',select('SET_REG_SIZE'),'Enter 141 STO→ .; press 2ND STO→ 2ND .'],error('Err: Index'),focus='domain',reference='contract')
q('register-truncated-index','Registers','Indirect addressing uses the integer part', ['This replaces A and i. Enter 77 STO→ MATH. Enter 1.9 STO→ .; press CLEAR; press 2ND STO→ 2ND .'],number(77,exact=True),reference='integer')
q('register-unit-index','Registers','A quantity cannot be an indirect index', ['This replaces i. Enter 1.',select('UNIT_METER'),'Press STO→ .; then 2ND STO→ 2ND .'],error('Err: Data Type'),focus='domain',reference='contract')
q('register-zero-unit-vars','Registers','Zero with a unit remains visible in VARS',['This replaces A. Enter 0.',select('UNIT_FOOT'),'Press STO→ MATH. Open VARS and locate A. Select it with ENTER.'],number(0,exact=True),focus='interaction',checks=['A was listed even though its magnitude is zero.','The recalled value still carries ft.'],reference='integer')
for cancel in ['CLEAR','DEL','ON']:
    q('prompt-cancel-'+cancel.lower(),'Registers','Cancel STO using '+cancel,['Enter 42 STO→ MATH. Enter 17 STO→; cancel the prompt with '+cancel+'. Press CLEAR, then 2ND STO→ MATH.'],number(42,exact=True),focus='interaction',reference='integer')
for ident,kind in [('STACK_EXCHANGE_REG','quantity'),('STACK_EXCHANGE_REG','date')]:
    steps=['This replaces A. Enter 17 STO→ MATH.']
    steps+=['Enter 12.',select('UNIT_FOOT')] if kind=='quantity' else [record('DATE_CREATE',[2024,2,29])]
    steps+=[select(ident),'At the register prompt press MATH (A). Check X, then recall A using 2ND STO→ MATH.']
    q('exchange-metadata-'+kind,'Registers','X<> preserves a '+kind,steps,number(12,exact=True) if kind=='quantity' else text('D{2024,2,29}'),[ident],focus='interaction',checks=['X became 17 immediately after exchanging.']+(['A recalls with unit ft.'] if kind=='quantity' else []),reference='integer')
for ident in ['ROUND_TO_N','ROUND_TO_GUARD']:
    q('round-prompt-cancel-'+ident.lower(),'NUM',COMMANDS[ident]['label']+': cancel the prompt',['Enter 2.675.',select(ident),'Press ON to cancel the digit prompt.'],number('2.675',exact=True),[ident],focus='interaction',reference='integer')
for ident in ['FIX','SCI','ENG']:
    for digits in [0,11,12]:
        prompt=str(digits) if digits<10 else '. then '+str(digits-10)
        q('display-precision-'+ident.lower()+str(digits),'MODE',COMMANDS[ident]['label']+': '+str(digits)+' digits', ['Enter 1 ENTER 7 ÷.',select(ident),'At the prompt press '+prompt+'.','Inspect the displayed format, then use SHOW for the stored value.'],number(mp.mpf(1)/7),[ident],focus='boundary',checks=['The selected format and digit setting appear in the status bar; changing precision has not rounded the stored value.'])
q('decimal-scientific-entry','MODE','Comma decimal with a negative exponent',[select('DECIMAL_COMMA'),'Type 1.25 , (−) 9 ENTER using the physical decimal and comma/EE keys.','Use SHOW. Enter the answer using the answer-notation selector that matches the text you type here.'],number('1.25E-9',exact=True),['DECIMAL_COMMA'],focus='interaction',checks=['Entry and SHOW use a comma in the mantissa; the exponent is still −9.'],reference='integer')
q('decimal-date-fields','MODE','Comma decimal does not change DATE field separators',[select('DECIMAL_COMMA'),record('DATE_CREATE',[2024,2,29])],text('D{2024,2,29}'),['DECIMAL_COMMA','DATE_CREATE'],focus='interaction',reference='date')
for i,exitkey in enumerate(['ENTER','CLEAR','DEL','ON']):
    q('help-exit-'+str(i),'HELP','Leave HELP with '+exitkey,['Enter 123.456.',select('HELP'),'Use ↑/↓ to view several pages. Press '+exitkey+' to return.'],number('123.456',exact=True),['HELP'],focus='interaction',reference='integer')
q('long-base-scroll','BASE','Scroll all 44 binary digits',['Enter 44.',select('SET_WORD_SIZE')]+enter_word((1<<44)-1)+[select('BIN'),'Use ←/→ until you have inspected both ends of X. Return to DEC.',select('DEC')],number((1<<44)-1,exact=True),['BIN','DEC'],focus='interaction',checks=['All 44 digits were 1; scrolling remained on one stack line and the continuation arrow was visible.'],reference='integer')
q('persist-numeric-extremes','Persistence','Save and recall both exponent limits',[
 'This replaces A and B. Enter 1 EE (−) 99 STO→ MATH. Enter 1 EE 99 STO→ APPS.',
 'Quit with 2ND MODE and relaunch the same build without deleting appvars. Recall A, then B; use SHOW for each.'
],checks=['A is exactly 1E-99 and B is exactly 1E99 after relaunch.','Neither value has lost its sign, exponent, or digits.'],focus='interaction',reference='integer')
q('persist-twelve-stack','Persistence','Save a 12-level stack',[
 'Enter 12.',select('SET_STACK_SIZE'),'Press CLEAR twice. Enter '+' ENTER '.join(map(str,range(1,13)))+'.',
 'Quit with 2ND MODE and relaunch. Press ( repeatedly to inspect all 12 levels.'
],checks=['The stack depth remains 12.','Before rolling X is 12; twelve rolls show 11,10,9,8,7,6,5,4,3,2,1,12 in that order.'],focus='interaction',reference='integer')
q('idle-save','Persistence','Idle timeout saves and exits',[
 'This takes at least five minutes. Enter 123 STO→ MATH. Leave the calculator without pressing keys for slightly more than five minutes.',
 'Reopen rpnCE and recall A using 2ND STO→ MATH.'
],number(123,exact=True),focus='interaction',checks=['The application saved and exited after the idle interval.'],reference='contract')

for ident in ['DEC','HEX','OCT','BIN']:
    digits={'DEC':str((1<<44)-1),'HEX':format((1<<44)-1,'X'),'OCT':format((1<<44)-1,'o'),'BIN':format((1<<44)-1,'b')}[ident]
    q('base-maximum-format-'+ident.lower(),'BASE',COMMANDS[ident]['label']+': the 44-bit maximum', ['Enter 44.',select('SET_WORD_SIZE')]+enter_word((1<<44)-1)+[select(ident),'Inspect all digits using left/right scrolling as needed.',select('DEC')],number((1<<44)-1,exact=True),[ident],checks=['Before returning to DEC, the digits were '+digits+'.'],reference='integer')
q('stat-count-first','STAT','Σ+ counts the first point',stat_data([('1','2')]),number(1,exact=True),['STAT_PLUS'],reference='integer')
q('stat-clear-populated','STAT','CLΣ resets a populated dataset',stat_data([('1','2'),('3','4')])+[select('STAT_CLEAR'),select('STAT_N')],number(0,exact=True),['STAT_CLEAR','STAT_N'],reference='integer')
q('stat-switch-to-all','STAT','ALLΣ cannot reconstruct missing logarithmic sums',stat_data([('1','2')],True)+[select('STAT_ALL_MODE'),select('CFIT_LOG')],error('Err: Stat'),['STAT_ALL_MODE','STAT_LINEAR_MODE'],focus='domain',reference='contract')
q('tvm-clear-settings','TVM','CLTV resets nondefault Finance settings', ['Enter 4.',select('TVM_PYR'),'Enter 2.',select('TVM_CYR'),select('TVM_BEGIN'),'Enter 999.',select('TVM_PV'),select('TVM_CLEAR'),'Inspect the Finance entries in VARS and the TVM timing marker.'],covers=['TVM_CLEAR'],checks=['N, I%YR, PV, PMT and FV are zero; P/YR and C/YR are 12.','END is selected.'],reference='contract')
q('tvm-reset-guesses','TVM','RSTV resets nondefault solver parameters', ['Enter 2.',select('TVM_IYR0'),'Enter 20.',select('TVM_IYR1'),'Enter 255.',select('TVM_ITER_MAX'),select('TVM_SOLVER_RESET'),'Use 2ND with each settings softkey to recall its value.'],covers=['TVM_IYR0','TVM_IYR1','TVM_ITER_MAX','TVM_SOLVER_RESET'],checks=['Recalls give IYR1 = 0, IYR2 = 100, and TMAX = 50.'],reference='contract')
for i,v in enumerate([0,256,'1.5']): fn('tvm-tmax-domain-'+str(i),'TVM_ITER_MAX',[v],error(),focus='domain',reference='contract')
for ident,inputs in [('SQUARE',['1E50']),('ALOG',['100']),('EXP',['231'])]: keyfn('range-overflow-'+ident.lower(),ident,inputs,error('Err: Overflow'),focus='domain',reference='contract')
for ident,v in [('CUBE','1E34'),('TWO_POW','333'),('SINH','231'),('COSH','231')]: fn('range-overflow-'+ident.lower(),ident,[v],error('Err: Overflow'),focus='domain',reference='contract')
for ident,inputs in [('RECIP',['9E99']),('SQUARE',['1E-50']),('ALOG',['-100']),('EXP',['-230']),('POW',['1E-50','2'])]:
    keyfn('range-underflow-'+ident.lower(),ident,inputs,error('Err: Underflow'),focus='domain',knownIssue=ident=='ALOG',reference='The exact nonzero result has magnitude below 1E-99. Require Err: Underflow and preserved operands; zero is not an acceptable answer.')
keyfn('range-overflow-pow','POW',['1E50','2'],error('Err: Overflow'),focus='domain',reference='The exact result is 1E100, outside the supported range. Require Err: Overflow and preserved operands.')
for ident,inputs in [('CUBE',['1E-34']),('TWO_POW',['-333']),('PERCENT',['1E-50','1E-50'])]:
    fn('range-underflow-'+ident.lower(),ident,inputs,error('Err: Underflow'),focus='domain',knownIssue=ident=='TWO_POW',reference='The exact nonzero result has magnitude below 1E-99. Require Err: Underflow and preserved operands; zero is not an acceptable answer.')
q('clock-full-day-rollover','DATE','ATZ shifts the application by a full day', [
 'Record your original CTZ and ATZ in Notes. This changes only the application zone declarations, not the TI-OS clock.',
 'Enter 12 (−).',select('SET_CLOCK_TIME_ZONE'),'Enter 12 (−).',select('SET_APP_TIME_ZONE'),
 select('GET_NOW_OFFSET_DATE_TIME'),'Record this date and wall time in Notes. Enter 12.',select('SET_APP_TIME_ZONE'),select('GET_NOW_OFFSET_DATE_TIME'),
 'Compare the new date and wall time, then inspect NOWD, NOWT, NWUT and the status bar. Restore your original CTZ and ATZ when finished.'
],covers=['SET_CLOCK_TIME_ZONE','SET_APP_TIME_ZONE','GET_NOW_OFFSET_DATE_TIME','GET_NOW_DATE','GET_NOW_TIME','GET_NOW_OFFSET_DATE_TIME_UTC'],focus='interaction',checks=['Changing ATZ from −12 to +12 advanced the application date by exactly one day, preserving wall time apart from elapsed testing time.','NOWD, NOWT and the status bar agree with NWDZ. NWUT still represents the same advancing instant.','The TI-OS clock was unchanged.'],reference='date')

for ident in ['CLEAR_CARRY_FLAG','SET_CARRY_FLAG']:
    q('carry-reset-'+ident.lower(),'BASE',COMMANDS[ident]['label']+': change an existing flag state', [select('SET_CARRY_FLAG' if ident=='CLEAR_CARRY_FLAG' else 'CLEAR_CARRY_FLAG'),select(ident),select('GET_CARRY_FLAG')],number(0 if ident=='CLEAR_CARRY_FLAG' else 1,exact=True),[ident],reference='integer')
for epoch,origin in [('EPOCH_UNIX',date(1970,1,1)),('EPOCH_NTP',date(1900,1,1)),('EPOCH_GPS',date(1980,1,6)),('EPOCH_TIOS',date(1997,1,1)),('EPOCH_Y2K',date(2000,1,1))]:
    q('epoch-negative-'+epoch.lower(),'DATE',COMMANDS[epoch]['label']+': day before the origin', [select(epoch),record('DATE_CREATE',[(origin-timedelta(days=1)).year,(origin-timedelta(days=1)).month,(origin-timedelta(days=1)).day]),select('DATE_TO_EPOCH_SECONDS')],number(-86400,exact=True),[epoch],reference='date')
q('epoch-custom-boundary','DATE','Custom epoch at the latest supported date', [record('DATE_CREATE',[9999,12,31]),select('EPOCH_SET_CUSTOM'),select('EPOCH_CUSTOM'),select('EPOCH_GET_CUSTOM')],text('D{9999,12,31}'),['EPOCH_SET_CUSTOM','EPOCH_CUSTOM','EPOCH_GET_CUSTOM'],reference='date')
fn('epoch-app-fraction','EPOCH_SECONDS_TO_OFFSET_DATE_TIME_APP',['1.5'],error(),focus='domain',reference='contract')
fn('clock-set-wrong-type','SET_CLOCK',[0],error('Err: Data Type'),focus='domain',reference='contract')
q('clock-epoch-origin','DATE','NOW uses the selected epoch', [
 select('EPOCH_UNIX'),select('GET_NOW'),'Press ENTER.',select('EPOCH_Y2K'),select('GET_NOW'),'Press − within 60 seconds of the first NOW. Use SHOW for the difference.'
],{'kind':'range','min':'946684740','max':'946684801'},['GET_NOW'],focus='boundary',reference='The exact Unix/Y2K epoch difference is 946684800 seconds. The first reading minus the second is that difference minus the elapsed whole seconds; finish within 60 seconds.')
for i,(steps,want) in enumerate([
 (['Type 1234567890123 ENTER; the thirteenth mantissa digit should be ignored.'],123456789012),
 (['Type 1 , 9 9 9 ENTER; the third exponent digit should be ignored.'],'1E99'),
 (['Type 1 . 2 . 3 ENTER; the second decimal key should be ignored.'],'1.23')
]): q('entry-limit-'+str(i),'Keyboard','Numeric entry limit '+str(i+1),steps,number(want,exact=True),reference='contract')

q('clear-unit-x','CLR','CLX removes a value and its unit',['Enter 12.',select('UNIT_FOOT'),select('CLEAR_X')],number(0,exact=True),['CLEAR_X'],checks=['The zero result has no unit.'],reference='integer')
q('clear-extra-registers','CLR','CLRG clears extras and preserves statistics',['This replaces ordinary registers and i. Enter 140.',select('SET_REG_SIZE'),'Enter 140 STO→ .; enter 123 STO→ 2ND .; enter 77 STO→ MATH.']+stat_data([('1','2')])+[select('CLEAR_REGS'),'Enter 140 STO→ .; press CLEAR; then 2ND STO→ 2ND . Check that the extra register is zero.',select('STAT_N')],number(1,exact=True),['CLEAR_REGS'],checks=['R140 recalled zero after CLRG.'],reference='integer')
q('clear-stat-alias','CLR','CLR clears populated statistics',stat_data([('1','2'),('3','4')])+[select('CLEAR_STAT'),select('STAT_N')],number(0,exact=True),['CLEAR_STAT'],reference='integer')
q('clear-tvm-alias','CLR','CLR restores TVM defaults',[select('TVM_BEGIN'),'Enter 4.',select('TVM_PYR'),'Enter 2.',select('TVM_CYR'),'Enter 99.',select('TVM_PV'),select('CLEAR_TVM'),'Inspect the TVM values in VARS and the timing marker.'],covers=['CLEAR_TVM'],checks=['N, I%YR, PV, PMT and FV are zero; P/YR and C/YR are 12; END is selected.'],reference='contract')
q('clear-message-preserves-x','CLR','CLD preserves the stack after an error',['Open CLR row 2 BEFORE creating the error. Enter 1 ENTER 0 ÷. Note the error; press Y= (CLD) without navigating away.'],number(0,exact=True),['CLEAR_DISPLAY'],focus='interaction',checks=['The message cleared and Y still contains 1.'],reference='integer')
q('tint-full-cycle','MODE','TINT wraps after all eight presets',['Enter 123.456. Note the starting tint.',*([select('SCREEN_TINT')]*8),'Check that the original tint has returned. Use SHOW to inspect the number.'],number('123.456',exact=True),['SCREEN_TINT'],focus='interaction',checks=['Eight selections returned to the starting tint; no preset was skipped.'],reference='contract')
for ident in ['FORMAT_RECORD_RAW','FORMAT_RECORD_STRING']:
    q('format-zoned-boundary-'+ident.lower(),'MODE',COMMANDS[ident]['label']+' on a long ZonedDateTime',[record('OFFSET_DATE_TIME_CREATE',[9999,12,31,23,59,59,-23,-45]),select(ident),'Inspect X using left/right scrolling as needed, then press SHOW to read the raw object.'],text('DZ{9999,12,31,23,59,59,-23,-45}'),[ident],focus='interaction',checks=['The chosen format is legible and scrolling stays within X.'],reference='date')
q('angle-mode-preserves-x','MODE','Angle-mode changes preserve X',['Enter 1 ENTER 7 ÷.',select('DEG'),select('RAD')],number(mp.mpf(1)/7),['DEG','RAD'],focus='interaction')
q('decimal-point-return','MODE','Return from comma decimal to period decimal',[select('DECIMAL_COMMA'),'Enter 1234.5 using the physical decimal key.',select('DECIMAL_POINT'),'Use SHOW and enter the answer here in period-decimal notation.'],number('1234.5',exact=True),['DECIMAL_POINT'],focus='interaction',checks=['The value is displayed with period decimal and comma grouping.'],reference='integer')

for bits in [8,32,44]:
    dividend=(1<<bits)-1; divisor=97
    for output in ['quotient','remainder']:
        q('bdiv-pair-'+str(bits)+'-'+output,'BASE','BDIV '+str(bits)+'-bit '+output,
          ['Enter '+str(bits)+'.',select('SET_WORD_SIZE')]+enter_word(dividend)+['Press ENTER.','Enter '+str(divisor)+'.',select('BASE_DIV2')]+(['Press ) (X↔Y) to move the quotient into X.'] if output=='quotient' else []),
          number(dividend//divisor if output=='quotient' else dividend%divisor,exact=True),['BASE_DIV2'],reference='integer',output=output)
q('lastx-quantity','Registers','LASTX retains a quantity',['Enter 12 (−).',select('UNIT_FOOT'),select('ABS'),'Press 2ND (−) (ANS) to recall LASTX.'],number(-12,exact=True),focus='interaction',checks=['LASTX is recalled with unit ft.'],reference='integer')
q('lastx-date','Registers','LASTX retains a DateTime',[record('DATE_TIME_CREATE',[2024,2,29,12,34,56]),select('DATE_TIME_SHRINK_TO_DATE'),'Press 2ND (−) (ANS) to recall LASTX.'],text('DT{2024,2,29,12,34,56}'),focus='interaction',reference='date')
for ident,operands in [('LOG_BASE',['8','1']),('GCD',['-1','2']),('ATANH',['1']),('XROOT_Y',['-8','2'])]:
    steps=['Enter 100 ENTER 7 +. LASTX is now 7.','Enter '+' ENTER '.join(keys(v) for v in operands)+'.',select(ident),'Note the error and the unchanged operands before clearing the message. Press 2ND (−) (ANS) to recall LASTX.']
    message='Err: Division By 0' if ident=='LOG_BASE' else 'Err: Domain'
    q('error-lastx-'+ident.lower(),'Keyboard','Failed '+COMMANDS[ident]['label']+' preserves LASTX',steps,number(7,exact=True),[ident],focus='interaction',checks=[message+' appeared, and the operands stayed unchanged.'],reference='contract')

q('tvm-rate-zero','TVM','I%YR: exact zero-interest cash flow',tvm_steps({'TVM_N':'10','TVM_PV':'1000','TVM_PMT':'-100','TVM_FV':'0'},'TVM_IYR'),number(0,exact=True),['TVM_IYR'],reference='tvm')
q('round-zero-significant','NUM','RNDG 0 uses one significant digit',['Enter 12345.',select('ROUND_TO_GUARD'),'At RNDG press 0.'],number(10000,exact=True),['ROUND_TO_GUARD'],reference='contract')
q('round-full-precision','NUM','RNDG at full stored precision',['Enter 1 EE 13 ENTER 1 +.',select('ROUND_TO_GUARD'),'At RNDG press . then 4 for 14 significant digits. Use SHOW for the full stored value.'],number(10000000000001,exact=True),['ROUND_TO_GUARD'],reference='rounding')

# Requirements are checked against the entire pool, not against completion counts.
DOMAIN_IDS=set([
 'XROOT_Y','LOG2','LOG_BASE','LN_ONE_PLUS','GCD','LCM','PRIME','MOD','PERCENT_CHANGE','COMB','PERM','FACTORIAL','ACOSH','ATANH',
 *BASE_OPS,'SET_WORD_SIZE','STAT_MEAN','STAT_WEIGHTED_MEAN','STAT_SAMPLE_SDEV','STAT_SAMPLE_COV',
 'CFIT_LINEAR','CFIT_LOG','CFIT_EXP','CFIT_POWER','CFIT_SLOPE','CFIT_CORRELATION','CFIT_FORECAST_X','CFIT_FORECAST_Y','CFIT_BEST',
 'TVM_N','TVM_IYR','TVM_PV','TVM_PMT','TVM_FV','TVM_PYR','TVM_CYR','TVM_ITER_MAX','SET_STACK_SIZE','SET_REG_SIZE'])
RULES=[]
for ident,command in COMMANDS.items():
    if ident not in {'COMPLEX_REAL','COMPLEX_IMAG','COMPLEX_CONJ','COMPLEX_ABS','COMPLEX_ANGLE','NUM_RESULT_MODE_REAL','NUM_RESULT_MODE_COMPLEX','COMPLEX_MODE_RECT','COMPLEX_MODE_RAD','COMPLEX_MODE_DEG'}:
        required=['functional','edge']
        if ident in DOMAIN_IDS or (command['section']=='DATE' and ident not in {'GET_NOW','GET_NOW_DATE','GET_NOW_TIME','GET_NOW_OFFSET_DATE_TIME','GET_NOW_OFFSET_DATE_TIME_UTC','GET_APP_TIME_ZONE','GET_CLOCK_TIME_ZONE','EPOCH_UNIX','EPOCH_NTP','EPOCH_GPS','EPOCH_TIOS','EPOCH_Y2K','EPOCH_CUSTOM','EPOCH_GET_CUSTOM'}): required+=['domain']
        RULES.append({'id':ident,'required':required})
for ident in KEYS:
    RULES.append({'id':'key:'+ident,'required':['functional','edge']+(['domain'] if ident in ['DIV','RECIP','SQRT','LN','LOG','POW','ASIN','ACOS'] else [])})

# Stable output keeps review diffs focused on the changed questions.
def render():
    ids=[q['id'] for q in QUESTIONS]
    if len(ids)!=len(set(ids)):
        from collections import Counter
        raise ValueError('Duplicate question IDs: '+str([k for k,v in Counter(ids).items() if v>1]))
    for question in QUESTIONS:
        assert question['steps'] and (question['expected'] or question['checks']),question['id']
        assert all(c in COMMANDS for c in question['covers']),question['id']
    data={'sourceDigest':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'unitDefinitionsDigest':hashlib.sha256((ROOT/'tools/genunits.py').read_bytes()).hexdigest(),'cases':QUESTIONS,'requirements':RULES}
    lines=['// Generated by questions.py; edit the source questions and regenerate.', '(function(root) {', "'use strict';",'const data = {', '  "sourceDigest": '+json.dumps(data['sourceDigest'])+',', '  "unitDefinitionsDigest": '+json.dumps(data['unitDefinitionsDigest'])+',', '  "cases": [']
    lines += [('    '+json.dumps(row,ensure_ascii=False,separators=(',',':'))+(',' if i<len(QUESTIONS)-1 else '')) for i,row in enumerate(QUESTIONS)]
    lines+=['  ],','  "requirements": '+json.dumps(RULES,ensure_ascii=False,separators=(',',':')),'};',"if (typeof module !== 'undefined' && module.exports) module.exports = data;",'else root.HardwareQuestions = data;','})(globalThis);','']
    return '\n'.join(lines)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true',help='Verify the checked-in browser data without writing it.')
    args=parser.parse_args();out=HERE/'questions.js';content=render()
    if args.check:
        if not out.exists() or out.read_text()!=content: sys.exit('questions.js is out of date; run questions.py to regenerate it.')
        print(f'{len(QUESTIONS)} expanded questions and their independent references verified.')
    else:
        out.write_text(content)
        print(f'Wrote {len(QUESTIONS)} expanded questions to {out.relative_to(ROOT)}')
