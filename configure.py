#!/usr/bin/env python3

from glob import glob
import io
import os
import sys
from tools.generate_tests import generate_tests
from vendor.ninja_syntax import Writer

generate_tests()

out_buf = io.StringIO()
n = Writer(out_buf)

file_extension = ''
if sys.platform.startswith('win32'):
    file_extension = '.exe'

n.variable('ninja_required_version', '1.3')
n.newline()

n.variable('builddir', 'build')
n.variable('outdir', 'out')
n.newline()

n.variable('compiler', 'g++')
n.newline()

common_ccflags = [
    '-DREVOLUTION',
    '-fcheck-new',
    '-ffp-contract=off',
    '-fno-asynchronous-unwind-tables',
    '-fno-exceptions',
    '-fno-rtti',
    '-fshort-wchar',
    '-fstack-protector-strong',
    '-isystem', '.',
    '-isystem', 'include',
    '-isystem', 'source',
    '-isystem', 'vendor',
    '-isystem', 'build',
    '-std=c++23',
    '-Wall',
    '-Wdouble-promotion',
    '-Werror',
    '-Wextra',
    '-Wno-delete-non-virtual-dtor',
    '-Wno-packed-bitfield-compat',
    '-Wsuggest-override',
    '-Wreorder',
]

target_cflags = [
    '-O3',
]

debug_cflags = [
    '-DBUILD_DEBUG',
    '-O0',
    '-ggdb',
]

# Release optimizations with gprof instrumentation, for finding CPU bottlenecks
profile_cflags = [
    *target_cflags,
    '-g',
    '-pg',
    '-fno-omit-frame-pointer',
]

common_ldflags = []

profile_ldflags = [
    '-pg',
]

# gprof can't map samples back to functions when MinGW's ASLR relocates the image
if sys.platform.startswith('win32'):
    profile_ldflags += [
        '-Wl,--disable-dynamicbase',
        '-Wl,--disable-high-entropy-va',
    ]

n.rule(
    'cc',
    command='$compiler -MD -MT $out -MF $out.d $ccflags -c $in -o $out',
    depfile='$out.d',
    deps='gcc',
    description='CC $out',
)
n.newline()

n.rule(
    'ld',
    command='$compiler $ldflags $in -o $out',
    description='LD $out',
)

code_in_files = [file for file in glob('**/*.cc', recursive=True)]

target_code_out_files = []
debug_code_out_files = []
profile_code_out_files = []

for in_file in code_in_files:
    _, ext = os.path.splitext(in_file)

    target_out_file = os.path.join('$builddir', in_file + '.o')
    target_code_out_files.append(target_out_file)

    debug_out_file = os.path.join('$builddir', in_file + 'D.o')
    debug_code_out_files.append(debug_out_file)

    profile_out_file = os.path.join('$builddir', in_file + 'P.o')
    profile_code_out_files.append(profile_out_file)

    n.build(
        target_out_file,
        ext[1:],
        in_file,
        variables={
            'ccflags': ' '.join([*common_ccflags, *target_cflags])
        }
    )
    n.newline()

    n.build(
        debug_out_file,
        ext[1:],
        in_file,
        variables={
            'ccflags': ' '.join([*common_ccflags, *debug_cflags])
        }
    )
    n.newline()

    n.build(
        profile_out_file,
        ext[1:],
        in_file,
        variables={
            'ccflags': ' '.join([*common_ccflags, *profile_cflags])
        }
    )
    n.newline()


n.build(
    os.path.join('$outdir', f'kinoko{file_extension}'),
    'ld',
    target_code_out_files,
    variables={
        'ldflags': ' '.join([
            *common_ldflags,
        ])
    },
)

n.build(
    os.path.join('$outdir', f'kinokoD{file_extension}'),
    'ld',
    debug_code_out_files,
    variables={
        'ldflags': ' '.join([
            *common_ldflags,
        ])
    },
)

profile_exe = os.path.join('$outdir', f'kinokoP{file_extension}')
n.build(
    profile_exe,
    'ld',
    profile_code_out_files,
    variables={
        'ldflags': ' '.join([
            *common_ldflags,
            *profile_ldflags,
        ])
    },
)
n.newline()

# The profiling build is opt-in via `ninja profile` so regular builds stay fast
n.build('profile', 'phony', profile_exe)
n.default([
    os.path.join('$outdir', f'kinoko{file_extension}'),
    os.path.join('$outdir', f'kinokoD{file_extension}'),
])
n.newline()

n.variable('configure', 'configure.py')
n.newline()

n.rule(
    'configure',
    command=f'{sys.executable} $configure',
    generator=True,
)
n.build(
    'build.ninja',
    'configure',
    implicit=[
        '$configure',
        os.path.join('vendor', 'ninja_syntax.py'),
    ],
)

with open('build.ninja', 'w') as out_file:
    out_file.write(out_buf.getvalue())
n.close()
