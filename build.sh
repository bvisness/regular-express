set -e

clang="clang"
wasmld="wasm-ld"

which clang-10 && clang="clang-10"
which wasm-ld-10 && wasmld="wasm-ld-10"

wat2wasm sys.wat

flags="--target=wasm32 -c -O2 -flto -nostdlib -I fakestdlib -Wno-builtin-requires-header -Wno-incompatible-pointer-types-discards-qualifiers -Wno-int-conversion"

$clang $flags -o stb_sprintf.o fakestdlib/stb_sprintf.c
$clang $flags -o stdio.o fakestdlib/stdio.c
$clang $flags -o stdlib.o fakestdlib/stdlib.c
$clang $flags -o string.o fakestdlib/string.c
$clang $flags -o test.o test.c
$clang $flags -o microui.o microui.c

$wasmld \
	--no-entry \
	--export-all --no-gc-sections \
	--allow-undefined \
	--import-memory \
	--lto-O2 \
	-o test.wasm \
	stb_sprintf.o stdio.o stdlib.o string.o test.o microui.o
