set -e

wat2wasm sys.wat

clang \
	--target=wasm32 \
	-c -O2 \
	-flto \
	-nostdlib \
	-I fakestdlib \
	-o stdlib.o \
	stdlib.c

clang \
	--target=wasm32 \
	-c -O2 \
	-flto \
	-nostdlib \
	-I fakestdlib \
	-o test.o \
	test.c

clang \
	--target=wasm32 \
	-c -O2 \
	-Wno-builtin-requires-header \
	-flto \
	-nostdlib \
	-I fakestdlib \
	-o microui.o \
	microui.c

wasm-ld \
	--no-entry \
	--export-all --no-gc-sections \
	--allow-undefined \
	--import-memory \
	--lto-O2 \
	-o test.wasm \
	stdlib.o test.o microui.o
