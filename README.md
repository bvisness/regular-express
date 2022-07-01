# Regular Express

![An example regular expression](screenshot.png)

A tool that allows you to build regular expressions completely visually. Use the power of regular expressions without getting bogged down by the syntax!

View the [task tracker](https://www.notion.so/bvisness/3708fea1fb9d43f1b51b7512d685f963?v=c0b80b9cebc843f5b95dac1370bfa76b) to see what's in the works.


## Building

You can build with `python build.py`. Python 3 is required. It should be portable across platforms as long as you have everything installed.

You will need the following installed and on your path:

- `clang` (and `wasm-ld`, included)
    - On Windows, it seems the easiest way is to install Visual Studio with the "clang development tools" or whatever. It is an optional add-on to the "Desktop development with C++" preset. Clang will then be available inside the VS developer command prompt.
- `wat2wasm` (from [wabt](https://github.com/WebAssembly/wabt))
- `wasm-opt` (from [binaryen](https://github.com/webassembly/binaryen))
