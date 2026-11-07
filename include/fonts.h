#ifndef FONTS_H
#define FONTS_H
#include "raylib.h"
#include <stddef.h>
#include "hashmap.h"

typedef enum {
  WEIGHT_REGULAR,
  WEIGHT_MEDIUM,
  WEIGHT_BOLD,
  WEIGHT_EXTRA_BOLD,
  WEIGHT_COUNT
} font_weight_t;

typedef enum { FAMILY_JETBRAINS, FAMILY_COUNT } font_family_t;

Font *get_font(font_family_t family, font_weight_t weight, int size);

void fonts_system_unload(void);

static hashmap_t fonts;

#endif
