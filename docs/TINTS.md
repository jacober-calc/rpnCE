# Screen tints

MODE > TINT cycles through these presets and shows the current name. The
selection is saved when RPNCE exits.

| Preset | Appearance |
| --- | --- |
| LCD | Original green LCD palette |
| LCD+ | Stronger contrast for the calculator's LCD |
| GRAY | Neutral grayscale |
| INVERTED | Former DARK: light green on dark green, with the original colors |
| AYU DARK | Deep blue-gray, warm amber for X and menus, blue for Y |
| MONOKAI | Charcoal, with purple T, cyan Z, green Y and pink X |
| PASTEL | Light panel, purple/blue/teal/berry stack rows, pastel menu keys |
| COMMODORE | Near-black with phosphor-green text, brighter X and dark green menus |

The original four saved preset indices stay in order, so a previously
saved DARK selection becomes INVERTED. The new presets append to the cycle;
they use the existing settings appvar without a format change.

The new palettes separate status-bar and menu colors from stack colors.
Numbers, unit labels and scroll arrows use the same row color. VARS
selection, HELP, messages and modifier badges retain contrasting colors.
PASTEL uses color consistently by position, with text and register labels
continuing to identify each function.

AYU DARK draws from the [official Ayu palette](https://github.com/ayu-theme/vscode-ayu/blob/master/ayu-dark.json).
MONOKAI draws from the [classic Monokai palette used by VS Code](https://github.com/microsoft/vscode/blob/main/extensions/theme-monokai/themes/monokai-color-theme.json).
Small labels and Monokai's pink are brighter for readability on the CE.
The four added presets' text/background pairs exceed 4.5:1 calculated
contrast after the same five-bit RGB conversion used by GraphX. This is a
palette check; actual LCD appearance still needs hardware testing.

A host-rendered preview is available locally at `bin/tints-preview.png`.
COMMODORE also has its own preview at `bin/commodore-preview.png`.
The previews use the actual display layout and numeric dot renderer with a substitute
font for GraphX's small system text; they are not calculator screenshots.
