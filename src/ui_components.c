#include "ui_components.h"
#include "app.h"
#include "config.h"
#include "fonts.h"
#include "icons.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

static Clay_TextElementConfig text_defaults(void) {
  return (Clay_TextElementConfig){.fontId =
                                      get_font_id(SIZE_20, WEIGHT_EXTRA_BOLD),
                                  .letterSpacing = 0,
                                  .fontSize = 20,
                                  .textColor = COLOR_TEXT,
                                  .textAlignment = CLAY_TEXT_ALIGN_CENTER};
}

static Clay_ElementDeclaration button_defaults(void) {
  return (Clay_ElementDeclaration){
      .layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT,
                 .childAlignment = {.y = CLAY_ALIGN_Y_CENTER,
                                    .x = CLAY_ALIGN_X_CENTER},
                 .sizing = {.width = CLAY_SIZING_FIT(0),
                            .height = CLAY_SIZING_FIT(0)}},
  };
}

typedef void (*button_click_fn)(Clay_ElementId id, void *user_data);

typedef struct {
  button_click_fn fn;
  void *user_data;
} button_click_ctx;

static void button_click_bridge(Clay_ElementId id, Clay_PointerData data,
                                void *ctx_raw) {
  if (data.state != CLAY_POINTER_DATA_RELEASED_THIS_FRAME)
    return;
  button_click_ctx *ctx = (button_click_ctx *)ctx_raw;
  if (ctx && ctx->fn)
    ctx->fn(id, ctx->user_data);
}

typedef struct {
  bool set_padding;
  Clay_Padding padding;

  bool set_background;
  Clay_Color background;

  bool set_sizing;
  Clay_Sizing sizing;

  bool set_aspect_ratio;
  Clay_AspectRatioElementConfig aspect_ratio;
} button_overrides_t;

typedef struct {
  Clay_ElementId id;
  Clay_String text;
  Texture2D *icon;
  Texture2D *hover_icon;
  Clay_TextElementConfig text_overrides;
  Clay_CornerRadius *corner_radius;
  Clay_Sizing *sizing;
  Clay_Sizing *icon_sizing;
  Clay_Color hover_bg;
  button_click_ctx *ctx;
} button_props;

void button_component(button_props props) {
  Clay_ElementDeclaration config = button_defaults();
  Clay_TextElementConfig text_config = text_defaults();

  bool hovered = Clay_PointerOver(props.id);

  if (props.corner_radius) {
    config.cornerRadius = *props.corner_radius;
  }

  if (props.sizing) {
    config.layout.sizing = *props.sizing;
  }

  if (props.hover_bg.a > 0 && hovered) {
    config.backgroundColor = props.hover_bg;
  }

  if (props.text_overrides.fontId && props.text_overrides.fontSize) {
    text_config.fontId = props.text_overrides.fontId;
    text_config.fontSize = props.text_overrides.fontSize;
  }

  if (hovered) {
    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
  }

  Texture2D *tex = NULL;
  if (hovered && props.hover_icon && props.hover_icon->id != 0) {
    tex = props.hover_icon;
  } else {
    tex = props.icon;
  }

  CLAY(props.id, config) {
    if (props.ctx && props.ctx->fn) {
      Clay_OnHover(button_click_bridge, props.ctx);
    }

    if (tex && tex->id != 0) {
      CLAY_AUTO_ID({.layout = {.sizing = props.icon_sizing ? *props.icon_sizing :
                                   (Clay_Sizing){
                                       .width = CLAY_SIZING_FIXED(ICON_SIZE),
                                       .height = CLAY_SIZING_FIXED(ICON_SIZE),
                                   }},
                    .image = {.imageData = tex}}) {}
    }
    if (!tex || tex->id == 0) {
      CLAY_TEXT(props.text, text_config);
    }
  }
}

static void on_minimize_click(Clay_ElementId id, void *user_data) {
  MinimizeWindow();
};

static void on_maximize_click(Clay_ElementId id, void *user_data) {
  if (IsWindowMaximized()) {
    RestoreWindow();
  } else {
    MaximizeWindow();
  }
}

static void on_close_click(Clay_ElementId id, void *user_data) {
  *(bool *)user_data = true;
}

void title_bar(bool *exit_requested) {
  static const int bar_height = 50;
  static const float icon_width = bar_height * .8 / .9;
  CLAY(CLAY_ID("TitleBar"),
       {.layout = {.childGap = 1,
                   .sizing = {.width = CLAY_SIZING_GROW(0),
                              .height = CLAY_SIZING_FIXED(bar_height)},
                   .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                   .padding = {.left = 14},
                   .layoutDirection = CLAY_LEFT_TO_RIGHT},
        .border = {.color = COLOR_SURFACE_1, .width = {.bottom = 1}}}) {

    Texture2D *logo = get_icon(ICON_LOGO, 40, COLOR_BASE, 0.0f, 2);
    if (logo) {
      CLAY_AUTO_ID({.layout =
                        {
                            .sizing =
                                {
                                    .width = CLAY_SIZING_FIXED(40),
                                    .height = CLAY_SIZING_FIXED(40),
                                },
                        },
                    .image = {.imageData = logo}}) {}
    }

    CLAY(CLAY_ID("SmallSpacer"),
         {.layout = {.sizing = {.width = CLAY_SIZING_FIXED(8)}}}) {}
    CLAY_TEXT(CLAY_STRING(WINDOW_TITLE), text_defaults());
    CLAY(CLAY_ID("TitleBarSpacer"),
         {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0)}}}) {}

    static button_click_ctx minimize_ctx = {on_minimize_click, NULL};
    Texture2D *minimize_icon =
        get_icon(ICON_MINUS, 20,
                 Clay_PointerOver(CLAY_ID("btnMinimize")) ? COLOR_OVERLAY_2
                                                          : COLOR_OVERLAY_0,
                 1.5f, 4);
    if (minimize_icon) {
      button_component((button_props){
          .id = CLAY_ID("btnMinimize"),
          .icon = minimize_icon,
          .icon_sizing = &(Clay_Sizing){.width = CLAY_SIZING_FIXED(20),
                                        .height = CLAY_SIZING_FIXED(20)},
          .hover_bg = COLOR_MANTLE,
          .sizing = &(Clay_Sizing){.width = CLAY_SIZING_FIXED(icon_width),
                                   .height = CLAY_SIZING_FIXED(bar_height)},
          .ctx = &minimize_ctx});
    }

    static button_click_ctx maximize_ctx = {on_maximize_click, NULL};
    Texture2D *maximize_icon =
        get_icon(IsWindowMaximized() ? ICON_MINIMIZE : ICON_MAXIMIZE, 20,
                 Clay_PointerOver(CLAY_ID("btnMaximize")) ? COLOR_OVERLAY_2
                                                          : COLOR_OVERLAY_0,
                 2.0f, 4);
    if (maximize_icon) {
      button_component((button_props){
          .id = CLAY_ID("btnMaximize"),
          .icon = maximize_icon,
          .hover_bg = COLOR_MANTLE,
          .sizing = &(Clay_Sizing){.width = CLAY_SIZING_FIXED(icon_width),
                                   .height = CLAY_SIZING_FIXED(bar_height)},
          .icon_sizing = &(Clay_Sizing){.width = CLAY_SIZING_FIXED(20),
                                        .height = CLAY_SIZING_FIXED(20)},
          .ctx = &maximize_ctx});
    }

    static button_click_ctx close_ctx = {on_close_click, NULL};
    close_ctx.user_data = exit_requested;
    Texture2D *close_icon = get_icon(
        ICON_CLOSE, 24,
        Clay_PointerOver(CLAY_ID("btnClose")) ? COLOR_MANTLE : COLOR_OVERLAY_0,
        16.0f, 4);
    if (close_icon) {
      button_component((button_props){
          .id = CLAY_ID("btnClose"),
          .icon = close_icon,
          .icon_sizing = &(Clay_Sizing){.width = CLAY_SIZING_FIXED(24),
                                        .height = CLAY_SIZING_FIXED(24)},
          .corner_radius = &(Clay_CornerRadius){.topRight = 12},
          .sizing = &(Clay_Sizing){.width = CLAY_SIZING_FIXED(icon_width),
                                   .height = CLAY_SIZING_FIXED(bar_height)},
          .hover_bg = COLOR_RED,
          .ctx = &close_ctx});
    }
  }
}

void side_bar() {
  CLAY(CLAY_ID("sideBar"),
       {.layout = {.sizing = {.width = CLAY_SIZING_FIXED(240),
                              .height = CLAY_SIZING_GROW(0)}},
        .border = {.color = COLOR_SURFACE_1, .width = {.right = 1}}}) {}
}

void main_content(app_data_t *data) {

  Clay_Sizing layout_expand = {
      .width = CLAY_SIZING_GROW(0),
      .height = CLAY_SIZING_GROW(0),
  };
  CLAY(CLAY_ID("mainContent"),
       {.backgroundColor = COLOR_MANTLE, .layout = layout_expand}) {
    const char *label = "";
    switch (data->api_request.state) {
    case API_IDLE:
      label = "Not started";
      break;
    case API_IN_FLIGHT:
      label = "Loading…";
      break;
    case API_DONE:
      label = "Loaded";
      break;
    case API_FAILED:
      label = "Error";
      break;
    };
    Clay_String text = {
        .length = (int32_t)strlen(label),
        .chars = label,
    };
    CLAY_TEXT(text, text_defaults());
  }
}

void create_ui(app_data_t *data, bool *exit_requested) {

  Clay_Sizing layout_expand = {
      .width = CLAY_SIZING_GROW(0),
      .height = CLAY_SIZING_GROW(0),
  };
  CLAY(CLAY_ID("OuterContainer"),
       {.layout =
            {
                .sizing = layout_expand,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        .cornerRadius = CLAY_CORNER_RADIUS(12),
        .backgroundColor = COLOR_BASE,
        .border = {.color = COLOR_SURFACE_1, .width = {1, 1, 1, 1}}}) {
    title_bar(exit_requested);
    CLAY_AUTO_ID({
        .layout = {.sizing = layout_expand,
                   .layoutDirection = CLAY_LEFT_TO_RIGHT},
    }) {
      side_bar();
      main_content(data);
    }
  }
}
