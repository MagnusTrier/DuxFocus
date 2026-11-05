#ifndef FONTS_H
#define FONTS_H
#include "raylib.h"
#include <stddef.h>

typedef enum {
  WEIGHT_REGULAR,
  WEIGHT_MEDIUM,
  WEIGHT_BOLD,
  WEIGHT_EXTRA_BOLD,
  WEIGHT_COUNT
} font_weight_t;

typedef enum {
  SIZE_12,
  SIZE_16,
  SIZE_20,
  SIZE_24,
  SIZE_32,
  SIZE_SIZE_COUNT
} font_size_t;

#define TOTAL_FONTS (SIZE_SIZE_COUNT * WEIGHT_COUNT)

static inline int get_font_id(font_size_t size, font_weight_t weight) {
  return (size * WEIGHT_COUNT) + weight;
}

void load_fonts(Font fonts_out[TOTAL_FONTS]);
void unload_fonts(Font fonts_out[TOTAL_FONTS]);

#endif
