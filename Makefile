# Toolchains
CC_LINUX   := gcc
CC_WIN     := x86_64-w64-mingw32-gcc
AR_WIN     := x86_64-w64-mingw32-ar

# Sources (assuming you moved them to src/)
SRC        := src/main.c src/win32_drag.c src/app.c src/fonts.c src/ui_components.c src/icons.c

# Unified Include Flag (works for both Linux and Windows!)
COMMON_INC := -I. -I./include -I./src

# Linux flags
LINUX_LIB  := -L./lib/linux
LINUX_LIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Windows flags
WIN_LIB    := -L./lib/win
WIN_LIBS   := -lraylib -lopengl32 -lgdi32 -lwinmm

# Raylib source path
RAYLIB_SRC := $(HOME)/raylib/src

.PHONY: all linux windows clean rebuild-raylib-win

all: linux windows

linux: app

windows: app.exe

app: $(SRC)
	$(CC_LINUX) $(SRC) -o $@ $(COMMON_INC) $(LINUX_LIB) $(LINUX_LIBS)

app.exe: $(SRC) lib/win/libraylib.a
	$(CC_WIN) $(SRC) -o $@ $(COMMON_INC) $(WIN_LIB) $(WIN_LIBS)

# Automatically place the compiled Windows library into lib/win/
lib/win/libraylib.a:
	mkdir -p lib/win
	$(MAKE) -C $(RAYLIB_SRC) PLATFORM=PLATFORM_DESKTOP CC=$(CC_WIN) AR=$(AR_WIN)
	mv $(RAYLIB_SRC)/libraylib.a $@

clean:
	rm -f app app.exe

rebuild-raylib-win:
	$(MAKE) -C $(RAYLIB_SRC) clean
	rm -f lib/win/libraylib.a
	$(MAKE) lib/win/libraylib.a
