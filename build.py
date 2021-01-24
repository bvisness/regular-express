#!/usr/bin/env python3

import glob
import os
import re
import subprocess

clang = 'clang'
wasmld = 'wasm-ld'

try:
    subprocess.run(['clang-10', '-v'], stderr=subprocess.DEVNULL)
    clang = 'clang-10'
except FileNotFoundError:
    pass

try:
    subprocess.run(['wasm-ld-10', '-v'], stdout=subprocess.DEVNULL)
    wasmld = 'wasm-ld-10'
except FileNotFoundError:
    pass

os.makedirs('build', exist_ok=True)
os.chdir('build')

for f in glob.glob('*'):
    os.remove(f)

flags = [
    '--target=wasm32',
    '-c', '-O2', '-flto',
    '-nostdlib',
    '-I', '../src/fakestdlib',
    '-Wall', '-Wno-builtin-requires-header',
];

for watfile in glob.glob('../src/**/*.wat', recursive=True):
    subprocess.run(['wat2wasm', watfile])

ofiles = []
for cfile in glob.glob('../src/**/*.c', recursive=True):
    safename = re.sub(r'[^a-zA-Z0-9]', '_', cfile[0:-2])
    ofile = safename + '.o'
    subprocess.run([clang] + flags + ['-o', ofile, cfile])
    ofiles.append(ofile)

page_bytes = 65536
num_pages = 256

subprocess.run([
    wasmld,
    '--no-entry',
    '--export-all', '--no-gc-sections',
    '--allow-undefined',
    '--import-memory',
    '--initial-memory={}'.format(page_bytes * num_pages),
    '--lto-O2',
    '-o', 'regex.wasm',
] + ofiles)

# subprocess.run(['wasm2wat', 'regex.wasm'], stdout=open('regex.wat', 'w'))
