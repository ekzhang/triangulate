EMCC = '/Users/ezhang/emsdk-portable/emscripten/1.37.28/emcc' # Path to emcc

triangulate.js: triangulate.cpp
	$(EMCC) triangulate.cpp -o triangulate.js \
		-O2 -std=c++14 \
		-s WASM=1 \
		-s EXPORTED_FUNCTIONS="['_triangulate']" \
		-s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'

all: triangulate.js

clean:
	rm -f triangulate.js triangulate.wasm

run: all
	http-server .
