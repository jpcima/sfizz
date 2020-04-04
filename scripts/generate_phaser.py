#!/usr/bin/env python

import os
import sys
import subprocess
import re

FAUST_ARGS = ['-inpl']
FAUST_CLASS = 'faustPhaserStage'
FAUST_IN = 'src/sfizz/effects/dsp/phaser_stage.dsp'
FAUST_OUT = 'src/sfizz/effects/gen/phaser_stage.cpp'

if not os.path.exists('src'):
    print('Please run this in the project root directory.')
    sys.exit(1)

code = subprocess.Popen(
    ['faust'] + FAUST_ARGS + ['-cn', FAUST_CLASS, FAUST_IN],
    stdout=subprocess.PIPE).communicate()[0].decode('utf-8')

code = re.sub(r'\bvoid\s+metadata\s*\([^}]+\}', '', code)
code = re.sub(r'\bvoid\s+buildUserInterface\s*\([^}]+\}', '', code)

code = re.sub(r':\s*public\s+dsp', '', code)
code = re.sub(r'\bvirtual\b', '', code)

open(FAUST_OUT, 'w').write(code)

subprocess.run(['clang-format', '-i', FAUST_OUT])
