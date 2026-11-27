#!/bin/bash
# The release zip (plan release-zip): the portable game build, media, content, Play, the player manual (manual/)
# and an empty saves/ in release-<version>.zip in the repo root (version: .version, pride versioning PROUD.DEFAULT.SHAME, see
# docs/development.md); Install-check (plan install-helper-script) checks the target PC.
# Portable (plan portable-linux-release): bin/game is built in the Steam Runtime sniper container
# (tools/release_build.sh), lib/ holds the SDL3 it was built with; both need only glibc >= GLIBC_FLOOR, checked here.
# --host builds on this PC instead (build/release-host, no lib/: the target needs this PC's glibc and libsdl3-0).
# --smoke runs the zip on other distros afterwards (tools/release_smoke.sh, pulls their images).
# Usage: tools/release.sh [--allow-dirty] [--host] [--smoke]
set -eu
cd "$(dirname "$0")/.."

GLIBC_FLOOR=2.31
# What bin/game and lib/ may link: glibc, and the bundled SDL3.
NEEDED_ALLOWED='^(libc|libm|libmvec|libdl|libpthread|librt|ld-linux-x86-64)\.so\.[0-9]+$|^libSDL3\.so\.0$'

allow_dirty=0
host=0
smoke=0
for arg in "$@"; do
	case "$arg" in
		--allow-dirty) allow_dirty=1 ;;
		--host) host=1 ;;
		--smoke) smoke=1 ;;
		*) echo "usage: tools/release.sh [--allow-dirty] [--host] [--smoke]" >&2; exit 2 ;;
	esac
done

version=$(cat .version)
if ! [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-dev)?$ ]]; then
	echo "release: .version holds '$version', not PROUD.DEFAULT.SHAME[-dev]" >&2
	exit 1
fi
if [ -n "$(git status --porcelain)" ]; then
	if [ "$allow_dirty" = 0 ]; then
		echo "release: dirty tree; commit first or use --allow-dirty" >&2
		exit 1
	fi
	version="$version-dirty"
fi
[ "$host" = 1 ] && version="$version-host"

if [ -e release ]; then
	echo "release: release/ exists (left over from a failed run?); remove it by hand" >&2
	exit 1
fi

# -march=x86-64-v2 either way: the dev build stays untouched.
if [ "$host" = 1 ]; then
	make RELEASE=1 RELEASE_HOST=1 build/release-host/bin/game
	game=build/release-host/bin/game
else
	tools/release_build.sh
	game=build/release/bin/game
fi

zip_path="$PWD/release-$version.zip"
rm -f "$zip_path"
mkdir release
trap 'rm -rf release' EXIT

mkdir release/bin
cp "$game" release/bin/game
if [ "$host" = 0 ]; then
	mkdir release/lib
	cp -L build/release-deps/prefix/lib/libSDL3.so.0 release/lib/
fi

# The newest glibc symbol version and the libraries each file needs: the floor holds, nothing outside the allow list.
glibc_needed=$GLIBC_FLOOR
for file in release/bin/game release/lib/*; do
	[ -e "$file" ] || continue
	newest=$(objdump -T "$file" | grep -oE 'GLIBC_[0-9]+\.[0-9]+(\.[0-9]+)?' | sed 's/GLIBC_//' | sort -uV | tail -1)
	[ -n "$newest" ] && glibc_needed=$(printf '%s\n%s\n' "$glibc_needed" "$newest" | sort -V | tail -1)
	[ "$host" = 1 ] && continue
	if [ "$glibc_needed" != "$GLIBC_FLOOR" ]; then
		echo "release: $file needs GLIBC_$newest, above the floor $GLIBC_FLOOR" >&2
		exit 1
	fi
	if objdump -T "$file" | grep -qE 'GLIBCXX_|CXXABI_'; then
		echo "release: $file needs the system libstdc++ (it should be linked in)" >&2
		exit 1
	fi
	bad=$(objdump -p "$file" | awk '$1 == "NEEDED" {print $2}' | grep -vE "$NEEDED_ALLOWED" || true)
	if [ -n "$bad" ]; then
		echo "release: $file needs libraries outside the zip and glibc:" $bad >&2
		exit 1
	fi
done

cp Play release/Play
# Install-check compares the target's glibc with this release's floor.
sed "s/^glibc_floor=.*/glibc_floor=$glibc_needed/" Install-check >release/Install-check
chmod +x release/Install-check
cp .version release/.version
for dir in media content; do
	cp -r "$dir" release/
done
cp -r docs/manual release/manual
find release -name .directory -delete
mkdir release/saves

(cd release && zip -qr "$zip_path" .)
unzip -l "$zip_path" | grep -q ' saves/$' || (cd release && zip -q "$zip_path" saves/)
unzip -l "$zip_path" | grep -q ' Play$' || { echo "release: Play missing in zip" >&2; exit 1; }
unzip -l "$zip_path" | grep -q ' manual/index.html$' || { echo "release: manual missing in zip" >&2; exit 1; }
if [ "$host" = 0 ]; then
	unzip -l "$zip_path" | grep -q ' lib/libSDL3.so.0$' || { echo "release: lib/libSDL3.so.0 missing in zip" >&2; exit 1; }
fi

echo "$zip_path ($(du -h "$zip_path" | cut -f1), glibc $glibc_needed+)"
if [ "$smoke" = 1 ]; then
	tools/release_smoke.sh "$zip_path"
fi
