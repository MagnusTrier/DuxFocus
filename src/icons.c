#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "icons.h"
#include "nanosvg.h"
#include "nanosvgrast.h"
#include "raylib.h"

#include "hashmap.h"

typedef struct {
  icon_id_t id;
  int target_size;
  Clay_Color tint;
  float stroke_width;
  int supersample;
  Texture2D texture;
  bool occupied;
} cached_icon;

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

typedef struct {
  icon_id_t id;
  int target_size;
  float tint[4];
  float stroke_width;
  int supersample;
} icon_key_t;

typedef struct {
  Texture2D *tex;
} icon_value_t;

static hashmap_t icons;

static uint32_t f32_bits(float f) {
  uint32_t u;
  memcpy(&u, &f, sizeof(u));
  return u;
}

static uint32_t icon_key_hash(const void *kp, void *user) {
  (void)user;
  const icon_key_t *k = kp;
  uint32_t h = 2166136261u;
  h = (h ^ (uint32_t)k->id) * 16777619u;
  h = (h ^ (uint32_t)k->target_size) * 16777619u;
  h = (h ^ (uint32_t)k->supersample) * 16777619u;
  h = (h ^ f32_bits(k->stroke_width)) * 16777619u;
  for (int i = 0; i < 4; ++i) {
    h = (h ^ f32_bits(k->tint[i])) * 16777619u;
  }
  return h;
}

static bool icon_key_eq(const void *a, const void *b, void *user) {
  (void)user;
  const icon_key_t *x = a;
  const icon_key_t *y = b;

  if (x->id != y->id || x->target_size != y->target_size ||
      x->supersample != y->supersample) {
    return false;
  }

  if (f32_bits(x->stroke_width) != f32_bits(y->stroke_width)) {
    return false;
  }

  for (int i = 0; i < 4; ++i) {
    if (f32_bits(x->tint[i]) != f32_bits(y->tint[i])) {
      return false;
    }
  }
  return true;
}

static void unload_value(const void *k, void *v, void *user) {
  (void)k;
  (void)user;
  icon_value_t *iv = v;
  if (iv->tex) {
    if (iv->tex->id) {
      UnloadTexture(*iv->tex);
      free(iv->tex);
    }
  }
}

Texture2D *get_icon(icon_id_t id, int target_size, Clay_Color tint,
                    float stroke_width, int supersample) {
  if (id < 0 || id >= ICON_COUNT) {
    return NULL;
  }

  if (!icons.capacity) {
    hashmap_init(&icons, sizeof(icon_key_t), sizeof(icon_value_t),
                 icon_key_hash, icon_key_eq, NULL);
  }

  icon_key_t key = {.id = id,
                    .target_size = target_size,
                    .tint = {tint.r, tint.g, tint.b, tint.a},
                    .stroke_width = stroke_width,
                    .supersample = supersample};

  icon_value_t *hit = hashmap_get(&icons, &key);
  if (hit) {
    return hit->tex;
  }

  Texture2D *tex = malloc(sizeof *tex);
  if (!tex) {
    return NULL;
  }

  *tex = load_svg_icon(icon_paths[id], target_size, tint, stroke_width,
                       supersample);
  if (!tex->id) {
    free(tex);
    return NULL;
  }

  icon_value_t val = {.tex = tex};
  icon_value_t *slot = hashmap_put(&icons, &key, &val, NULL);
  if (!slot) {
    UnloadTexture(*tex);
    return NULL;
  }

  return tex;
}

void icons_system_unload(void) {
  hashmap_foreach(&icons, unload_value, NULL);
  hashmap_destroy(&icons);
}
