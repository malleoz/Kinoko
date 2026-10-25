# Validate that the framecounts in STATUS.md are up-to-date. This is done by trying to run the entire test case and
# asserting that the sync framecount is the highest it currently can be. This helps us capture when changes focused
# on one test case actually benefit a different test case.

from generate_tests import generate_tests
import json
import os
import subprocess
import sys
import time
from typing import Dict, TypedDict


def rkg_files(dir: str):
    yield from os.listdir(dir)


dirs = [
    "E:\\AllSyncedGhosts\\01-lc", "E:\\AllSyncedGhosts\\01-lc-tas",
    "E:\\AllSyncedGhosts\\02-mmm-ng", "E:\\AllSyncedGhosts\\02-mmm-tas",
    "E:\\AllSyncedGhosts\\03-mg-glitch", "E:\\AllSyncedGhosts\\03-mg-ng", "E:\\AllSyncedGhosts\\03-mg-nu",
    "E:\\AllSyncedGhosts\\04-tf-glitch", "E:\\AllSyncedGhosts\\04-tf-ng", "E:\\AllSyncedGhosts\\04-tf-tas",

    "E:\\AllSyncedGhosts\\05-mc-glitch", "E:\\AllSyncedGhosts\\05-mc-ng", "E:\\AllSyncedGhosts\\05-mc-tas",
    "E:\\AllSyncedGhosts\\06-cm-glitch", "E:\\AllSyncedGhosts\\06-cm-ng", "E:\\AllSyncedGhosts\\06-cm-nu", "E:\\AllSyncedGhosts\\06-cm-tas",
    "E:\\AllSyncedGhosts\\07-dks", "E:\\AllSyncedGhosts\\07-dks-tas",
    "E:\\AllSyncedGhosts\\08-wgm-glitch", "E:\\AllSyncedGhosts\\08-wgm-ng", "E:\\AllSyncedGhosts\\08-wgm-tas",

    "E:\\AllSyncedGhosts\\09-dc", "E:\\AllSyncedGhosts\\09-dc-tas",
    "E:\\AllSyncedGhosts\\10-kc-glitch", "E:\\AllSyncedGhosts\\10-kc-ng", "E:\\AllSyncedGhosts\\10-kc-tas",
    "E:\\AllSyncedGhosts\\11-mt-glitch", "E:\\AllSyncedGhosts\\11-mt-ng", "E:\\AllSyncedGhosts\\11-mt-tas",
    "E:\\AllSyncedGhosts\\12-gv-glitch", "E:\\AllSyncedGhosts\\12-gv-ng", "E:\\AllSyncedGhosts\\12-gv-nosc", "E:\\AllSyncedGhosts\\12-gv-tas",

    "E:\\AllSyncedGhosts\\13-ddr", "E:\\AllSyncedGhosts\\13-ddr-tas",
    "E:\\AllSyncedGhosts\\14-mh", "E:\\AllSyncedGhosts\\14-mh-tas",
    "E:\\AllSyncedGhosts\\15-bc-glitch", "E:\\AllSyncedGhosts\\15-bc-ng", "E:\\AllSyncedGhosts\\15-bc-tas",
    "E:\\AllSyncedGhosts\\16-rr-glitch", "E:\\AllSyncedGhosts\\16-rr-ng", "E:\\AllSyncedGhosts\\16-rr-tas",

    "E:\\AllSyncedGhosts\\17-rpb-glitch", "E:\\AllSyncedGhosts\\17-rpb-ng", "E:\\AllSyncedGhosts\\17-rpb-tas",
    "E:\\AllSyncedGhosts\\18-ryf", "E:\\AllSyncedGhosts\\18-ryf-tas",
    "E:\\AllSyncedGhosts\\19-rgv2-ng", "E:\\AllSyncedGhosts\\19-rgv2-glitch", "E:\\AllSyncedGhosts\\19-rgv2-tas",
    "E:\\AllSyncedGhosts\\20-rmr", "E:\\AllSyncedGhosts\\20-rmr-tas",

    "E:\\AllSyncedGhosts\\21-rsl-glitch", "E:\\AllSyncedGhosts\\21-rsl-ng", "E:\\AllSyncedGhosts\\21-rsl-tas",
    "E:\\AllSyncedGhosts\\22-rsgb-glitch", "E:\\AllSyncedGhosts\\22-rsgb-ng", "E:\\AllSyncedGhosts\\22-rsgb-tas",
    "E:\\AllSyncedGhosts\\23-rds", "E:\\AllSyncedGhosts\\23-rds-tas",
    "E:\\AllSyncedGhosts\\24-rws-glitch", "E:\\AllSyncedGhosts\\24-rws-ng", "E:\\AllSyncedGhosts\\24-rws-tas",

    "E:\\AllSyncedGhosts\\25-rdh-glitch", "E:\\AllSyncedGhosts\\25-rdh-ng", "E:\\AllSyncedGhosts\\25-rdh-nu",
    "E:\\AllSyncedGhosts\\25-rdh-tas",
    "E:\\AllSyncedGhosts\\26-rbc3", "E:\\AllSyncedGhosts\\26-rbc3-nosc", "E:\\AllSyncedGhosts\\26-rbc3-tas",
    "E:\\AllSyncedGhosts\\27-rdkjp-glitch", "E:\\AllSyncedGhosts\\27-rdkjp-ng", "E:\\AllSyncedGhosts\\27-rdkjp-nosc", "E:\\AllSyncedGhosts\\27-rdkjp-tas",
    "E:\\AllSyncedGhosts\\28-rmc-glitch", "E:\\AllSyncedGhosts\\28-rmc-ng", "E:\\AllSyncedGhosts\\28-rmc-tas",

    "E:\\AllSyncedGhosts\\29-rmc3", "E:\\AllSyncedGhosts\\29-rmc3-tas",
    "E:\\AllSyncedGhosts\\30-rpg-glitch", "E:\\AllSyncedGhosts\\30-rpg-ng", "E:\\AllSyncedGhosts\\30-rpg-tas",
    "E:\\AllSyncedGhosts\\31-rdkm-glitch", "E:\\AllSyncedGhosts\\31-rdkm-ng", "E:\\AllSyncedGhosts\\31-rdkm-tas",
    "E:\\AllSyncedGhosts\\32-rbc-glitch", "E:\\AllSyncedGhosts\\32-rbc-ng", "E:\\AllSyncedGhosts\\32-rbc-nosc", "E:\\AllSyncedGhosts\\32-rbc-tas",

]


def getFileCount(dir):
    count = len(os.listdir(dir))

    print(f"{dir}: {count}")

    return count


COUNT = 0
FILES_TESTED = 0
FILES_SYNCED = 0


def main():
    # Let's first calculate total ghost count out of all dirs we will visit
    global COUNT
    for dir in dirs:
        COUNT += getFileCount(dir)

    print(f"Total ghosts that will be run: {COUNT}")

    # Run Kinoko. subprocess.run shell behavior varies between Windows and Linux
    args = [
        "out/kinoko.exe",
        "replay",
        "-p",
        "1000",
        "-ghost",
    ]

    args += [dir for dir in dirs]

    start_time = time.perf_counter()
    try:
        subprocess.run(args, cwd='out', text=True)
    finally:
        elapsed_time = time.perf_counter() - start_time
        print(f"Total Kinoko runtime: {elapsed_time:.2f} seconds")


if __name__ == '__main__':
    main()
