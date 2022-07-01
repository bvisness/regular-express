#!/usr/bin/env python3

import glob
import os
import re
import subprocess
import random
import shutil
import string
import sys

RELEASE = len(sys.argv) > 1 and sys.argv[1] == 'release'

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

[os.remove(f) for f in glob.iglob('build/dist/*', recursive=True)]
for ext in ['*.o', '*.wasm', '*.wat']:
    [os.remove(f) for f in glob.iglob('build/**/' + ext, recursive=True)]

os.makedirs('build', exist_ok=True)
os.chdir('build')

flags = [
    '--target=wasm32',
    '-c', '-O2', '-flto',
    '-nostdlib',
    '-I', '../src/fakestdlib',
    '-Wall', '-Wno-builtin-requires-header',
]

print('Compiling .wat files:')
for watfile in glob.glob('../src/**/*.wat', recursive=True):
    print('- ' + watfile)
    subprocess.run(['wat2wasm', watfile])

print('Compiling .c files:')
ofiles = []
for cfile in glob.glob('../src/**/*.c', recursive=True):
    safename = re.sub(r'[^a-zA-Z0-9]', '_', cfile[0:-2])
    ofile = safename + '.o'
    print('- ' + cfile)
    subprocess.run([clang] + flags + ['-o', ofile, cfile])
    ofiles.append(ofile)

print('Linking...')
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
if RELEASE:
    print('Optimizing WASM...')
    subprocess.run([
        'wasm-opt', 'regex.wasm',
        '-o', 'regex.wasm',
        '-O2', # general perf optimizations
        '--memory-packing', # remove unnecessary and extremely large .bss segment
        '--zero-filled-memory',
    ])

#
# Output the dist folder for upload
#

print('Building dist folder...')
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

# Produce a WAT version of the code for inspection.
if not RELEASE:
    os.chdir('build')
    print('Producing .wat files...')
    subprocess.run(['wasm2wat', 'regex.wasm', '-o', 'regex.wat'])
    os.chdir('..')

print('Done!')
