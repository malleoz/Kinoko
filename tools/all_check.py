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


dirs = [
    "../../AllSyncedGhosts/lc",
    "../../AllSyncedGhosts/mmm-ng", "../../AllSyncedGhosts/mmm-tas",
    "../../AllSyncedGhosts/mg-glitch", "../../AllSyncedGhosts/mg-ng", "../../AllSyncedGhosts/mg-nu",
    "../../AllSyncedGhosts/tf-glitch", "../../AllSyncedGhosts/tf-ng", "../../AllSyncedGhosts/tf-tas",

    "../../AllSyncedGhosts/mc-glitch", "../../AllSyncedGhosts/mc-ng", "../../AllSyncedGhosts/mc-tas",
    "../../AllSyncedGhosts/cm-glitch", "../../AllSyncedGhosts/cm-ng", "../../AllSyncedGhosts/cm-nu", "../../AllSyncedGhosts/cm-tas",
    "../../AllSyncedGhosts/dks", "../../AllSyncedGhosts/dks-tas",
    "../../AllSyncedGhosts/wgm-glitch", "../../AllSyncedGhosts/wgm-ng", "../../AllSyncedGhosts/wgm-tas",

    "../../AllSyncedGhosts/dc", "../../AllSyncedGhosts/dc-tas",
    "../../AllSyncedGhosts/kc-glitch", "../../AllSyncedGhosts/kc-ng", "../../AllSyncedGhosts/kc-tas",
    "../../AllSyncedGhosts/mt-glitch", "../../AllSyncedGhosts/mt-ng",
    "../../AllSyncedGhosts/gv-glitch", "../../AllSyncedGhosts/gv-ng", "../../AllSyncedGhosts/gv-nosc", "../../AllSyncedGhosts/gv-tas",

    "../../AllSyncedGhosts/ddr", "../../AllSyncedGhosts/ddr-tas",
    "../../AllSyncedGhosts/mh", "../../AllSyncedGhosts/mh-tas",
    "../../AllSyncedGhosts/bc-glitch", "../../AllSyncedGhosts/bc-ng", "../../AllSyncedGhosts/bc-tas",
    "../../AllSyncedGhosts/rr-glitch", "../../AllSyncedGhosts/rr-ng", "../../AllSyncedGhosts/rr-tas",
    
    "../../AllSyncedGhosts/rpb-glitch", "../../AllSyncedGhosts/rpb-ng", "../../AllSyncedGhosts/rpb-tas",
    "../../AllSyncedGhosts/ryf", "../../AllSyncedGhosts/ryf-tas",
    "../../AllSyncedGhosts/rgv2-ng", "../../AllSyncedGhosts/rgv2-glitch", 
    "../../AllSyncedGhosts/rmr",
    
    "../../AllSyncedGhosts/rsl-glitch", "../../AllSyncedGhosts/rsl-ng", "../../AllSyncedGhosts/rsl-tas",
    "../../AllSyncedGhosts/rsgb-glitch", "../../AllSyncedGhosts/rsgb-ng", "../../AllSyncedGhosts/rsgb-tas",
    "../../AllSyncedGhosts/rds", # TODO: DOUBLE CHECK ALL DESYNCING GHOSTS
    "../../AllSyncedGhosts/rws-glitch", "../../AllSyncedGhosts/rws-ng",

    "../../AllSyncedGhosts/rdh-glitch", "../../AllSyncedGhosts/rdh-ng", "../../AllSyncedGhosts/rdh-nu", "../../AllSyncedGhosts/rdh-tas",
    "../../AllSyncedGhosts/rbc3", "../../AllSyncedGhosts/rbc3-nosc", "../../AllSyncedGhosts/rbc3-tas",









    
    "../../AllSyncedGhosts/rdkjp-glitch", "../../AllSyncedGhosts/rdkjp-ng", "../../AllSyncedGhosts/rdkjp-nosc", "../../AllSyncedGhosts/rdkjp-tas",
    "../../AllSyncedGhosts/rmc-glitch", "../../AllSyncedGhosts/rmc-ng", "../../AllSyncedGhosts/rmc-tas",
    
    
    "../../AllSyncedGhosts/rmc3",
    "../../AllSyncedGhosts/rpg-glitch", "../../AllSyncedGhosts/rpg-ng", "../../AllSyncedGhosts/rpg-tas",
    "../../AllSyncedGhosts/rdkm-glitch", "../../AllSyncedGhosts/rdkm-ng", "../../AllSyncedGhosts/rdkm-tas",
    "../../AllSyncedGhosts/rbc-glitch", "../../AllSyncedGhosts/rbc-ng", "../../AllSyncedGhosts/rbc-nosc", "../../AllSyncedGhosts/rbc-tas",
    
    ]


def getFileCount(dir):
    count = len(os.listdir(dir))

    print(f"{dir}: {count}")

    return count


COUNT = 0
FILES_TESTED = 0
FILES_SYNCED = 0


def doDir(dir):
    directory = dir
    files_desynced = []
    dirFilesTested = 0
    dirFilesSynced = 0

    print(f"Validating ghosts in {dir}")

    for file in rkg_files(directory):
        filepath = os.path.join(directory, file)

        # Run Kinoko. subprocess.run shell behavior varies between Windows and Linux
        args = [
            "kinoko_setting",
            "replay",
            "-ghost",
            f"../{filepath}",
            # NOTE: Because Kinoko only accepts relative paths right now, you must copy the ghost folder to the root Kinoko folder.
        ]
        result = subprocess.run(
            args, cwd='out', shell=True, capture_output=True, text=True)

        global FILES_TESTED
        global FILES_SYNCED
        FILES_TESTED += 1
        dirFilesTested += 1

        if (result.returncode != 0):
            files_desynced.append(file)
            print(
                f"DESYNC: {file} [{len(files_desynced)} / {FILES_TESTED} ({len(files_desynced) / FILES_TESTED * 100:.2f}%)]")
        else:
            FILES_SYNCED += 1
            dirFilesSynced += 1

        if (FILES_TESTED % 100 == 0):
            print(
                f"PROGRESS: [{FILES_TESTED} / {COUNT} ({FILES_TESTED / COUNT * 100:.2f}%)]")

    with open('./ctgp_results.txt', 'w+') as f:
        f.writelines(file + '\n' for file in files_desynced)

    print(
        f"DIRECTORY RESULT - {dir} - {dirFilesSynced} / {dirFilesTested} synced!")


def main():
    # Let's first calculate total ghost count out of all dirs we will visit
    global COUNT
    for dir in dirs:
        COUNT += getFileCount(dir)

    print(f"Total ghosts that will be run: {COUNT}")

    for dir in dirs:
        doDir(dir)

    global FILES_SYNCED
    global FILES_TESTED
    print(f"RESULTS: [{FILES_SYNCED} / {FILES_TESTED}]")


if __name__ == '__main__':
    main()
