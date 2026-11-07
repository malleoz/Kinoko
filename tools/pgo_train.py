#!/usr/bin/env python3

"""Trains the PGO build by replaying every sample ghost with the instrumented executable.

Usage: pgo_train.py <instrumented exe> <stamp> <profile dir>

The instrumented executable writes its profile data (.gcda) into the profile directory, which is
where the optimized build reads it back from when compiling with -fprofile-use.
"""

from glob import glob
import os
import subprocess
import sys


def find_profiles(directory):
    return glob(os.path.join(directory, '**', '*.gcda'), recursive=True)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)

    exe, stamp, profile_dir = sys.argv[1:]
    exe = os.path.abspath(exe)

    # Profile data accumulates across runs, so data from any previous training must be discarded
    for profile in find_profiles(profile_dir):
        os.remove(profile)

    ghosts = sorted(os.path.abspath(ghost) for ghost in glob(os.path.join('samples', '*.rkg')))
    if not ghosts:
        sys.exit('No sample ghosts found in samples/')

    # Kinoko loads game files relative to the working directory, which is expected to be the
    # output directory containing the extracted Race folder
    for ghost in ghosts:
        result = subprocess.run([exe, 'replay', '-g', ghost], cwd=os.path.dirname(exe),
                capture_output=True, text=True)
        if result.returncode != 0:
            print(f'warning: replay of {os.path.basename(ghost)} exited with {result.returncode}')
            print(result.stdout + result.stderr)

    profiles = find_profiles(profile_dir)
    if not profiles:
        sys.exit(f'No profile data was generated in {profile_dir}')

    print(f'Trained on {len(ghosts)} ghosts, generated {len(profiles)} profiles')

    with open(stamp, 'w') as stamp_file:
        stamp_file.write('\n'.join(ghosts) + '\n')


if __name__ == '__main__':
    main()
