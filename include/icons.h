#ifndef ICONS_H
#define ICONS_H

#include "clay.h"
#include "raylib.h"

typedef enum {
  ICON_CLOSE,
  ICON_GHOST,
  ICON_DRAFTING_COMPASS,
  ICON_MINIMIZE,
  ICON_MAXIMIZE,
  ICON_LOGO,
  ICON_MINUS,
  ICON_COUNT,

} icon_id_t;

Texture2D *get_icon(icon_id_t id, int target_size, Clay_Color tint,
                    float stroke_width, int supersample);

void icons_system_unload(void);

#endif
