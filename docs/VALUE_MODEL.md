# Universal values

`value_t` is the only user-value container. Its union holds either two
`real_t` numeric components or a `dt_obj_t` record. A type discriminator and
unit tag travel with the payload. The CE layout is 20 bytes; real-only
statistics and TI-OS finance working data continue to use `real_t`.

`value_eval()` owns arithmetic dispatch and error protection. Stack math,
STO/RCL arithmetic, structural DATE aliases and BASE operations use it.
DATE and quantity rules are specialized mathematical rules within that
interface, not alternative stack/register implementations. Mixed numeric
operations promote real operands to complex. Real-only consumers use the
shared value predicates or `value_get_real()` boundary.
Numeric operands are range-checked before dispatch. The real kernel checks
converted quantities again because unit conversion can change their range.

`value_parse()` is the canonical parser for completed user entry, including
DATE records, BASE words and rectangular/polar complex pairs. Editing only
constructs that text. `value_format()` serves stack, catalog and SHOW;
BASE word presentation remains a display option around numeric values.

Complex kernels use TI decimal primitives. The CE C library exposes only a
small subset of the assembly program's complex wrappers, so the shared
numeric evaluator supplies the remaining operations. Complex powers use
principal logarithms, with integer powers evaluated directly. Polar input
and display never change the stored rectangular components.

Stack and register assignments always copy complete values. Code must not
copy just `.real` into a user slot or maintain external unit arrays. Scalar
kernels may read `.real` after checking the supported value type. Errors
are computed before committing stack/register changes; statistics and BASE
callers use trial state for operations that update their working data.

Current saved state is version 6. `storage_import.c` is a read-only importer
for version 5, including its nine-byte DATE markers and optional unit
sidecar/footer. These representations exist only at that historical file
boundary. No current writer emits them. Importing and saving removes the
retired unit sidecar; old binaries reject the new version.

## Numeric modes and presentation

`calc_t.degrees` starts true (DEG) on fresh state; saved state restores the
user's choice. `complex_results` starts false (RRES), and `complex_mode`
starts at `COMPLEX_RECT`. These are independent settings. CANG follows
`degrees`; PRAD/PDEG and polar entry specify their own angle units. Complex
logarithms and exponentials always use radians. Display mode changes do
not rewrite stored rectangular components or coerce a value to real.

`value_eval()` promotes real operands only for supported complex
operations. CRES extends negative real square roots, logarithms, powers
and general roots to principal complex results. Explicit complex inputs
remain complex even when their imaginary component becomes zero. REAL
and IMAG explicitly extract real components. BASE words, statistics, TVM,
numeric settings and DATE scalar arguments reject complex values at their
real-value boundaries; complex quantities with units are unsupported.

`draw_status()` in `src/ui/display.c` always shows the display format.
FIX with `DISP_FLOAT` is ALL; floating SCI/ENG retain SCI-/ENG-. DEG, DEC,
RRES and RECT have no labels. RAD, HEX/OCT/BIN, CF, CRES, PRAD/PDEG and
pending 2ND/ALPHA badges appear when active. The full ATZ timestamp becomes
HH:MM when needed to prevent overlap. MODE marks all selected choices.

The user-facing contract and illustrated keystrokes are in
[Complex numbers](guide/11-complex-numbers.md),
[Number types](guide/05-number-entry.md#number-types) and
[Status bar](guide/04-display.md#status-bar). Rebuild resources after HELP
changes and regenerate the function index and screenshots with their
source tools; generated files must match the current code and text.
