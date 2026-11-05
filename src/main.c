#include "raylib.h"
#include <stddef.h>
#define CLAY_IMPLEMENTATION

#include "api.h"
#include "app.h"
#include "clay.h"
#include "clay_renderer_raylib.c"
#include "config.h"
#include "fonts.h"
#include "icons.h"
#include <stdio.h>

#ifdef _WIN32
void InitWin32Drag(void);
#endif

void HandleClayErrors(Clay_ErrorData errorData) {
  printf("%s", errorData.errorText.chars);
}

int main(void) {
  Clay_Raylib_Initialize(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE,
                         FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI |
                             FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT |
                             FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT);
#ifdef _WIN32
  InitWin32Drag();
#endif

  Font fonts[TOTAL_FONTS] = {0};
  load_fonts(fonts);

  uint64_t clay_required_memory = Clay_MinMemorySize();
  Clay_Arena clay_memory_top = Clay_CreateArenaWithCapacityAndMemory(
      clay_required_memory, malloc(clay_required_memory));
  Clay_Context *clay_context_top = Clay_Initialize(
      clay_memory_top,
      (Clay_Dimensions){.width = GetScreenWidth(), .height = GetScreenHeight()},
      (Clay_ErrorHandler){HandleClayErrors});

  size_t arena_size = 64 * 1024;
  void *arena_mem = malloc(arena_size);
  memset(arena_mem, 0, arena_size);
  app_data_t data_top = {.frameArena = {.memory = (intptr_t)arena_mem}};

  api_request_start(&data_top.api_request, "https://httpbin.org/json");
  Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

  bool exit_requested = false;

  while (!WindowShouldClose() && !exit_requested) {
    api_request_poll(&data_top.api_request);
    Clay_RenderCommandArray render_commands_top =
        create_layout(clay_context_top, &data_top, &exit_requested);
    BeginDrawing();
    ClearBackground(BLANK);
    Clay_Raylib_Render(render_commands_top, fonts);
    EndDrawing();
  }
  icons_system_unload();
  api_request_free(&data_top.api_request);
  Clay_Raylib_Close();
}
