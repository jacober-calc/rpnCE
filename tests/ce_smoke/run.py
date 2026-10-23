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
                            (ROOT / 'obj/RPNCE.map').read_text(), re.M).group(1), 16)
    files = [str(ROOT / 'bin' / name) for name in ('RPNCE.8xp', 'RPNCEres.8xv')]
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

    def expectation(name, offset, digit):
        # TI real_t: sign, biased decimal exponent, fourteen packed digits.
        value = bytes([0, 0x80, digit << 4]) + bytes(6)
        hashes[name] = {'description': name, 'start': hex(address + offset), 'size': 9,
                        'expected_CRCs': [f'{crc32c(value):08X}']}
        return 'hash|' + name

    sequence = launch + [expectation('fresh-X-is-zero', 0, 0),
                         'key|2', 'key|enter', 'key|3', 'key|+',
                         expectation('keyboard-addition-X-is-five', 0, 5),
                         'key|sto', 'key|math', 'delay|500',
                         # A follows twelve reals, their unit tags, depth,
                         # LASTX and LASTX's unit in the CE's packed calc_t.
                         expectation('store-A-is-five', 131, 5),
                         'key|2nd', 'key|mode', 'delay|1500']
    sequence += launch + ['key|1', 'key|+',
                          expectation('reopen-restores-X-before-addition', 0, 6),
                          expectation('reopen-restores-register-A', 131, 5),
                          'key|2nd', 'key|mode', 'delay|1500']
    config = {'transfer_files': files, 'target': target, 'delay_after_key': 100,
              'sequence': sequence, 'hashes': hashes}
    path = output / 'ce-smoke.json'
    path.write_text(json.dumps(config, indent=2) + '\n')
    subprocess.run(['cemu-autotester', str(path)], check=True)


if __name__ == '__main__':
    main()
