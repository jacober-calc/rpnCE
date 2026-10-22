#!/usr/bin/env python3
"""Run release checks and record the exact source and calculator artifacts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parent.parent
GENERATED = ('data/RPNCEres.bin', 'src/ui/menu_data.h', 'src/ui/res_info.h',
             'src/core/units_count.h', 'tests/units_data.c')
MANUAL_CHECKS = ('fresh_install', 'upgrade_and_saved_data', 'keys_and_menus',
                 'display_and_tints', 'brightness', 'clock_and_timezones',
                 'units_including_temperature_and_fuel', 'statistics_and_tvm')


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def source_identity(root=ROOT):
    """Include uncommitted work; exclude distribution copies and build output."""
    names = subprocess.check_output(
        ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'],
        cwd=root).decode().split('\0')
    digest = hashlib.sha256()
    for name in sorted(set(names)):
        if not name or name.startswith('prod/'):
            continue
        path = root / name
        if path.is_file():
            digest.update(name.encode() + b'\0' + path.read_bytes() + b'\0')
    return digest.hexdigest()


def version(root=ROOT):
    return re.search(r'#define RPNCE_VERSION "([^"]+)"',
                     (root / 'src/core/version.h').read_text()).group(1)


def ti_payload(data, name, kind):
    """Validate a single-variable TI container, including its checksum."""
    if len(data) < 76 or data[:11] != b'**TI83F*\x1a\x0a\x00':
        raise ValueError('invalid TI file header')
    length = struct.unpack_from('<H', data, 53)[0]
    if len(data) != 55 + length + 2:
        raise ValueError('invalid TI file length')
    section = data[55:-2]
    if sum(section) & 0xFFFF != struct.unpack_from('<H', data, len(data) - 2)[0]:
        raise ValueError('TI file checksum mismatch')
    header, size = struct.unpack_from('<HH', section)
    if header != 13 or section[4] != kind or section[5:13].rstrip(b'\0') != name.encode():
        raise ValueError('unexpected TI variable type, name or header')
    if struct.unpack_from('<H', section, 15)[0] != size or len(section) != 17 + size:
        raise ValueError('invalid TI variable payload size')
    payload = section[17:]
    if len(payload) < 2 or struct.unpack_from('<H', payload)[0] != len(payload) - 2:
        raise ValueError('invalid TI variable data length')
    return payload[2:]


def validate_resource(payload, root=ROOT):
    if payload != (root / 'data/RPNCEres.bin').read_bytes():
        raise ValueError('appvar differs from generated resource data')
    if len(payload) < 34 or payload[:4] != b'RPNR':
        raise ValueError('invalid resource header')
    fmt = struct.unpack_from('<H', payload, 4)[0]
    stamp = struct.unpack_from('<I', payload, 12)[0]
    info = (root / 'src/ui/res_info.h').read_text()
    expected = (int(re.search(r'RES_FORMAT (\d+)', info).group(1)),
                int(re.search(r'RES_STAMP 0x([0-9A-Fa-f]+)', info).group(1), 16),
                int(re.search(r'RES_SIZE (\d+)', info).group(1)))
    h = 2166136261
    for byte in payload[16:]:
        h = ((h ^ byte) * 16777619) & 0xFFFFFFFF
    if (fmt, stamp, len(payload)) != expected or h != stamp:
        raise ValueError('resource format, stamp, size or body checksum mismatch')
    return {'format': fmt, 'stamp': f'{stamp:08X}'}


def map_metrics(text):
    sections = {name: int(size, 16) for name, size in re.findall(
        r'^\.(header|init|text|rodata|data|bss)\s+0x[\da-fA-F]+\s+0x([\da-fA-F]+)',
        text, re.M)}
    if not {'header', 'init', 'text', 'rodata', 'data', 'bss'} <= sections.keys():
        raise ValueError('missing sections in CE linker map')
    return {'image_bytes': sum(sections[n] for n in ('header', 'init', 'text', 'rodata', 'data')),
            'static_ram_bytes': sections['bss']}


def artifacts(root=ROOT):
    files = {}
    for name, kind in (('RPNCE', 6), ('RPNCEres', 21)):
        filename = name + ('.8xp' if kind == 6 else '.8xv')
        data = (root / 'bin' / filename).read_bytes()
        payload = ti_payload(data, name, kind)
        files[filename] = {'bytes': len(data), 'sha256': sha256(data)}
        if kind == 21:
            resource = validate_resource(payload, root)
    metrics = map_metrics((root / 'bin/RPNCE.map').read_text())
    metrics.update(program_bytes=files['RPNCE.8xp']['bytes'],
                   resource_bytes=files['RPNCEres.8xv']['bytes'])
    return {'files': files, 'metrics': metrics, 'resource': resource}


def check_generated(root=ROOT):
    # Regenerate in isolation so a failed check cannot change the checkout.
    with tempfile.TemporaryDirectory(prefix='rpnce-resources-') as temp:
        copy = Path(temp)
        for name in ('tools/genres.py', 'tools/genmenu.py', 'tools/genunits.py',
                     'menus/menus.txt', 'help/help.txt'):
            target = copy / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / name, target)
        for folder in ('src/ui', 'src/core', 'tests'):
            (copy / folder).mkdir(parents=True, exist_ok=True)
        subprocess.run([sys.executable, str(copy / 'tools/genres.py')],
                       cwd=copy, check=True)
        stale = [name for name in GENERATED
                 if (copy / name).read_bytes() != (root / name).read_bytes()]
        if stale:
            raise ValueError('stale generated files: ' + ', '.join(stale))


def check_documentation(root=ROOT):
    missing = []
    count = 0
    paths = [root / 'README.md', *(root / 'docs').rglob('*.md')]
    for path in paths:
        text = path.read_text()
        targets = re.findall(r'!?\[[^\]]*\]\(([^\s)]+)(?:\s+"[^"]*")?\)', text)
        targets += re.findall(r'<img\s[^>]*src="([^"]+)"', text)
        for target in targets:
            if '://' in target or target.startswith(('#', 'mailto:')):
                continue
            target = target.split('#', 1)[0]
            if target:
                count += 1
                if not (path.parent / target).exists():
                    missing.append(f'{path.relative_to(root)}: {target}')
    if missing:
        raise ValueError('missing documentation targets:\n' + '\n'.join(missing))
    print(f'{count} local documentation/image targets exist (external URLs and anchors excluded).')


def validate_acceptance(acceptance, report):
    if report.get('status') != 'automated_passed':
        raise ValueError('a complete passing automated report is required')
    for key in ('source_sha256', 'artifacts'):
        expected = report[key] if key == 'source_sha256' else report[key]['files']
        if acceptance.get(key) != expected:
            raise ValueError('acceptance does not identify these tested sources and files')
    if not acceptance.get('tester', '').strip() or not acceptance.get('date', '').strip():
        raise ValueError('acceptance needs a tester and date')
    if not acceptance.get('optimization_review', '').strip():
        raise ValueError('record the optimization review, including deferred work')
    if any(acceptance.get('checks', {}).get(key) != 'pass' for key in MANUAL_CHECKS):
        raise ValueError('physical-device acceptance is incomplete')


def run_command(name, command, out, steps, env=None):
    print(f'[{name}]', flush=True)
    with (out / (name + '.log')).open('w') as log:
        result = subprocess.run(command, cwd=ROOT, env=env, stdout=log,
                                stderr=subprocess.STDOUT)
    steps.append({'name': name, 'status': 'pass' if result.returncode == 0 else 'fail'})
    print((out / (name + '.log')).read_text(), end='', flush=True)
    if result.returncode:
        raise RuntimeError(f'{name} failed; see {out / (name + ".log")}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--host-only', action='store_true', help='desktop checks; not release qualification')
    mode.add_argument('--measure-only', action='store_true', help='inspect an already built baseline; no tests')
    parser.add_argument('--output', type=Path, default=ROOT / 'release/validation')
    parser.add_argument('--compare', type=Path, help='earlier JSON report made with the same toolchain')
    args = parser.parse_args()
    if not args.measure_only and args.compare is None:
        args.compare = ROOT / 'tests/release/baseline.json'
    out = args.output.resolve()
    if out == ROOT or ROOT in out.parents and out.relative_to(ROOT).parts[0] not in ('release',):
        parser.error('write reports under release/ or outside the checkout')
    out.mkdir(parents=True, exist_ok=True)
    report = {'schema': 1, 'version': version(), 'status': 'failed',
              'started_utc': datetime.now(timezone.utc).isoformat(),
              'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT).decode().strip(),
              'source_sha256': source_identity(), 'steps': [], 'physical_acceptance': 'pending'}
    report['build_settings'] = {'makefile_sha256': sha256((ROOT / 'makefile').read_bytes())}
    try:
        if not args.measure_only:
            # Missing native tools are a failure, never a silently skipped pass.
            required = ['cc', 'git'] + ([] if args.host_only else ['make', 'cedev-config', 'cemu-autotester'])
            missing = [name for name in required if not shutil.which(name)]
            if missing:
                raise ValueError('missing tools: ' + ', '.join(missing))
            if not args.host_only:
                for name in ('AUTOTESTER_ROM', 'AUTOTESTER_LIBS_GROUP'):
                    if not Path(os.environ.get(name, '')).is_file():
                        raise ValueError(f'set {name} to your local file')
            run_command('release-tools', [sys.executable, '-m', 'unittest', 'discover',
                                         '-s', 'tests/release', '-v'], out, report['steps'])
            check_generated()
            report['steps'].append({'name': 'generated-resources', 'status': 'pass'})
            check_documentation()
            report['steps'].append({'name': 'documentation', 'status': 'pass'})
            run_command('desktop', ['sh', 'tests/run_tests.sh'], out, report['steps'])
            env = dict(os.environ, EXTRA_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer')
            env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
            run_command('desktop-sanitized', ['sh', 'tests/run_tests.sh'], out, report['steps'], env)
            run_command('core-equivalence', ['sh', 'tests/check_core_equivalence.sh'], out, report['steps'])
            run_command('core-equivalence-sanitized', ['sh', 'tests/check_core_equivalence.sh'],
                        out, report['steps'], env)
            if not args.host_only:
                run_command('ce-build', ['make', '-B'], out, report['steps'])
                native = [sys.executable, 'tests/ce_date/run.py']
                if os.environ.get('AUTOTESTER_LAUNCHER'):
                    native.append(os.environ['AUTOTESTER_LAUNCHER'])
                run_command('ce-native', native, out, report['steps'])
                run_command('ce-smoke', [sys.executable, 'tests/ce_smoke/run.py'],
                            out, report['steps'])
        if not args.host_only:
            report['artifacts'] = artifacts()
            report['toolchain'] = subprocess.check_output(['cedev-config', '--version'], cwd=ROOT).decode().strip()
            if not args.measure_only:
                report['native_environment'] = {
                    name.lower() + '_sha256': sha256(Path(os.environ[name]).read_bytes())
                    for name in ('AUTOTESTER_ROM', 'AUTOTESTER_LIBS_GROUP', 'AUTOTESTER_LAUNCHER')
                    if os.environ.get(name)}
                report['native_environment']['emulator_sha256'] = sha256(
                    Path(shutil.which('cemu-autotester')).read_bytes())
            if args.compare:
                baseline = json.loads(args.compare.read_text())
                if baseline.get('toolchain') != report['toolchain']:
                    raise ValueError('size comparison requires the same CE toolchain')
                if baseline.get('build_settings') != report['build_settings']:
                    raise ValueError('size comparison requires the same makefile/build flags')
                report['size_delta'] = {key: value - baseline['artifacts']['metrics'][key]
                                        for key, value in report['artifacts']['metrics'].items()}
                print('Size changes:', json.dumps(report['size_delta'], sort_keys=True))
        if source_identity() != report['source_sha256']:
            raise ValueError('sources changed during checks; rerun against the final source')
        report['status'] = ('measurement_only' if args.measure_only else
                            'host_only_passed' if args.host_only else 'automated_passed')
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        report['error'] = str(error)
        print(f'Release checks failed: {error}', file=sys.stderr)
    report['finished_utc'] = datetime.now(timezone.utc).isoformat()
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    if report['status'] == 'automated_passed':
        acceptance = {'tester': '', 'date': '', 'source_sha256': report['source_sha256'],
                      'artifacts': report['artifacts']['files'], 'optimization_review': '',
                      'checks': {name: 'pending' for name in MANUAL_CHECKS}}
        (out / 'acceptance.template.json').write_text(json.dumps(acceptance, indent=2) + '\n')
    print(f"{report['status']}: {out / 'report.json'}")
    return report['status'] == 'failed'


if __name__ == '__main__':
    sys.exit(main())
