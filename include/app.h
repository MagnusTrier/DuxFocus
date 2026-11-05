#ifndef APP_H
#define APP_H

#include "api.h"
#include "clay.h"
#include <stdint.h>

typedef struct {
  intptr_t offset;
  intptr_t memory;
} Arena;

typedef struct {
  float yOffset;
  Arena frameArena;
  api_request_t api_request;
} app_data_t;

Clay_RenderCommandArray create_layout(Clay_Context *context, app_data_t *data,
                                      bool *exit_requested);

#endif
