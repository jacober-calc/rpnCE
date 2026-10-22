#!/usr/bin/env python3
"""Package verified calculator files; promote prod/ after device acceptance."""
import argparse
import json
from pathlib import Path
import shutil
import sys
import zipfile

from release_check import ROOT, artifacts, source_identity, validate_acceptance, version


def qualified_report(report):
    if report.get('schema') != 1 or report.get('status') != 'automated_passed':
        raise ValueError('run the full release checks first')
    required = {'release-tools', 'generated-resources', 'documentation', 'desktop',
                'desktop-sanitized', 'core-equivalence', 'core-equivalence-sanitized',
                'ce-build', 'ce-native', 'ce-smoke'}
    passed = {step['name'] for step in report.get('steps', []) if step['status'] == 'pass'}
    if not required <= passed or any(step['status'] != 'pass' for step in report['steps']):
        raise ValueError('automated report is missing required checks')
    if report.get('version') != version() or report.get('source_sha256') != source_identity():
        raise ValueError('sources changed after testing; rerun release checks')
    if report.get('artifacts') != artifacts():
        raise ValueError('calculator files changed after testing; rerun release checks')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, default=ROOT / 'release/validation/report.json')
    parser.add_argument('--acceptance', type=Path, help='completed device acceptance JSON; makes the final ZIP and updates prod/')
    args = parser.parse_args()
    try:
        report = json.loads(args.report.read_text())
        qualified_report(report)
        acceptance = None
        if args.acceptance:
            acceptance = json.loads(args.acceptance.read_text())
            validate_acceptance(acceptance, report)
        label = 'RPNCE-' + version() + ('' if acceptance else '-rc')
        folder = ROOT / 'release' / label
        # Never recursively delete an existing directory. Overwrite only
        # these known package members, then zip precisely that file list.
        folder.mkdir(parents=True, exist_ok=True)
        members = ['RPNCE.8xp', 'RPNCEres.8xv', 'INSTALL.txt', 'LICENSE',
                   'RELEASE_NOTES.md', 'TEST_RESULTS.json']
        for name in members[:2]:
            shutil.copyfile(ROOT / 'bin' / name, folder / name)
        install = (ROOT / 'docs/INSTALL.txt').read_text().replace('@VERSION@', version())
        if not acceptance:
            install = 'RELEASE CANDIDATE: physical-device acceptance is pending.\n\n' + install
        (folder / 'INSTALL.txt').write_text(install)
        shutil.copyfile(ROOT / 'LICENSE', folder / 'LICENSE')
        notes = (ROOT / 'docs/RELEASE_NOTES.md').read_text()
        guide_url = 'https://github.com/jacober-calc/rpnce/blob/' + ('main' if acceptance else 'dev') + '/docs/'
        notes = notes.replace('(guide/', '(' + guide_url + 'guide/')
        notes = notes.replace('(USER_GUIDE.md)', '(' + guide_url + 'USER_GUIDE.md)')
        (folder / 'RELEASE_NOTES.md').write_text(notes)
        (folder / 'TEST_RESULTS.json').write_text(json.dumps(report, indent=2) + '\n')
        if acceptance:
            members.append('ACCEPTANCE.json')
            (folder / 'ACCEPTANCE.json').write_text(json.dumps(acceptance, indent=2) + '\n')
        archive = folder.parent / (label + '.zip')
        with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as package:
            for name in members:
                package.write(folder / name, label + '/' + name)
        # Reopen and compare the actual distributable, not just its inputs.
        with zipfile.ZipFile(archive) as package:
            if package.testzip() is not None:
                raise ValueError('release ZIP checksum failure')
            for name in members:
                if package.read(label + '/' + name) != (folder / name).read_bytes():
                    raise ValueError('release ZIP content mismatch: ' + name)
        if acceptance:
            for name in members[:3]:
                shutil.copyfile(folder / name, ROOT / 'prod' / name)
        print(archive)
        print('Final package; prod/ updated.' if acceptance else
              'Candidate package; complete device acceptance before final promotion.')
        return 0
    except (OSError, ValueError, KeyError) as error:
        print(f'Packaging failed: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
