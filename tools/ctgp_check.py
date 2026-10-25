# Validate that the framecounts in STATUS.md are up-to-date. This is done by trying to run the entire test case and
# asserting that the sync framecount is the highest it currently can be. This helps us capture when changes focused
# on one test case actually benefit a different test case.

from generate_tests import generate_tests
import json
import os
import subprocess
import sys
from typing import Dict, TypedDict


def rkg_files(dir: str):
    yield from os.listdir(dir)


def main():
    directory = sys.argv[1]

    files_tested = 0
    files_synced = 0
    files_desynced = []
    file_count = len(os.listdir(directory))

    for file in rkg_files(directory):
        filepath = os.path.join(directory, file)

        # Run Kinoko. subprocess.run shell behavior varies between Windows and Linux
        exec = os.path.join('.', 'kinoko')
        args = [
            exec,
            "replay",
            "-ghost",
            f"../{filepath}",
            # NOTE: Because Kinoko only accepts relative paths right now, you must copy the ghost folder to the root Kinoko folder.
        ]
        result = subprocess.run(args, cwd='out', shell=True, capture_output=True, text=True)

        files_tested += 1

        if (result.returncode != 0):
            files_desynced.append(file)
            print(f"DESYNC: {file} [{len(files_desynced)} / {files_tested} ({len(files_desynced) / files_tested * 100:.2f}%)]")
        else:
            files_synced += 1

        if (files_tested % 100 == 0):
            print(f"PROGRESS: [{files_tested} / {file_count} ({files_tested / file_count * 100:.2f}%)]")

    print(f"RESULTS: [{files_synced} / {files_tested}]")

    with open('./ctgp_results.txt', 'w+') as f:
        f.writelines(file + '\n' for file in files_desynced)


if __name__ == '__main__':
    main()
