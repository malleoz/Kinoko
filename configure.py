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
    '-ffp-contract=off',
    '-fno-asynchronous-unwind-tables',
    '-fno-exceptions',
    '-fno-rtti',
    '-fshort-wchar',
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
]

target_cflags = [
    '-O3',
    '-ffat-lto-objects',
    '-flto',
    '-fno-math-errno',
    '-fno-trapping-math',
    '--param inline-unit-growth=1000',
    '-fcf-protection=none',
    '-fno-pie',
    '-fno-stack-clash-protection',
    '-fno-stack-protector',
    '-U_FORTIFY_SOURCE',
]

debug_cflags = [
    '-DBUILD_DEBUG',
    '-O0',
    '-ggdb',
    '-fcheck-new',
    '-fsignaling-nans',
    '-fstack-protector-strong',
]

# Release optimizations for CPUs supporting AVX2/FMA/BMI2 (x86-64-v3), tuned for Zen 4. Results
# remain accurate because -ffp-contract=off prevents fusing multiplies and adds into FMA
native_cflags = [
    *target_cflags,
    '-march=native',
    '-mtune=native',
]

# Profile-guided optimization is a two stage build. The instrumented build replays the sample
# ghosts to record branch and call frequencies, which then guide optimization of the final build
pgo_gen_cflags = [
    *native_cflags,
    '-fprofile-generate',
]

pgo_use_cflags = [
    *native_cflags,
    '-fprofile-use',
]

common_ldflags = []

target_ldflags = [
    '-flto',
    '-flto-partition=one',
    '-no-pie',
]

# LTO optimizes at link time, so the optimization flags must be passed to the linker as well
pgo_gen_ldflags = [
    *pgo_gen_cflags,
    *target_ldflags,
]

pgo_use_ldflags = [
    *pgo_use_cflags,
    *target_ldflags,
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

n.rule(
    'pgo_train',
    command=f'{sys.executable} $script $in $out $profile_dir',
    description='PGO train $in',
)
n.newline()

code_in_files = [file for file in glob('**/*.cc', recursive=True)]

target_code_out_files = []
debug_code_out_files = []
pgo_gen_code_out_files = []
pgo_code_out_files = []

pgo_profile_dir = os.path.join('$builddir', 'pgo')
pgo_stamp = os.path.join(pgo_profile_dir, 'profile.stamp')

for in_file in code_in_files:
    _, ext = os.path.splitext(in_file)

    target_out_file = os.path.join('$builddir', in_file + '.o')
    target_code_out_files.append(target_out_file)

    debug_out_file = os.path.join('$builddir', in_file + 'D.o')
    debug_code_out_files.append(debug_out_file)

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

    pgo_gen_out_file = os.path.join('$builddir', in_file + 'PGOGen.o')
    pgo_gen_code_out_files.append(pgo_gen_out_file)

    pgo_out_file = os.path.join('$builddir', in_file + 'PGO.o')
    pgo_code_out_files.append(pgo_out_file)

    # GCC derives the profile data path, and the profile IDs of functions with internal linkage,
    # from the object's auxiliary base name. Both stages must share it for their profiles to match.
    pgo_dumpbase = ['-dumpbase', os.path.join(pgo_profile_dir, in_file)]

    n.build(
        pgo_gen_out_file,
        ext[1:],
        in_file,
        variables={
            'ccflags': ' '.join([*common_ccflags, *pgo_gen_cflags, *pgo_dumpbase])
        }
    )
    n.newline()

    n.build(
        pgo_out_file,
        ext[1:],
        in_file,
        implicit=[pgo_stamp],
        variables={
            'ccflags': ' '.join([*common_ccflags, *pgo_use_cflags, *pgo_dumpbase])
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
            *target_ldflags,
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
n.newline()

pgo_gen_exe = os.path.join('$outdir', f'kinokoPGOGen{file_extension}')
n.build(
    pgo_gen_exe,
    'ld',
    pgo_gen_code_out_files,
    variables={
        'ldflags': ' '.join([
            *common_ldflags,
            *pgo_gen_ldflags,
        ])
    },
)
n.newline()

pgo_train_script = os.path.join('tools', 'pgo_train.py')
n.build(
    pgo_stamp,
    'pgo_train',
    pgo_gen_exe,
    implicit=[
        pgo_train_script,
        *sorted(glob(os.path.join('samples', '*.rkg'))),
    ],
    variables={
        'script': pgo_train_script,
        'profile_dir': pgo_profile_dir,
    },
)
n.newline()

pgo_exe = os.path.join('$outdir', f'kinokoPGO{file_extension}')
n.build(
    pgo_exe,
    'ld',
    pgo_code_out_files,
    variables={
        'ldflags': ' '.join([
            *common_ldflags,
            *pgo_use_ldflags,
        ])
    },
)
n.newline()

# The PGO build is opt-in via `ninja pgo`, as training requires the extracted Race folder in out/
n.build('pgo', 'phony', pgo_exe)
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
