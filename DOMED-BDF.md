# DomeD BDF-only HarfBuzz

Forked from HarfBuzz 12.3.2 (b42511e071162fe76102f613a6ccc009726c99af).

This fork accepts native Unicode BDF bitmap strikes through blob and FreeType
face/font construction. Glyph IDs, coverage, advances and ink extents come from
FreeType without SFNT conversion or bitmap resampling. Font metrics scale in
HarfBuzz units. The normal HarfBuzz shaping/cluster machinery remains available.

Outline fonts, collections and public table-callback face construction return
empty faces. The Graphite2 backend is disabled in the supported build. Arbitrary
client-defined font callbacks are not a security boundary.

DomeD maintains the paired Pango patch, pinned build recipe and raw-glyph,
outline-rejection and pixel regression tests in native/harfbuzz-bdf and scripts
of https://github.com/jebot-git/DomeD. The hb_dome_bdf_only() policy probe returns
true; DomeD sessions require that probe and the recorded runtime checksums.

The original HarfBuzz license in COPYING applies to these changes as well.
