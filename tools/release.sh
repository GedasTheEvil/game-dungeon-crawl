#!/bin/bash
# The release zip (plan release-zip): the portable game build, media, content, Play and an empty saves/ in
# release-<version>.zip in the repo root (version: .version, pride versioning PROUD.DEFAULT.SHAME, see
# docs/development.md). Usage: tools/release.sh [--allow-dirty]
set -eu
cd "$(dirname "$0")/.."

allow_dirty=0
for arg in "$@"; do
	case "$arg" in
		--allow-dirty) allow_dirty=1 ;;
		*) echo "usage: tools/release.sh [--allow-dirty]" >&2; exit 2 ;;
	esac
done

version=$(cat .version)
if ! [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
	echo "release: .version holds '$version', not PROUD.DEFAULT.SHAME" >&2
	exit 1
fi
if [ -n "$(git status --porcelain)" ]; then
	if [ "$allow_dirty" = 0 ]; then
		echo "release: dirty tree; commit first or use --allow-dirty" >&2
		exit 1
	fi
	version="$version-dirty"
fi

if [ -e release ]; then
	echo "release: release/ exists (left over from a failed run?); remove it by hand" >&2
	exit 1
fi

# Portable build (-march=x86-64-v2) in build/release: the dev build stays untouched.
make RELEASE=1 build/release/bin/game

zip_path="$PWD/release-$version.zip"
rm -f "$zip_path"
mkdir release
trap 'rm -rf release' EXIT

mkdir release/bin
cp build/release/bin/game release/bin/game
cp Play release/Play
cp .version release/.version
for dir in media content; do
	cp -r "$dir" release/
done
find release -name .directory -delete
mkdir release/saves

(cd release && zip -qr "$zip_path" .)
unzip -l "$zip_path" | grep -q ' saves/$' || (cd release && zip -q "$zip_path" saves/)
unzip -l "$zip_path" | grep -q ' Play$' || { echo "release: Play missing in zip" >&2; exit 1; }

echo "$zip_path ($(du -h "$zip_path" | cut -f1))"
