#!/usr/bin/env python3
"""Exercise the shipped program's keyboard loop and real appvar persistence."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/native'))
from run import crc32c


def main():
    output = Path(__file__).resolve().parent / 'bin'
    output.mkdir(parents=True, exist_ok=True)
    address = int(re.search(r'^\s*\.bss\._calc\s+0x([0-9a-fA-F]+)',
                            (ROOT / 'obj/rpnCE.map').read_text(), re.M).group(1), 16)
    files = [str(ROOT / 'bin' / name) for name in ('rpnCE.8xp', 'rpnCEres.8xv')]
    target = {'name': 'RPNCE', 'isASM': True}
    launch = ['action|launch', 'delay|1500']
    if os.environ.get('AUTOTESTER_LAUNCHER'):
        launcher = Path(os.environ['AUTOTESTER_LAUNCHER']).resolve()
        data = launcher.read_bytes()
        if data[:8] != b'**TI83F*' or len(data) < 74 or data[59] != 5:
            sys.exit('Expected an arTIfiCE v2 TI-BASIC launcher.')
        files.append(str(launcher))
        target = {'name': data[60:68].rstrip(b'\0').decode('ascii'), 'isASM': False}
        launch += ['key|enter', 'delay|1500']
    hashes = {}

    def expectation(name, offset, digit, exponent=0, negative=False):
        # TI real_t: sign, biased decimal exponent, fourteen packed digits.
        value = bytes([0x80 if negative else 0, 0x80 + exponent, digit << 4]) + bytes(6)
        hashes[name] = {'description': name, 'start': hex(address + offset), 'size': 9,
                        'expected_CRCs': [f'{crc32c(value):08X}']}
        return 'hash|' + name

    def complex_expectation(name, real_digit, imag_digit, negative_real=False):
        real = bytes([0x80 if negative_real else 0, 0x80, real_digit << 4]) + bytes(6)
        imag = bytes([0, 0x80, imag_digit << 4]) + bytes(6)
        value = real + imag + bytes([1, 0])  # complex discriminator, no unit
        hashes[name] = {'description': name, 'start': hex(address), 'size': 20,
                        'expected_CRCs': [f'{crc32c(value):08X}']}
        return 'hash|' + name

    sequence = launch + [expectation('fresh-X-is-zero', 0, 0),
                         'key|2', 'key|enter', 'key|3', 'key|+',
                         expectation('keyboard-addition-X-is-five', 0, 5),
                         'key|2nd', 'key|enter', 'delay|2000', 'key|clear',
                         expectation('CLEAR-dismisses-SHOW-without-clearing-X', 0, 5),
                         'key|2nd', 'key|enter', 'key|enter',
                         expectation('ENTER-after-SHOW-still-duplicates-X', 20, 5),
                         'key|sto', 'key|math', 'delay|500',
                         # A follows twelve 20-byte values, depth, and LASTX.
                         expectation('store-A-is-five', 262, 5),
                         'key|2nd', 'key|mode', 'delay|1500']
    sequence += launch + ['key|1', 'key|+',
                          expectation('reopen-restores-X-before-addition', 0, 6),
                          expectation('reopen-restores-register-A', 262, 5),
                          'key|clear', 'key|1', 'key|,', 'key|(-)', 'key|9', 'key|9', 'key|enter',
                          expectation('keyboard-minimum-exponent', 0, 1, -99),
                          'key|sto', 'key|apps', 'key|2nd', 'key|mode', 'delay|1500']
    sequence += launch + ['key|2nd', 'key|sto', 'key|apps',
                          expectation('reopen-restores-tiny-register-B', 0, 1, -99),
                          'key|clear', 'key|1', 'key|,', 'key|9', 'key|9', 'key|enter',
                          expectation('keyboard-maximum-exponent', 0, 1, 99),
                          'key|sto', 'key|prgm', 'key|2nd', 'key|mode', 'delay|1500']
    sequence += launch + ['key|2nd', 'key|sto', 'key|prgm',
                          expectation('reopen-restores-large-register-C', 0, 1, 99),
                          'key|2nd', 'key|mode', 'delay|1500']
    sequence += launch + ['key|clear', 'key|clear', 'key|stat', 'key|graph']
    for _ in range(3):
        sequence += ['key|' + digit for digit in '100000001'] + ['key|y=']
    sequence += ['key|down', 'key|down', 'key|y=',
                 expectation('constant-large-data-has-zero-deviation', 0, 0),
                 'key|2nd', 'key|mode', 'delay|1500']
    sequence += launch + ['key|7', 'key|stat', 'key|down', 'key|down', 'key|y=',
                          expectation('reopen-retains-statistics-constancy', 0, 0),
                          'key|clear', 'key|clear', 'key|8', 'key|enter', 'key|1', 'key|(-)',
                          'key|ln', 'delay|2000', 'key|clear',
                          expectation('CLEAR-dismisses-error-without-changing-X', 0, 1, negative=True),
                          expectation('CLEAR-dismisses-error-without-changing-Y', 20, 8),
                          'key|ln', 'key|enter',
                          expectation('ENTER-after-error-duplicates-X', 20, 1, negative=True),
                          expectation('ENTER-after-error-lifts-old-Y', 40, 8),
                          'key|5', 'key|enter', 'key|math', 'key|down', 'key|down', 'key|window',
                          'key|down', 'key|down', 'key|zoom', 'key|clear',
                          expectation('CLEAR-dismisses-a-visible-notice', 0, 5),
                          'key|zoom', 'delay|2000', 'key|clear',
                          expectation('notice-times-out-and-CLEAR-resumes-CLX', 0, 0),
                          'key|2nd', 'key|mode', 'delay|1500']
    sequence += launch + ['key|clear', 'key|clear', 'key|1', 'key|2nd', 'key|.',
                          'key|2', 'key|enter',
                          complex_expectation('keyboard-constructs-one-complex-value', 1, 2),
                          'key|sto', 'key|math', 'key|2nd', 'key|mode', 'delay|1500']
    sequence += launch + ['key|2nd', 'key|sto', 'key|math',
                          complex_expectation('reopen-restores-both-complex-components', 1, 2),
                          'key|^2', complex_expectation('keyboard-complex-square', 3, 4, True),
                          'key|2nd', 'key|mode', 'delay|1500']
    config = {'transfer_files': files, 'target': target, 'delay_after_key': 100,
              'sequence': sequence, 'hashes': hashes}
    path = output / 'ce-smoke.json'
    path.write_text(json.dumps(config, indent=2) + '\n')
    subprocess.run(['cemu-autotester', str(path)], check=True)


if __name__ == '__main__':
    main()
