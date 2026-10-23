#!/usr/bin/env python3
"""Run the native CE suite in CEmu; optionally pass a TI-BASIC launcher."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
CHECK_COUNT = int(re.search(r'NATIVE_CHECK_COUNT (\d+)',
                           (HERE / 'checks.h').read_text()).group(1))


def crc32c(data):
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0x82F63B78 if crc & 1 else 0)
    return crc ^ 0xFFFFFFFF


def main():
    if not os.environ.get('AUTOTESTER_ROM'):
        sys.exit('Set AUTOTESTER_ROM to your local TI-84 Plus CE ROM file.')
    if len(sys.argv) > 2:
        sys.exit('Usage: python3 tests/native/run.py [launcher.8xp]')
    subprocess.run(['make', '-B', '-C', str(HERE)], check=True)
    files = [str(HERE / 'bin' / name) for name in ('RPNTEST.8xp', 'RPNCEres.8xv')]
    target = {'name': 'RPNTEST', 'isASM': True}
    sequence = ['action|launch', 'delay|2000']
    if len(sys.argv) == 2:
        launcher = Path(sys.argv[1]).resolve()
        if not launcher.is_file():
            sys.exit(f'Launcher not found: {launcher}')
        # arTIfiCE v2 is a TI-BASIC program, usually named A. Read its name
        # from the TI container instead of depending on the disk filename.
        data = launcher.read_bytes()
        if data[:8] != b'**TI83F*' or len(data) < 74 or data[59] != 5:
            sys.exit('Expected the arTIfiCE v2 TI-BASIC program file.')
        name = data[60:68].rstrip(b'\0').decode('ascii')
        files.append(str(launcher))
        target = {'name': name, 'isASM': False}
        sequence = ['action|launch', 'delay|1500', 'key|enter', 'delay|2000']
    # Count + pass flags + zero padding + completion marker. The count
    # verifies the whole suite ran; the flags verify each numerical result.
    expected = bytes([CHECK_COUNT]) + bytes([1] * CHECK_COUNT) + bytes(126 - CHECK_COUNT) + bytes([0xAA])
    assert len(expected) == 128
    config = {
        'transfer_files': files,
        'target': target,
        'sequence': sequence + ['hash|native'],
        'hashes': {'native': {
            'description': f'{CHECK_COUNT} native release checks: arithmetic, units, statistics, TVM, DATE and display',
            'start': '0xD40000', 'size': '128',
            'expected_CRCs': [f'{crc32c(expected):08X}'],
            'timeout': 30000,
        }},
    }
    config_path = HERE / 'bin' / 'autotest.json'
    config_path.write_text(json.dumps(config, indent=2) + '\n')
    subprocess.run(['cemu-autotester', str(config_path)], check=True)


if __name__ == '__main__':
    main()
