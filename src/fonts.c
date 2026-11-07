#include "fonts.h"
#include "hashmap.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

static const char *FONT_FAMILIES[FAMILY_COUNT] = {[FAMILY_JETBRAINS] =
                                                      "JetBrainsMonoNerdFont"};

static const char *FONT_WEIGHTS[WEIGHT_COUNT] = {
    [WEIGHT_BOLD] = "Bold",
    [WEIGHT_EXTRA_BOLD] = "ExtraBold",
    [WEIGHT_MEDIUM] = "Medium",
    [WEIGHT_REGULAR] = "Regular",
};

static Font load_font(font_family_t family, font_weight_t weight, int size) {
  char path[256];
  snprintf(path, sizeof(path), "resources/fonts/%s-%s.ttf",
           FONT_FAMILIES[family], FONT_WEIGHTS[weight]);
  Font f = LoadFontEx(path, size, NULL, 0);
  SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
  return f;
}

typedef struct {
  font_family_t family;
  font_weight_t weight;
  int size;
} font_key_t;

typedef struct {
  Font *f;
} font_value_t;

static hashmap_t fonts;

static uint32_t font_key_hash(const void *kp, void *user) {
  (void)user;
  const font_key_t *k = kp;
  uint32_t h = 2166136261u;
  h = (h ^ (uint32_t)k->family) * 16777619u;
  h = (h ^ (uint32_t)k->size) * 16777619u;
  h = (h ^ (uint32_t)k->weight) * 16777619u;
  return h;
}

static bool font_key_eq(const void *a, const void *b, void *user) {
  (void)user;
  const font_key_t *x = a;
  const font_key_t *y = b;

  if (x->family != y->family || x->size != y->size || x->weight != y->weight) {
    return false;
  }

  return true;
}

static void unload_value(const void *k, void *v, void *user) {
  (void)k;
  (void)user;
  font_value_t *fv = v;
  if (fv->f) {
    UnloadFont(*fv->f);
  }
}

Font *get_font(font_family_t family, font_weight_t weight, int size) {
  if (family < 0 || family >= FAMILY_COUNT || weight < 0 ||
      weight >= WEIGHT_COUNT) {
    return NULL;
  }

  if (!fonts.capacity) {
    hashmap_init(&fonts, sizeof(font_key_t), sizeof(font_value_t),
                 font_key_hash, font_key_eq, NULL);
  }

  font_key_t key = {.family = family, .weight = weight, .size = size};

  font_value_t *hit = hashmap_get(&fonts, &key);
  if (hit) {
    return hit->f;
  }

  Font *f = malloc(sizeof *f);
  if (!f) {
    return NULL;
  }

  *f = load_font(family, weight, size);
  if (!f) {
    free(f);
    return NULL;
  }

  font_value_t val = {.f = f};
  font_value_t *slot = hashmap_put(&fonts, &key, &val, NULL);
  if (!slot) {
    UnloadFont(*f);
    return NULL;
  }

  return f;
}

void fonts_system_unload(void) {
  hashmap_foreach(&fonts, unload_value, NULL);
  hashmap_destroy(&fonts);
}
