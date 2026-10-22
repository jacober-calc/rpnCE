"""Release checks must reject damaged files and incomplete acceptance."""
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
import release_check as release
import package_release as package


def container(payload=b'example', name='RPNCEres', kind=21):
    data = struct.pack('<H', len(payload)) + payload
    section = struct.pack('<HHB8sBBH', 13, len(data), kind,
                          name.encode(), 0, 0x80, len(data)) + data
    return (b'**TI83F*\x1a\x0a\x00' + bytes(42) + struct.pack('<H', len(section)) +
            section + struct.pack('<H', sum(section) & 0xFFFF))


class ReleaseTests(unittest.TestCase):
    def test_stale_package_inputs(self):
        names = ('release-tools', 'generated-resources', 'documentation', 'desktop',
                 'desktop-sanitized', 'core-equivalence', 'core-equivalence-sanitized',
                 'ce-build', 'ce-native', 'ce-smoke')
        report = {'schema': 1, 'status': 'automated_passed', 'version': '0.7.0',
                  'source_sha256': 'source', 'artifacts': {'files': 'tested'},
                  'steps': [{'name': name, 'status': 'pass'} for name in names]}
        with patch.object(package, 'version', return_value='0.7.0'), \
             patch.object(package, 'source_identity', return_value='source') as source, \
             patch.object(package, 'artifacts', return_value={'files': 'tested'}) as files:
            package.qualified_report(report)
            source.return_value = 'changed'
            with self.assertRaises(ValueError):
                package.qualified_report(report)
            source.return_value = 'source'
            files.return_value = {'files': 'changed'}
            with self.assertRaises(ValueError):
                package.qualified_report(report)
            files.return_value = {'files': 'tested'}
            report['steps'].append({'name': 'extra', 'status': 'fail'})
            with self.assertRaises(ValueError):
                package.qualified_report(report)

    def test_documentation_missing_file(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'README.md').write_text('[missing](missing.md)')
            with self.assertRaises(ValueError):
                release.check_documentation(root)

    def test_ti_container(self):
        self.assertEqual(release.ti_payload(container(), 'RPNCEres', 21), b'example')

    def test_ti_damage(self):
        data = container()
        for damaged in (data[:-1], data + b'\0', bytes([data[0] ^ 1]) + data[1:],
                        data[:-3] + bytes([data[-3] ^ 1]) + data[-2:]):
            with self.assertRaises(ValueError):
                release.ti_payload(damaged, 'RPNCEres', 21)
        with self.assertRaises(ValueError):
            release.ti_payload(data, 'OTHER', 21)
        with self.assertRaises(ValueError):
            release.ti_payload(data, 'RPNCEres', 6)

    def test_resource_damage(self):
        payload = (release.ROOT / 'data/RPNCEres.bin').read_bytes()
        self.assertEqual(release.validate_resource(payload)['format'], 2)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'data').mkdir()
            (root / 'src/ui').mkdir(parents=True)
            (root / 'src/ui/res_info.h').write_bytes((release.ROOT / 'src/ui/res_info.h').read_bytes())
            damaged = bytearray(payload)
            damaged[-1] ^= 1
            (root / 'data/RPNCEres.bin').write_bytes(damaged)
            with self.assertRaises(ValueError):
                release.validate_resource(bytes(damaged), root)

    def test_map_sections(self):
        text = '\n'.join(f'.{name} 0x1234 0x10' for name in
                         ('header', 'init', 'text', 'rodata', 'data', 'bss'))
        self.assertEqual(release.map_metrics(text), {'image_bytes': 80, 'static_ram_bytes': 16})
        with self.assertRaises(ValueError):
            release.map_metrics('')

    def test_acceptance(self):
        report = {'status': 'automated_passed', 'source_sha256': 'source',
                  'artifacts': {'files': {'program': 'hash'}}}
        acceptance = {'tester': 'Tester', 'date': '2026-10-21',
                      'source_sha256': 'source', 'artifacts': {'program': 'hash'},
                      'optimization_review': 'Reviewed; numerical changes deferred.',
                      'checks': {name: 'pass' for name in release.MANUAL_CHECKS}}
        release.validate_acceptance(acceptance, report)
        for field, value in (('tester', ''), ('date', ''), ('optimization_review', ''),
                             ('source_sha256', 'other'), ('artifacts', {}), ('checks', {})):
            changed = dict(acceptance, **{field: value})
            with self.assertRaises(ValueError):
                release.validate_acceptance(changed, report)
        for status in ('host_only_passed', 'measurement_only', 'failed'):
            with self.assertRaises(ValueError):
                release.validate_acceptance(acceptance, dict(report, status=status))

    def test_partial_reports_cannot_package(self):
        for status in ('host_only_passed', 'measurement_only', 'failed', 'automated_passed'):
            with self.assertRaises(ValueError):
                package.qualified_report({'schema': 1, 'status': status, 'steps': []})


if __name__ == '__main__':
    unittest.main()
