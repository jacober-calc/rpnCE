# Guided hardware tests

Open [index.html](index.html) in a browser. It runs locally without an
installation, server, or calculator connection. Enter the device, OS and
build details, apply the starting settings, then follow each case on the
calculator and record what you see. Start with the focused run, or choose
all cases or a particular section. These exercises replace stack and
register values; TVM cases also change the shared TI-OS Finance variables.

**Save report** downloads a JSON file containing the build details, test
procedures, references, answers, observations, notes and attempt history.
**Open saved run** resumes it. Browser storage is a convenience; save a file
before closing, especially when opening the tool directly from disk. Put
reports you want reviewed in `tests/hardware/results/` or attach them when
reporting an issue. Reports are never uploaded or committed automatically.
Review personal notes before sharing them.

The catalog has 2,604 cases. All 421 implemented menu commands and 18
keyboard math functions have functional and edge coverage, with additional
invalid-input cases where required. The ten reserved complex-number
commands are checked for their unavailable message. The focused run stays
at 29 cases; choose all cases or a section for the expanded pool. The
boundary filter and per-function coverage table help find specific risks.

Questions include zeros and signs, rounding ties, extreme magnitudes,
near-domain and cancellation regressions, all 169 units at different
scales, BASE operations at 1, 8, 32 and 44 bits, both outputs of paired functions,
statistics and curve-fit degeneracy, every TVM unknown, Gregorian/epoch
boundaries, stack/register limits and saved state. References use exact
integer/decimal arithmetic and independently evaluated mathematical
equations. Expanded transcendental references use 180 decimal digits.
Unit checks share the program's exact unit definitions; they do not
independently audit those definitions.

An unrecorded or skipped case never counts as passed. Use SHOW for
numerical answers; tiny nonzero references have no absolute tolerance
that could let zero pass. Unexpected calculator errors can be recorded as
failures. Overflow requires an overflow error. Underflow accepts only
zero or an explicit underflow/overflow error; the final policy remains
open. Tolerances for iterative rates, near-pole inputs and cancellation
are stated in the individual questions.

[catalog.js](catalog.js) contains the menu snapshot and initial cases.
[questions.py](questions.py) defines the expanded questions, calculates
their references, and writes the browser data in `questions.js`. Edit the
Python source rather than the generated file. Each question has a stable
ID, steps, command coverage, a risk category, an expectation and optional
device observations. Update the menu snapshot/routes when menus change.
The grading checks enforce coverage requirements so adding a function
without its required tests is detected.

From this directory, regenerate or verify the expanded data with Python
and `mpmath` installed:

```sh
python3 questions.py
python3 questions.py --check
```

Catalog coverage describes available questions, not completed hardware
tests. No finite pool covers every input or interaction; remaining limits
are shown in the tool. A content signature prevents resuming a report
against changed questions. Earlier report files retain their procedures
for review; use the matching tool revision to resume them. Earlier browser
data is preserved and can be downloaded with **Save previous data**.

The browser tool's grading, report validation and menu coverage checks
require Node.js 18 or newer. From this directory, run:

```sh
node --test test_engine.cjs
```
