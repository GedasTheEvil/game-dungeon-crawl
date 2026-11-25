#!/bin/sh
# Tests Install-check (plans install-helper-script, portable-linux-release) with stub `ldd`, `getconf` and `ldconfig`
# on PATH: missing, nothing missing, non-apt distro, too old glibc, no GL driver, the bundled lib/ gone.
cd "$(dirname "$0")/.." || exit 1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/rel/bin" "$tmp/stub"
cp Install-check "$tmp/rel/"
: > "$tmp/rel/bin/game"
printf 'ID=linuxmint\nID_LIKE="ubuntu debian"\n' > "$tmp/mint"
printf 'ID=fedora\n' > "$tmp/fedora"
fails=0
check() { # name, expected exit, pattern, os-release
	out=$(cd "$tmp/rel" && IC_OS_RELEASE=$4 PATH="$tmp/stub:$PATH" ./Install-check 2>&1)
	rc=$?
	if [ "$rc" != "$2" ] || ! printf '%s' "$out" | grep -q -- "$3"; then
		echo "install_check_test: FAIL $1 (exit $rc): $out"
		fails=$((fails + 1))
	fi
}
stub() { # command, its output
	printf '#!/bin/sh\ncat <<X\n%s\nX\n' "$2" > "$tmp/stub/$1"
	chmod +x "$tmp/stub/$1"
}
stub getconf 'glibc 2.39'
stub ldconfig '	libGL.so.1 (libc6,x86-64) => /lib/x86_64-linux-gnu/libGL.so.1'

stub ldd '	libc.so.6 => /lib/libc.so.6 (0x1)'
check "nothing missing" 0 "All good, run ./Play" "$tmp/mint"
stub ldd '	libSDL3.so.0 => not found
	libGL.so.1 => not found
	libweird.so.1 => not found'
check "missing, mint" 1 "sudo apt install libgl1 libgl1-mesa-dri libsdl3-0" "$tmp/mint"
stub ldd '	libfreetype.so.6 => not found'
check "freetype" 1 "sudo apt install libfreetype6" "$tmp/mint"
stub ldd '	libSDL3.so.0 => not found
	libGL.so.1 => not found
	libweird.so.1 => not found'
check "unknown soname" 1 "No package known for: libweird.so.1" "$tmp/mint"
check "non-apt distro" 1 "Unsupported system.*libSDL3.so.0" "$tmp/fedora"

stub ldd '	libc.so.6 => /lib/libc.so.6 (0x1)'
stub getconf 'glibc 2.28'
check "old glibc" 1 "Too old system: glibc 2.28, the game needs 2.31" "$tmp/mint"
stub getconf 'glibc 2.31'
check "glibc at the floor" 0 "All good" "$tmp/mint"

stub ldconfig '	libfoo.so.1 (libc6,x86-64) => /lib/libfoo.so.1'
check "no GL driver" 1 "sudo apt install libgl1 libgl1-mesa-dri" "$tmp/mint"
stub ldconfig '	libEGL.so.1 (libc6,x86-64) => /lib/x86_64-linux-gnu/libEGL.so.1'
check "EGL only" 0 "All good" "$tmp/mint"

mkdir "$tmp/rel/lib"
check "bundled lib gone" 1 "lib/libSDL3.so.0 is missing" "$tmp/mint"
: > "$tmp/rel/lib/libSDL3.so.0"
check "bundled lib there" 0 "All good" "$tmp/mint"
if [ "$fails" != 0 ]; then
	exit 1
fi
echo "install_check_test: ok"
