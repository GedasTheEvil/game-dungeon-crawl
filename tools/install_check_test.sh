#!/bin/sh
# Tests Install-check (plan install-helper-script) with a stub `ldd` on PATH: missing, nothing missing, non-apt distro.
cd "$(dirname "$0")/.." || exit 1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/rel/bin" "$tmp/stub"
cp Install-check "$tmp/rel/"
: > "$tmp/rel/bin/game"
printf 'ID=linuxmint\nID_LIKE="ubuntu debian"\n' > "$tmp/mint"
printf 'ID=fedora\n' > "$tmp/fedora"
fails=0
check() { # name, expected exit, pattern
	out=$(cd "$tmp/rel" && IC_OS_RELEASE=$4 PATH="$tmp/stub:$PATH" ./Install-check 2>&1)
	rc=$?
	if [ "$rc" != "$2" ] || ! printf '%s' "$out" | grep -q -- "$3"; then
		echo "install_check_test: FAIL $1 (exit $rc): $out"
		fails=$((fails + 1))
	fi
}
stub() { printf '#!/bin/sh\ncat <<X\n%s\nX\n' "$1" > "$tmp/stub/ldd"; chmod +x "$tmp/stub/ldd"; }

stub '	libc.so.6 => /lib/libc.so.6 (0x1)'
check "nothing missing" 0 "All good, run ./Play" "$tmp/mint"
stub '	libSDL3.so.0 => not found
	libGL.so.1 => not found
	libweird.so.1 => not found'
check "missing, mint" 1 "sudo apt install libgl1 libsdl3-0" "$tmp/mint"
stub '	libfreetype.so.6 => not found'
check "freetype" 1 "sudo apt install libfreetype6" "$tmp/mint"
stub '	libSDL3.so.0 => not found
	libGL.so.1 => not found
	libweird.so.1 => not found'
check "unknown soname" 1 "No package known for: libweird.so.1" "$tmp/mint"
check "non-apt distro" 1 "Unsupported system.*libSDL3.so.0" "$tmp/fedora"
if [ "$fails" != 0 ]; then
	exit 1
fi
echo "install_check_test: ok"
