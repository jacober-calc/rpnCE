# Release workflow

Prepare releases on `dev`. Merge into `main` and create the version tag
after the exact release files have passed device acceptance. Complex
numbers are outside the 0.7.0 release scope.

## Tools

- A C compiler, Git, and Python 3.9 or newer.
- The Python dependency in `tools/requirements-release.txt`. Install it
  in a virtual environment with `python3 -m pip install -r tools/requirements-release.txt`.
- CE toolchain v15.0, with `cedev-config` on PATH.
- `cemu-autotester` with working link transfers. Verified with
  [CEmu source e5ffbb2](https://github.com/CE-Programming/CEmu/tree/e5ffbb2).
  The older v2.0 runner failed to make transferred programs available to
  the launcher in this setup.
- Your own calculator ROM, CE libraries and launcher where required.

Set your local paths:

```sh
export AUTOTESTER_ROM=/path/to/ti84CE.rom
export AUTOTESTER_LIBS_GROUP=/path/to/clibs.8xg
export AUTOTESTER_LAUNCHER=/path/to/arTIfiCE_v2.1.8xp
```

Omit the launcher variable when the ROM supports direct native execution.
Each emulator suite starts fresh; it does not change the physical
calculator or its saved variables. Keep external tools and ROMs outside
the repository.

## 1. Review and measure optimization candidates

Every release includes a size and efficiency review. Accept an optimization
when its benefit is measured and its behavior is verified. Keeping an
existing implementation is a valid outcome of this review.

Before changing program code, record a baseline with the release flags:

```sh
make -B
python3 tools/release_check.py --measure-only --output release/baseline
```

Review repeated work, unused initialization, temporary copies and expensive
operations in rendering and conversions. Inspect the linker map. Preserve
numerical operation order, stack/LASTX behavior, keyboard operation and
save formats. Keep feature additions separate from optimization changes.

Compare with the same toolchain and build flags:

```sh
python3 tools/release_check.py --compare release/baseline/report.json
```

The report records image size, static RAM, compressed program and resource
sizes, and their changes. These exclude peak runtime stack and library
allocations. Speed claims need native timing or work counters.

The full runner defaults to the measured baseline in
`tests/release/baseline.json`. Keep the previous release's report as the
next comparison, or pass `--compare` explicitly. Establish a fresh baseline
when changing toolchains or build flags.

For 0.7.0, the accepted follow-up selects curve-fit sums through read-only
references and a common model table while retaining the numerical
operations. It also removes unused `r_tenth` initialization. The image
shrinks from 83,249 to 82,935 bytes and the program from 46,832 to 46,743
bytes; static RAM remains 3,860 bytes and resources 17,043 bytes. The
compiler had already eliminated the unused constant's storage. A BASE
formatting experiment increased code size and was deferred. No speed
percentage is claimed.

Further candidates include sharing BASE display validation, reducing
overlapping text measurements, and avoiding repeated factor parsing.
Clock caching, TVM formula changes and resource-format changes need a
separate accuracy and compatibility review.

## 2. Run the consolidated checks

```sh
python3 tools/release_check.py
```

The full command runs:

1. Tests of the release-file validator and acceptance requirements.
2. Resource regeneration in isolation, comparing all generated outputs.
3. Local documentation and image-target checks; external URLs and
   Markdown anchors are excluded.
4. All 16 desktop suites, normally and under address/undefined-behavior
   sanitizers. These include conversion, storage and rounding across all
   units, TEMP/FUEL refusal rules, and 23,232 BASE formatting cases.
5. Core comparison, normally and under sanitizers: 22,848 command/state
   cases and 8,568 entry steps. This checks established scalar behavior;
   new features need their own expected-result tests.
6. A fresh CE build and 83 native checks using TI-OS arithmetic, covering
   all-unit base round trips, all four curve-fit models, reported bugs,
   DATE, TVM and display pixels.
7. Five checks against the shipped program in CEmu: startup, keyboard
   addition, storing A, quitting/reopening with X retained, and A retained.
8. TI file checksums and identities, resource payload and stamp validation,
   artifact hashes, and linker-map measurements.

Missing native tools or inputs fail the full run. For desktop-only work:

```sh
python3 tools/release_check.py --host-only
```

A desktop-only report cannot qualify a release package. Reports and logs
go under ignored `release/validation/`. The report identifies the Git
commit and a digest of the working sources, including uncommitted work.
It hashes the native test environment without storing ROM paths or contents.

## 3. Test the candidate on a calculator

Package the already-tested files:

```sh
python3 tools/package_release.py
```

This creates `release/RPNCE-<version>-rc.zip` with the program, matching
resource, installation instructions, release notes, license and test
report. Packaging rejects changed source or binaries. `prod/` continues
to hold the published release.

Follow [HARDWARE_TEST.md](HARDWARE_TEST.md) on the physical calculator.
Record fresh installation, upgrading and saved data, keys and menus, LCD
readability and tints, brightness, actual clock behavior, units, statistics
and TVM. Emulator results do not close physical-device checks.

Copy `release/validation/acceptance.template.json` to
`release/validation/acceptance.json`. Enter tester, date, optimization
review outcome, and set each verified category to `pass`. Leave untested
or failed categories pending. Keep the source and artifact hashes intact.

## 4. Review the final repository and documentation

Before freezing the final source, check version labels, installation,
examples, limitations and release notes. Remove obsolete working notes
and conversational boilerplate. Retain useful user documentation,
reproducible build/test instructions, comments explaining design decisions,
licenses and required attribution.

Source changes require a new automated run and acceptance template. Reuse
device evidence only when the tested program/resource hashes are identical;
otherwise repeat the affected calculator checks. Version numbers alone
do not identify tested files.

## 5. Promote and publish

After recorded device acceptance:

```sh
python3 tools/package_release.py --acceptance release/validation/acceptance.json
```

This verifies both records, writes `release/RPNCE-<version>.zip`, verifies
the ZIP contents, and updates the three distribution files in `prod/`.
It does not rebuild the accepted files.

Commit the final source, documentation and `prod/` changes on `dev`.
Merge `dev` into `main`, tag the release commit, and publish the tested
ZIP with the release notes. Keep the test and acceptance records with it.

`sh tools/release.sh` is the convenience command for a fresh full check
and candidate package. Set `PYTHON` to select a virtual environment.
