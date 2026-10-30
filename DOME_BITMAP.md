DomeD bitmap font fork
====================

This branch merges upstream HarfBuzz 14.1.0 into the published DomeD fork,
retaining the earlier fork commits. Native Unicode BDF and PCF faces are opened
through FreeType, retaining strike pixels, character maps and glyph metrics.
Outline and table-only faces are unavailable in this build.

`hb_dome_bitmap_only()` returns true; `hb_dome_bitmap_formats()` returns 3
(BDF | PCF). The historical BDF-only probe returns false because PCF is now
supported. Shared library ABI and the upstream companion libraries are retained.

DomeD carries the matching FreeType, Cairo and Pango patches, reproducible build
instructions and regression tests in native/bitmap-runtime.
