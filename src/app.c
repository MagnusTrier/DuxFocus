#include "app.h"
#include "clay.h"
#include "raylib.h"
#include "ui_components.h"

Clay_RenderCommandArray CreateRenderCommandArray(app_data_t *data,
                                                 bool *exit_requested) {
  data->frameArena.offset = 0;
  Clay_BeginLayout();

  SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  create_ui(data, exit_requested);

  Clay_RenderCommandArray renderCommands = Clay_EndLayout(GetFrameTime());
  for (int32_t i = 0; i < renderCommands.length; ++i) {
    Clay_RenderCommandArray_Get(&renderCommands, i)->boundingBox.y +=
        data->yOffset;
  };
  return renderCommands;
}

Clay_RenderCommandArray create_layout(Clay_Context *context, app_data_t *data,
                                      bool *exit_requested) {
  Clay_SetCurrentContext(context);

  Vector2 scale = GetWindowScaleDPI();
  static bool initialized = false;
  if (!initialized || IsWindowResized()) {
    Clay_SetLayoutDimensions(
        (Clay_Dimensions){.width = GetScreenWidth() / scale.x,
                          .height = GetScreenHeight() / scale.y});
    initialized = true;
  }

  Vector2 mousePosition = GetMousePosition();
  bool isInsideWindow = IsCursorOnScreen() && IsWindowFocused();

  if (isInsideWindow) {
    mousePosition.x /= scale.x;
    mousePosition.y /= scale.y;

    mousePosition.y -= data->yOffset;

    Vector2 scrollDelta = GetMouseWheelMoveV();
    Clay_SetPointerState((Clay_Vector2){mousePosition.x, mousePosition.y},
                         IsMouseButtonDown(0));
    Clay_UpdateScrollContainers(
        true, (Clay_Vector2){scrollDelta.x, scrollDelta.y}, GetFrameTime());
  } else {
    Clay_SetPointerState((Clay_Vector2){-100.0f, -100.0f}, false);
  }

  return CreateRenderCommandArray(data, exit_requested);
}
