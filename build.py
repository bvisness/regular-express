#!/usr/bin/env python3

import glob
import os
import re
import subprocess
import random
import shutil
import string

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

try:
    shutil.rmtree('build')
except:
    pass
os.makedirs('build', exist_ok=True)
os.chdir('build')

flags = [
    '--target=wasm32',
    '-c', '-O2', '-flto',
    '-nostdlib',
    '-I', '../src/fakestdlib',
    '-Wall', '-Wno-builtin-requires-header',
]

for watfile in glob.glob('../src/**/*.wat', recursive=True):
    subprocess.run(['wat2wasm', watfile])

ofiles = []
for cfile in glob.glob('../src/**/*.c', recursive=True):
    safename = re.sub(r'[^a-zA-Z0-9]', '_', cfile[0:-2])
    ofile = safename + '.o'
    subprocess.run([clang] + flags + ['-o', ofile, cfile])
    ofiles.append(ofile)

subprocess.run([
    wasmld,
    '--no-entry',
    '--export-all', '--no-gc-sections',
    '--allow-undefined',
    '--import-memory',
    '--lto-O2',
    '-o', 'regex.wasm',
] + ofiles)

# Optimize output WASM file
subprocess.run([
    'wasm-opt', 'regex.wasm',
    '-o', 'regex.wasm',
    '-O2', # general perf optimizations
    '--memory-packing', # remove unnecessary and extremely large .bss segment
    '--zero-filled-memory',
])

# Produce a WAT version of the code for inspection.
subprocess.run(['wasm2wat', 'regex.wasm', '-o', 'regex.wat'])

#
# Output the dist folder for upload
#

os.chdir('..')
os.makedirs('build/dist', exist_ok=True)

buildId = ''.join(random.choices(string.ascii_uppercase + string.digits, k=8)) # so beautiful. so pythonic.

root = 'src/index.html'
assets = [
    'src/normalize.css',
    'build/regex.wasm',
    'build/sys.wasm',
]

rootContents = open(root).read()

def addId(filename, id):
    parts = filename.split('.')
    parts.insert(-1, buildId)
    return '.'.join(parts)

for asset in assets:
    basename = os.path.basename(asset)
    newFilename = addId(basename, buildId)
    shutil.copy(asset, 'build/dist/{}'.format(newFilename))

    rootContents = rootContents.replace(basename, newFilename)

with open('build/dist/index.html', 'w') as f:
    f.write(rootContents)
