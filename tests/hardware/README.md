# Guided hardware tests

Open [index.html](index.html) in a browser. It runs locally without an
installation, server, or calculator connection. Enter the device, OS and
build details, apply the starting settings, then follow each case on the
calculator and record what you see. New runs include all cases; choose a
particular section to work through a smaller group. These exercises replace stack and
register values; TVM cases also change the shared TI-OS Finance variables.

**Save report** downloads a JSON file containing the build details, test
procedures, references, answers, observations, notes and attempt history.
**Open saved run** resumes it. Browser storage is a convenience; save a file
before closing, especially when opening the tool directly from disk. Put
reports you want reviewed in `tests/hardware/results/` or attach them when
reporting an issue. Reports are never uploaded or committed automatically.
Review personal notes before sharing them.

The catalog has 2,602 cases. All 421 implemented menu commands and 18
keyboard math functions have functional and edge coverage, with additional
invalid-input cases where required. The ten reserved complex-number
commands are checked for their unavailable message. The boundary filter
and per-function coverage table help find specific risks.

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
failures. Overflow requires an overflow error; underflow requires
**Err: Underflow**, with no silent zero. Selected precision and intermediate
range cases accept either the stated numerical reference or the specific
error listed in that question. Other errors and misleading zero answers
still fail. Tolerances are stated in the individual questions.

Catalog revision `hardware-0.7.0-8` uses the rpnCE branding and retains
the message acknowledgement checks:
CLEAR dismisses a visible message without changing the stack; other keys
dismiss it and perform their normal action. Routine notices time out after
about 1.5 seconds. The carry indicator is **CF**, including in DEC mode.
New runs start with all cases selected.
The focused run and completed 14-case numerical confirmation run have been
removed, along with the 14 completed individual cases. Their original device reports
remain unchanged in [results/](results/), and their native regression checks
remain in place. Start a new run with this catalog; earlier saved data can
still be downloaded for review, but cannot be resumed against changed cases.

The question audit corrected weekday input types, SCI/ENG display-rounding
references and exact-match expectations for logarithm/root calculations and
unit ratios. Nine additional range cases require explicit overflow or
underflow errors for powers, exponentials, reciprocal, square, cube and
percent. The generator rejects numerical references outside the supported
14-digit range instead of silently treating them as ordinary answers.

Hardware testing confirmed silent zeros for `10^(-100)` and `2^(-333)`,
and an incorrect remainder near 2 inches for `6 ft MOD 2 in`. The corrected
program reports **Err: Underflow** for the powers and **Err: Precision**
for the ambiguous converted-unit remainder. These regression cases remain
under **All cases** → **Known issues** pending hardware confirmation of the
fixes. The browser does not mark them passed or create a device report
automatically.

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
