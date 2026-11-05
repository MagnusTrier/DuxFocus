#include <assert.h>
#include <stdint.h>
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "config.h"
#include "icons.h"
#include "nanosvg.h"
#include "nanosvgrast.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  icon_id_t id;
  int target_size;
  Clay_Color tint;
  float stroke_width;
  int supersample;
  Texture2D texture;
  bool occupied;
} cached_icon;

typedef struct {
  size_t length;
  size_t capacity;
  cached_icon *cache;
} icon_cache_table_t;

static icon_cache_table_t icon_cache = {0};

static const char *icon_paths[ICON_COUNT] = {
    [ICON_CLOSE] = "resources/icons/x.svg",
    [ICON_GHOST] = "resources/icons/ghost.svg",
    [ICON_MINIMIZE] = "resources/icons/minimize.svg",
    [ICON_MAXIMIZE] = "resources/icons/maximize.svg",
    [ICON_LOGO] = "resources/logo.svg",
    [ICON_MINUS] = "resources/icons/minus.svg",
    [ICON_DRAFTING_COMPASS] = "resources/icons/drafting-compass.svg",
};

static Texture2D load_svg_icon(const char *path, int target_size,
                               Clay_Color tint, float stroke_width,
                               int supersample) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "load_svg_icon: failed to open %s\n", path);
    return (Texture2D){0};
  }

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (len <= 0) {
    fclose(f);
    return (Texture2D){0};
  }

  char *src = malloc((size_t)len + 1);
  if (!src) {
    fclose(f);
    return (Texture2D){0};
  }

  size_t read = fread(src, 1, (size_t)len, f);
  fclose(f);
  src[read] = '\0';

  char hex[8];
  snprintf(hex, sizeof(hex), "#%02x%02x%02x", (unsigned)tint.r,
           (unsigned)tint.g, (unsigned)tint.b);

  const char *needle = "currentColor";
  size_t needle_len = strlen(needle);
  size_t hex_len = strlen(hex);
  size_t count = 0;
  for (const char *p = src; (p = strstr(p, needle)) != NULL; p += needle_len) {
    ++count;
  }

  size_t new_len = (size_t)read + count * (hex_len - needle_len) + 1;
  char *replaced = malloc(new_len);
  if (!replaced) {
    free(src);
    return (Texture2D){0};
  }

  const char *in = src;
  char *out = replaced;
  const char *hit;
  while ((hit = strstr(in, needle)) != NULL) {
    size_t chunk = (size_t)(hit - in);
    memcpy(out, in, chunk);
    out += chunk;
    memcpy(out, hex, hex_len);
    out += hex_len;
    in = hit + needle_len;
  }

  strcpy(out, in);
  free(src);

  NSVGimage *image = nsvgParse(replaced, "px", 96.0f);
  free(replaced);
  if (!image) {
    fprintf(stderr, "load_svg_icon: fialed to parse %s\n", path);
    return (Texture2D){0};
  }

  if (stroke_width > 0.0f) {
    for (NSVGshape *shape = image->shapes; shape != NULL; shape = shape->next) {
      shape->strokeWidth = stroke_width;
    }
  }

  int w = target_size * supersample;
  int h = target_size * supersample;
  if (image->width <= 0.0f || image->height <= 0.0f) {
    fprintf(stderr, "load_svg_icon: %s has no valid dimensions\n", path);
    nsvgDelete(image);
    return (Texture2D){0};
  }
  float scale = (float)w / image->width;
  float scale_h = (float)h / image->height;
  if (scale_h < scale)
    scale = scale_h;

  NSVGrasterizer *rast = nsvgCreateRasterizer();
  if (!rast) {
    nsvgDelete(image);
    return (Texture2D){0};
  }

  unsigned char *pixels = calloc(1, (size_t)w * h * 4);
  if (!pixels) {
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    return (Texture2D){0};
  }

  nsvgRasterize(rast, image, 0, 0, scale, pixels, w, h, w * 4);
  nsvgDeleteRasterizer(rast);
  nsvgDelete(image);

  Image img = {
      .data = pixels,
      .width = w,
      .height = h,
      .mipmaps = 1,
      .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
  };
  Texture2D tex = LoadTextureFromImage(img);
  UnloadImage(img);
  SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);

  return tex;
}

static uint32_t hash_icon_key(icon_id_t id, int target_size, Clay_Color tint,
                              float stroke_width, int supersample) {
  uint32_t hash = 2166136261u;
  const uint32_t hash_power = 16777619u;

  hash = (hash ^ (uint32_t)id) * hash_power;
  hash = (hash ^ (uint32_t)target_size) * hash_power;

  hash = (hash ^ *(uint32_t *)&tint.r) * hash_power;
  hash = (hash ^ *(uint32_t *)&tint.g) * hash_power;
  hash = (hash ^ *(uint32_t *)&tint.b) * hash_power;
  hash = (hash ^ *(uint32_t *)&tint.a) * hash_power;

  hash = (hash ^ *(uint32_t *)&stroke_width) * hash_power;
  hash = (hash ^ (uint32_t)supersample) * hash_power;

  return hash;
}

void icons_system_unload(void) {
  for (size_t i = 0; i < icon_cache.capacity; ++i) {
    cached_icon *entry = &icon_cache.cache[i];
    if (entry->occupied && entry->texture.id != 0) {
      UnloadTexture(entry->texture);
    }
  }
  free(icon_cache.cache);
  memset(&icon_cache, 0, sizeof(icon_cache));
}

cached_icon *insert(cached_icon *cache, size_t capacity, cached_icon *entry) {

  size_t hash = hash_icon_key(entry->id, entry->target_size, entry->tint,
                              entry->stroke_width, entry->supersample);
  size_t index = hash & (capacity - 1);
  for (size_t j = 0; j < capacity; ++j) {
    if (!cache[index].occupied) {
      cache[index] = *entry;
      return &cache[index];
    } else {
      index = (index + 1) & (capacity - 1);
    }
  }
  fprintf(stderr, "Icon insertion into cached failed");
  return NULL;
}

static void change_capacity(size_t new_capacity) {
  assert((new_capacity & (new_capacity - 1)) == 0 && new_capacity > 0 &&
         new_capacity > icon_cache.length);

  cached_icon *new_cache = calloc(new_capacity, sizeof(cached_icon));

  size_t count = 0;

  for (size_t i = 0; i < icon_cache.capacity; ++i) {
    cached_icon *entry = &icon_cache.cache[i];
    if (count == icon_cache.length) {
      break;
    }

    if (entry->occupied) {
      ++count;
      if (!insert(new_cache, new_capacity, entry)) {
        fprintf(stderr, "change_capacity: rehash insert failed for icon %d\n",
                entry->id);
        abort();
      }
    }
  }

  icon_cache.capacity = new_capacity;
  free(icon_cache.cache);
  icon_cache.cache = new_cache;
}

static void initialize_icon_cache_table(void) { change_capacity(256); }

Texture2D *get_icon(icon_id_t id, int target_size, Clay_Color tint,
                    float stroke_width, int supersample) {
  if (icon_cache.capacity == 0) {
    initialize_icon_cache_table();
  }
  size_t hash = hash_icon_key(id, target_size, tint, stroke_width, supersample);
  size_t index = hash & (icon_cache.capacity - 1);

  for (size_t i = 0; i < icon_cache.capacity; ++i) {
    cached_icon *entry = &icon_cache.cache[index];

    if (!entry->occupied) {
      if (id < 0 || id >= ICON_COUNT) {
        fprintf(stderr, "Error: Invalid icon ID %d\n", id);
        return NULL;
      }

      if (icon_cache.length > icon_cache.capacity * 0.7) {
        change_capacity(icon_cache.capacity * 2);
      }

      Texture2D new_texture = load_svg_icon(icon_paths[id], target_size, tint,
                                            stroke_width, supersample);
      if (new_texture.id == 0) {
        return NULL;
      }

      cached_icon *new_entry =
          insert(icon_cache.cache, icon_cache.capacity,
                 &(cached_icon){.id = id,
                                .target_size = target_size,
                                .tint = tint,
                                .stroke_width = stroke_width,
                                .texture = new_texture,
                                .supersample = supersample,
                                .occupied = true});

      if (!new_entry) {
        UnloadTexture(new_texture);
        return NULL;
      }

      ++icon_cache.length;
      return &new_entry->texture;
    }

    if (entry->id == id && entry->target_size == target_size &&
        entry->tint.r == tint.r && entry->tint.g == tint.g &&
        entry->tint.b == tint.b && entry->tint.a == tint.a &&
        entry->stroke_width == stroke_width &&
        entry->supersample == supersample) {
      return &entry->texture;
    }

    index = (index + 1) & (icon_cache.capacity - 1);
  }

  fprintf(stderr, "Error: Icon cache table is full!\n");
  return NULL;
}
