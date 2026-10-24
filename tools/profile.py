#!/usr/bin/env python3

"""Builds the gprof-instrumented Kinoko binary, runs it, and writes a CPU profile report.

Usage (from the repository root):
    python tools/profile.py                                  # profile the test suite
    python tools/profile.py --runs 20 -- replay -g ghost.rkg # profile a replay

The workload is repeated `--runs` times and the gprof samples are merged, since a single
pass over the test suite is too short to collect a meaningful number of samples.
"""

import glob
import os
import shutil
import subprocess
import sys
from argparse import ArgumentParser

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT_DIR, 'out')
EXE_NAME = 'kinokoP.exe' if sys.platform.startswith('win32') else 'kinokoP'


def main():
    parser = ArgumentParser(description='Profile Kinoko with gprof')
    parser.add_argument('--runs', type=int, default=10,
                        help='number of times to run the workload (default: 10)')
    parser.add_argument('--report', default=os.path.join(OUT_DIR, 'profile.txt'),
                        help='path of the report to write (default: out/profile.txt)')
    parser.add_argument('--top', type=int, default=25,
                        help='number of flat profile rows to print (default: 25)')
    parser.add_argument('--no-build', action='store_true',
                        help='skip running `ninja profile` first')
    parser.add_argument('kinoko_args', nargs='*', default=['test', '-s', 'testCases.bin'],
                        help='arguments passed to kinoko (default: test -s testCases.bin)')
    args = parser.parse_args()

    if not args.no_build:
        subprocess.run(['ninja', 'profile'], cwd=ROOT_DIR, check=True)

    exe = os.path.join(OUT_DIR, EXE_NAME)
    gmon = os.path.join(OUT_DIR, 'gmon.out')
    for stale in glob.glob(os.path.join(OUT_DIR, 'gmon*.out')):
        os.remove(stale)

    run_files = []
    for i in range(args.runs):
        print(f'Run {i + 1}/{args.runs}...', flush=True)
        result = subprocess.run([exe, *args.kinoko_args], cwd=OUT_DIR,
                                stdout=subprocess.DEVNULL)
        if result.returncode != 0:
            sys.exit(f'kinoko exited with code {result.returncode}')
        if not os.path.exists(gmon):
            sys.exit('gmon.out was not written; kinoko must exit normally to save profile data')

        run_file = os.path.join(OUT_DIR, f'gmon.{i}.out')
        shutil.move(gmon, run_file)
        run_files.append(run_file)

    # `gprof -s` merges all runs into gmon.sum
    subprocess.run(['gprof', '-s', exe, *run_files], cwd=OUT_DIR, check=True)
    for run_file in run_files:
        os.remove(run_file)

    report = subprocess.run(['gprof', '-b', '--demangle', exe, 'gmon.sum'], cwd=OUT_DIR,
                            check=True, capture_output=True, text=True).stdout
    with open(args.report, 'w') as f:
        f.write(report)

    # Print the header plus the hottest functions from the flat profile
    flat = report.split('\f')[0].splitlines()
    print('\n'.join(flat[:5 + args.top]))
    print(f'\nFull report (flat profile + call graph): {args.report}')
    print('Note: _mcount_private / __fentry__ / mcount entries are gprof instrumentation overhead.')


if __name__ == '__main__':
    main()
