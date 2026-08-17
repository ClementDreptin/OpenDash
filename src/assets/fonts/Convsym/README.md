# Convsym patched font

This font ships with the Xbox 360 Software Development Kit, at `%XEDK%\xboxfont\Convsym.ttf`, and provides icons such as the controller buttons mapped from codepoint `U+F020` to `U+F0FD`. Unfortunately, Dear ImGui is not able to load this font as is.

## The problem

Dear ImGui's default font builder (`stb_truetype`) requires a `cmap` subtable with one of the following platform/encoding combinations to resolve `info->index_map`:

- `(3, 1)` - Microsoft Unicode BMP
- `(3, 10)` - Microsoft Unicode Full
- `(0, x)` - Unicode platform

`Convsym.ttf` only ships two `cmap` subtables:

- `(1, 0)` - Mac Roman
- `(3, 0)` - Microsoft Symbol encoding

## The solution

The base font file was patched, a duplicate `cmap` subtable was added with the same codepoint mappings as the existing `(3, 0)` Symbol subtable, but declared as `(3, 1)` Unicode BMP (the encoding `stb_truetype` actually recognizes).

The following python script was used to patch the TTF file.

```py
"""
Requires fonttools:
pip install fonttools

This script was written by Claude Sonnet 5.
"""

import copy
from fontTools.ttLib import TTFont

# Load the original font.
font = TTFont("Convsym.ttf")
cmap_table = font["cmap"]

# Find the existing (3, 0) Symbol subtable.
symbol_subtable = next(st for st in cmap_table.tables
                        if st.platformID == 3 and st.platEncID == 0)

# Clone it and just flip the encoding ID to (3, 1) Unicode BMP.
new_subtable = copy.deepcopy(symbol_subtable)
new_subtable.platEncID = 1
cmap_table.tables.append(new_subtable)

# Save the patched font.
font.save("Convsym_patched.ttf")
```
