#!/bin/bash
# The portable release build (plan portable-linux-release), run by tools/release.sh. On the host it fetches the pinned
# SDL3 and FreeType sources into build/release-deps/ (sha256 checked) and runs itself in the Steam Runtime 3 "sniper"
# SDK container (glibc 2.31, g++-14); there it builds SDL3 (shared, its X11 / Wayland / audio backends loaded at run
# time) and FreeType (static, plain TrueType: no zlib, png, bzip2, brotli, harfbuzz) into build/release-deps/prefix,
# then the game in build/release. The zip ships prefix/lib/libSDL3.so.0 as lib/; bin/game finds it by its RUNPATH.
# Usage: tools/release_build.sh (host), tools/release_build.sh --in-container (inside, by the host run).
set -eu
cd "$(dirname "$0")/.."

# Pinned: a new version needs its sha256 (the signatures were checked with gpg when pinned, 2026-11-25).
SNIPER_IMAGE=registry.gitlab.steamos.cloud/steamrt/sniper/sdk@sha256:bb30053d3cd19d35c76ae69b2790dbaa1430448006f8240cb6d7e723f2d1940e
SDL_VERSION=3.4.20
SDL_SHA256=f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f
SDL_URL=https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL3-$SDL_VERSION.tar.gz
FT_VERSION=2.14.1
FT_SHA256=32427e8c471ac095853212a37aef816c60b42052d4d9e48230bab3bdf2936ccc
FT_URL=https://download.savannah.gnu.org/releases/freetype/freetype-$FT_VERSION.tar.xz

DEPS=build/release-deps
PREFIX=$DEPS/prefix
# What prefix/ was built from: another pin or compiler rebuilds it.
DEPS_STAMP="sdl $SDL_VERSION $SDL_SHA256, freetype $FT_VERSION $FT_SHA256, gcc-14 by CC"

fetch() { # url, sha256
	local file
	file=$DEPS/$(basename "$1")
	if [ ! -f "$file" ] || ! echo "$2  $file" | sha256sum -c --status; then
		curl -sSLf -o "$file.part" "$1"
		echo "$2  $file.part" | sha256sum -c --status || {
			echo "release_build: $file: sha256 mismatch" >&2
			rm -f "$file.part"
			exit 1
		}
		mv "$file.part" "$file"
	fi
}

build_deps() {
	[ "$(cat "$PREFIX/.stamp" 2>/dev/null)" = "$DEPS_STAMP" ] && return
	rm -rf "$PREFIX" "$DEPS/src"
	mkdir -p "$DEPS/src"
	tar -C "$DEPS/src" -xzf "$DEPS/SDL3-$SDL_VERSION.tar.gz"
	tar -C "$DEPS/src" -xJf "$DEPS/freetype-$FT_VERSION.tar.xz"
	# gcc-14 by CC / CXX, not by the image's toolchain file (/usr/share/steamrt/cmake/gcc-14.cmake): with that, SDL's
	# Wayland and D-Bus checks find no pkg-config and drop both backends unseen.
	export CC=gcc-14 CXX=g++-14
	cmake -S "$DEPS/src/SDL3-$SDL_VERSION" -B "$DEPS/src/sdl-build" \
		-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/$PREFIX" -DCMAKE_INSTALL_LIBDIR=lib \
		-DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
		-DSDL_DEPS_SHARED=ON >"$DEPS/sdl-configure.log"
	local backend
	for backend in X11 WAYLAND WAYLAND_LIBDECOR KMSDRM DBUS PULSEAUDIO PIPEWIRE ALSA; do
		grep -qE "SDL_${backend} +\(Wanted: ON\): ON" "$DEPS/sdl-configure.log" || {
			echo "release_build: SDL3 configured without $backend ($DEPS/sdl-configure.log)" >&2
			exit 1
		}
	done
	cmake --build "$DEPS/src/sdl-build" -j "$(nproc)" >"$DEPS/sdl-build.log"
	cmake --install "$DEPS/src/sdl-build" >/dev/null
	cmake -S "$DEPS/src/freetype-$FT_VERSION" -B "$DEPS/src/ft-build" \
		-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/$PREFIX" -DCMAKE_INSTALL_LIBDIR=lib \
		-DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON \
		-DFT_DISABLE_PNG=ON -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON >"$DEPS/ft-configure.log"
	cmake --build "$DEPS/src/ft-build" -j "$(nproc)" >"$DEPS/ft-build.log"
	cmake --install "$DEPS/src/ft-build" >/dev/null
	rm -rf "$DEPS/src"
	echo "$DEPS_STAMP" >"$PREFIX/.stamp"
}

if [ "${1:-}" != --in-container ]; then
	mkdir -p "$DEPS"
	fetch "$SDL_URL" "$SDL_SHA256"
	fetch "$FT_URL" "$FT_SHA256"
	# HOME in the build tree: ccache and git need a writable one, the user has none in the image.
	exec docker run --rm -v "$PWD:/src" -w /src -u "$(id -u):$(id -g)" -e HOME=/src/$DEPS/home \
		-e CCACHE_DIR=/src/$DEPS/ccache "$SNIPER_IMAGE" tools/release_build.sh --in-container
fi

mkdir -p "$HOME"
git config --global --get safe.directory >/dev/null || git config --global safe.directory /src
build_deps
# Objects from another compiler (an older host build in build/release) are dropped.
compiler=$(g++-14 --version | head -1)
if [ "$(cat build/release/.compiler 2>/dev/null)" != "$compiler" ]; then
	rm -rf build/release
	mkdir -p build/release
	echo "$compiler" >build/release/.compiler
fi
# Only the prefix: the image's own FreeType and SDL are not used.
export PKG_CONFIG_LIBDIR=$PWD/$PREFIX/lib/pkgconfig
# SDL_LIBS by hand: its pkg-config file adds an RPATH to the build tree.
make RELEASE=1 CXX=g++-14 SDL_LIBS="-L$PWD/$PREFIX/lib -lSDL3" build/release/bin/game
