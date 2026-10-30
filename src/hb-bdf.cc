/* Native BDF support for DomeD. Same MIT-style license as HarfBuzz.
 * Raw Unicode bitmap strikes only: no SFNT wrappers, outlines or resampling. */
#include "hb.hh"
#include "hb-bdf.hh"
#include <climits>
#include FT_FONT_FORMATS_H

struct bdf_data_t {
  FT_Library library = nullptr;
  FT_Face face = nullptr;
  hb_blob_t *blob = nullptr;
  hb_mutex_t mutex;
};
static hb_user_data_key_t bdf_key, ft_owner_key;
static void destroy_bdf (void *p) {
  auto *d = static_cast<bdf_data_t *> (p);
  if (d->face) FT_Done_Face (d->face);
  if (d->library) FT_Done_FreeType (d->library);
  hb_blob_destroy (d->blob);
  d->~bdf_data_t (); hb_free (d);
}
static hb_blob_t *reference_bdf (hb_face_t *, hb_tag_t tag, void *p) {
  auto *d = static_cast<bdf_data_t *> (p);
  return tag == HB_TAG_NONE ? hb_blob_reference (d->blob) : hb_blob_get_empty ();
}
hb_face_t *_hb_bdf_face_create (hb_blob_t *blob, unsigned index) {
  if (!blob || index || !hb_blob_get_length (blob)) return hb_face_get_empty ();
  void *storage = hb_malloc (sizeof (bdf_data_t));
  auto *d = storage ? new (storage) bdf_data_t : nullptr;
  if (!d) return hb_face_get_empty ();
  d->blob = hb_blob_reference (blob);
  unsigned length = 0;
  const char *bytes = hb_blob_get_data (blob, &length);
  if (FT_Init_FreeType (&d->library) ||
      FT_New_Memory_Face (d->library, reinterpret_cast<const FT_Byte *> (bytes), length, 0, &d->face) ||
      !FT_Get_Font_Format (d->face) || strcmp (FT_Get_Font_Format (d->face), "BDF") ||
      FT_IS_SCALABLE (d->face) || d->face->num_fixed_sizes != 1 ||
      FT_Select_Charmap (d->face, FT_ENCODING_UNICODE) || FT_Select_Size (d->face, 0) ||
      !d->face->size->metrics.y_ppem || d->face->num_glyphs <= 0) {
    destroy_bdf (d); return hb_face_get_empty ();
  }
  hb_face_t *face = _hb_face_create_for_bitmap (reference_bdf, d, destroy_bdf);
  if (face == hb_face_get_empty ()) return face;
  if (!hb_face_set_user_data (face, &bdf_key, d, nullptr, false)) {
    hb_face_destroy (face); return hb_face_get_empty ();
  }
  hb_face_set_glyph_count (face, d->face->num_glyphs);
  hb_face_set_upem (face, d->face->size->metrics.y_ppem);
  return face;
}
hb_face_t *_hb_bdf_face_from_ft (FT_Face ft, hb_destroy_func_t destroy) {
  hb_face_t *face = hb_face_get_empty ();
  if (ft && ft->stream && !FT_IS_SCALABLE (ft) && FT_Get_Font_Format (ft) &&
      !strcmp (FT_Get_Font_Format (ft), "BDF") && ft->stream->size <= UINT_MAX) {
    unsigned n = ft->stream->size;
    char *bytes = static_cast<char *> (hb_malloc (n));
    if (bytes) {
      bool read = false;
      if (ft->stream->read) read = ft->stream->read (ft->stream, 0, reinterpret_cast<FT_Byte *> (bytes), n) == n;
      else if (ft->stream->base) { memcpy (bytes, ft->stream->base, n); read = true; }
      if (read) {
        hb_blob_t *blob = hb_blob_create (bytes, n, HB_MEMORY_MODE_READONLY, bytes, hb_free);
        face = _hb_bdf_face_create (blob, 0);
        hb_blob_destroy (blob);
      } else hb_free (bytes);
    }
  }
  if (destroy) {
    if (face == hb_face_get_empty ()) destroy (ft);
    else if (!hb_face_set_user_data (face, &ft_owner_key, ft, destroy, false)) {
      hb_face_destroy (face); destroy (ft); return hb_face_get_empty ();
    }
  }
  return face;
}
static hb_position_t scaled (FT_Pos value, int scale, unsigned ppem) {
  if (!ppem) return 0;
  int64_t result = int64_t (value) * scale / (64 * ppem);
  return static_cast<hb_position_t> (hb_clamp (result, int64_t (INT_MIN), int64_t (INT_MAX)));
}
static hb_bool_t nominal (hb_font_t *, void *p, hb_codepoint_t cp, hb_codepoint_t *glyph, void *) {
  auto *d = static_cast<bdf_data_t *> (p);
  d->mutex.lock (); *glyph = FT_Get_Char_Index (d->face, cp); d->mutex.unlock ();
  return *glyph != 0;
}
static hb_position_t advance (hb_font_t *font, void *p, hb_codepoint_t glyph, void *) {
  auto *d = static_cast<bdf_data_t *> (p); int x, y; hb_font_get_scale (font, &x, &y);
  d->mutex.lock ();
  hb_position_t n = FT_Load_Glyph (d->face, glyph, FT_LOAD_DEFAULT) ? 0 :
    scaled (d->face->glyph->advance.x, x, d->face->size->metrics.y_ppem);
  d->mutex.unlock (); return n;
}
static hb_bool_t extents (hb_font_t *font, void *p, hb_codepoint_t glyph, hb_glyph_extents_t *out, void *) {
  auto *d = static_cast<bdf_data_t *> (p); int x, y; hb_font_get_scale (font, &x, &y);
  d->mutex.lock ();
  bool ok = !FT_Load_Glyph (d->face, glyph, FT_LOAD_DEFAULT);
  if (ok) {
    auto &m = d->face->glyph->metrics;
    unsigned xp = d->face->size->metrics.y_ppem, yp = xp;
    out->x_bearing = scaled (m.horiBearingX, x, xp);
    out->y_bearing = scaled (m.horiBearingY, y, yp);
    out->width = scaled (m.width, x, xp); out->height = -scaled (m.height, y, yp);
  }
  d->mutex.unlock (); return ok;
}
static hb_bool_t font_extents (hb_font_t *font, void *p, hb_font_extents_t *out, void *) {
  auto *d = static_cast<bdf_data_t *> (p); int x, y; hb_font_get_scale (font, &x, &y);
  d->mutex.lock (); auto m = d->face->size->metrics;
  out->ascender = scaled (m.ascender, y, m.y_ppem);
  out->descender = scaled (m.descender, y, m.y_ppem);
  out->line_gap = scaled (m.height - m.ascender + m.descender, y, m.y_ppem);
  d->mutex.unlock (); return true;
}
bool _hb_bdf_font_set_funcs (hb_font_t *font) {
  auto *d = static_cast<bdf_data_t *> (hb_face_get_user_data (hb_font_get_face (font), &bdf_key));
  if (!d) return false;
  hb_font_funcs_t *funcs = hb_font_funcs_create ();
  hb_font_funcs_set_nominal_glyph_func (funcs, nominal, nullptr, nullptr);
  hb_font_funcs_set_glyph_h_advance_func (funcs, advance, nullptr, nullptr);
  hb_font_funcs_set_glyph_extents_func (funcs, extents, nullptr, nullptr);
  hb_font_funcs_set_font_h_extents_func (funcs, font_extents, nullptr, nullptr);
  hb_font_funcs_make_immutable (funcs);
  hb_font_set_funcs (font, funcs, d, nullptr);
  hb_font_funcs_destroy (funcs); return true;
}

bool _hb_bdf_collect_unicodes (hb_face_t *face, hb_set_t *out) {
  auto *d = static_cast<bdf_data_t *> (hb_face_get_user_data (face, &bdf_key));
  if (!d) return false;
  d->mutex.lock ();
  FT_UInt glyph;
  for (FT_ULong cp = FT_Get_First_Char (d->face, &glyph); glyph;
       cp = FT_Get_Next_Char (d->face, cp, &glyph)) hb_set_add (out, cp);
  d->mutex.unlock ();
  return true;
}

/* Session policy probe; stock HarfBuzz deliberately has no such symbol. */
hb_bool_t hb_dome_bdf_only (void) { return true; }
