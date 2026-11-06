# Toolchain
CC_WIN     := x86_64-w64-mingw32-gcc
AR_WIN     := x86_64-w64-mingw32-ar

# Sources and objects
SRC        := src/main.c src/win32_drag.c src/app.c src/fonts.c \
              src/ui_components.c src/icons.c src/api.c src/hashmap.c
OBJ        := $(SRC:src/%.c=build/%.o)
DEP        := $(OBJ:.o=.d)

# Include paths and defines
COMMON_INC := -I. -I./include -I./src -I./lib/win/include -DCURL_STATICLIB

# Library search path and libraries
WIN_LIB    := -L./lib/win/lib
WIN_LIBS   := -lraylib -lopengl32 -lgdi32 -lwinmm \
              -lcurl -lcrypt32 -lwldap32 -lws2_32 -lbcrypt

# Dependency source trees
RAYLIB_SRC := $(HOME)/raylib/src
CURL_SRC   := $(HOME)/curl

.PHONY: all clean distclean rebuild-raylib-win rebuild-curl-win

all: app.exe

app.exe: $(OBJ) lib/win/lib/libraylib.a lib/win/lib/libcurl.a
	$(CC_WIN) $(OBJ) -o $@ $(WIN_LIB) $(WIN_LIBS)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC_WIN) $(COMMON_INC) -MMD -MP -c $< -o $@

-include $(DEP)

# --- raylib ---
lib/win/lib/libraylib.a:
	mkdir -p lib/win/lib
	$(MAKE) -C $(RAYLIB_SRC) PLATFORM=PLATFORM_DESKTOP CC=$(CC_WIN) AR=$(AR_WIN)
	mv $(RAYLIB_SRC)/libraylib.a $@

rebuild-raylib-win:
	$(MAKE) -C $(RAYLIB_SRC) clean
	rm -f lib/win/lib/libraylib.a
	$(MAKE) lib/win/lib/libraylib.a

# --- libcurl ---
lib/win/lib/libcurl.a:
	mkdir -p lib/win/include lib/win/lib
	cd $(CURL_SRC) && $(MAKE) distclean 2>/dev/null || true
	cd $(CURL_SRC) && CC=$(CC_WIN) AR=$(AR_WIN) ./configure \
		--build=x86_64-pc-linux-gnu \
		--host=x86_64-w64-mingw32 \
		--prefix=$(CURDIR)/lib/win \
		--disable-shared \
		--enable-static \
		--with-schannel \
		--disable-ldap \
		--disable-ldaps \
		--disable-rtsp \
		--disable-dict \
		--disable-telnet \
		--disable-tftp \
		--disable-pop3 \
		--disable-imap \
		--disable-smb \
		--disable-smtp \
		--disable-gopher \
		--disable-mqtt \
		--disable-manual \
		--disable-threaded-resolver \
		--without-zlib \
		--without-brotli \
		--without-zstd \
		--without-libpsl \
		--without-libidn2 \
		--without-nghttp2 \
		--without-ngtcp2 \
		--without-nghttp3 \
		--without-quiche \
		--without-libssh2 \
		--without-librtmp
	cd $(CURL_SRC) && CC=$(CC_WIN) AR=$(AR_WIN) $(MAKE) LDFLAGS=-all-static
	cd $(CURL_SRC) && $(MAKE) install
	-rm -rf lib/win/bin lib/win/share lib/win/lib/*.la lib/win/lib/pkgconfig

rebuild-curl-win:
	cd $(CURL_SRC) && $(MAKE) distclean 2>/dev/null || true
	rm -f lib/win/lib/libcurl.a
	$(MAKE) lib/win/lib/libcurl.a

clean:
	rm -rf build
	rm -f app.exe

distclean: clean
	rm -f lib/win/lib/libraylib.a lib/win/lib/libcurl.a
