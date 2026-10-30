/* DomeD bitmap-only HarfBuzz fork. Same MIT-style license as HarfBuzz. */
#ifndef HB_BDF_HH
#define HB_BDF_HH
#include "hb.h"
#include <ft2build.h>
#include FT_FREETYPE_H
HB_INTERNAL hb_face_t *_hb_face_create_for_bitmap (hb_reference_table_func_t, void *, hb_destroy_func_t);
HB_INTERNAL hb_face_t *_hb_bdf_face_create (hb_blob_t *, unsigned);
HB_INTERNAL hb_face_t *_hb_bdf_face_from_ft (FT_Face, hb_destroy_func_t);
HB_INTERNAL bool _hb_bdf_collect_unicodes (hb_face_t *, hb_set_t *);
HB_INTERNAL bool _hb_bdf_font_set_funcs (hb_font_t *);
HB_BEGIN_DECLS
HB_EXTERN hb_bool_t hb_dome_bdf_only (void);
HB_EXTERN hb_bool_t hb_dome_bitmap_only (void);
HB_EXTERN unsigned hb_dome_bitmap_formats (void);
HB_END_DECLS
#endif
