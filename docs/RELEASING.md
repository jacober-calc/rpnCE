# Release workflow

Prepare and test on `dev`. Merge into `main` and tag the release after
the distributed files pass device acceptance.

## Setup

- C compiler, Git, Python 3.9 or newer, and the dependency in
  `tools/requirements-release.txt`.
- CE toolchain v15.0 with `cedev-config` on PATH.
- `cemu-autotester` with link-transfer support; verified against
  [CEmu e5ffbb2](https://github.com/CE-Programming/CEmu/tree/e5ffbb2).
- A local calculator ROM, CE libraries, and a launcher if required.

Install Python dependencies in a virtual environment:

```sh
python3 -m pip install -r tools/requirements-release.txt
```

Set paths for native testing:

```sh
export AUTOTESTER_ROM=/path/to/ti84CE.rom
export AUTOTESTER_LIBS_GROUP=/path/to/clibs.8xg
export AUTOTESTER_LAUNCHER=/path/to/launcher.8xp
```

Omit the launcher for direct native execution. Keep external tools and
ROMs outside the repository. Emulator suites start fresh and do not
change the physical calculator.

## Optimization review

Review repeated work, unused code, temporary copies and expensive
rendering/conversion paths. Preserve arithmetic order, stack and LASTX
rules, keyboard behavior and saved-data formats.

Measure before changing code:

```sh
make -B
python3 tools/release_check.py --measure-only --output release/baseline
```

Run checks and compare with identical toolchain and build flags:

```sh
python3 tools/release_check.py --compare release/baseline/report.json
```

The full runner defaults to `tests/release/baseline.json`, a measurement
of the current program. Record image size, static RAM, compressed program
and resource size separately. Peak runtime stack and library allocations
need separate measurements; speed claims need native timing or work
counters. Record accepted changes and deferred candidates in the release
acceptance record. Keeping an existing routine is a valid review outcome.

## Automated checks

```sh
python3 tools/release_check.py
```

The command runs release-tool tests, isolated resource regeneration,
documentation/asset checks, all 17 desktop suites with and without
sanitizers, a fresh CE build, 83 native checks and five checks against
the actual program's keyboard and save/load paths. It validates TI file
checksums, resource identity, artifact hashes and linker-map measurements.
See [tests/README.md](../tests/README.md) for coverage.

Missing native tools or inputs fail the full run. For desktop work:

```sh
python3 tools/release_check.py --host-only
```

Reports and logs go under ignored `release/validation/`. A partial report
cannot qualify a package. Reports identify the working source, including
uncommitted changes, and the native test environment by hashes.

## Candidate and device acceptance

```sh
python3 tools/package_release.py
```

This packages the tested files as `release/RPNCE-<version>-rc.zip`, with
installation instructions, release notes, license and test report. Changed
sources or artifacts require a fresh test run.

Follow [device acceptance](../tests/release/HARDWARE.md). Copy
`release/validation/acceptance.template.json` to
`release/validation/acceptance.json`, enter tester, date and optimization
review, and mark verified categories `pass`. Keep untested categories
pending. Preserve the recorded source and artifact hashes.

## Publication

Check the version, installation instructions, examples, limitations,
documentation links and release notes. Keep the published tree free of
temporary outputs and working notes. Source changes need a fresh automated
run; repeat affected device checks when program or resource hashes change.

After recorded device acceptance:

```sh
python3 tools/package_release.py --acceptance release/validation/acceptance.json
```

This verifies both records, writes `release/RPNCE-<version>.zip`, validates
its contents, and creates the distribution copies in `prod/`. It packages
the accepted files without rebuilding them.

Commit on `dev`, merge into `main`, tag the release commit, and publish the
tested ZIP and release notes. Keep the test and acceptance records with it.

`sh tools/release.sh` combines a full check and candidate packaging.
Set `PYTHON` to select a virtual-environment interpreter.
