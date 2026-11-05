#include "fonts.h"
#include "raylib.h"

static const int FONT_SIZES[SIZE_SIZE_COUNT] = {12, 16, 20, 24, 32};
static const char *WEIGHT_FILES[WEIGHT_COUNT] = {
    "resources/fonts/JetBrainsMonoNerdFont-Regular.ttf",
    "resources/fonts/JetBrainsMonoNerdFont-Medium.ttf",
    "resources/fonts/JetBrainsMonoNerdFont-Bold.ttf",
    "resources/fonts/JetBrainsMonoNerdFont-ExtraBold.ttf"};

void load_fonts(Font fonts_out[TOTAL_FONTS]) {
  for (int s = 0; s < SIZE_SIZE_COUNT; ++s) {
    for (int w = 0; w < WEIGHT_COUNT; ++w) {
      int font_id = get_font_id((font_size_t)s, (font_weight_t)w);
      int px_size = FONT_SIZES[s];

      fonts_out[font_id] = LoadFontEx(WEIGHT_FILES[w], px_size, NULL, 0);
      SetTextureFilter(fonts_out[font_id].texture, TEXTURE_FILTER_BILINEAR);
    }
  }
}

void unload_fonts(Font fonts_out[TOTAL_FONTS]) {
  for (int i = 0; i < TOTAL_FONTS; ++i) {
    UnloadFont(fonts_out[i]);
  }
}
