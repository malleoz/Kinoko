from dataclasses import dataclass
import glob
import os
import shutil
import sys
from typing import BinaryIO, Tuple

COURSE_NAMES = {
    0: "mc",
    1: "mmm",
    2: "mg",
    3: "gv",
    4: "tf",
    5: "cm",
    6: "dks",
    7: "wgm",
    8: "lc",
    9: "dc",
    10: "mh",
    11: "mt",
    12: "bc",
    13: "rr",
    14: "ddr",
    15: "kc",
    16: "rpb",
    17: "rmc",
    18: "rws",
    19: "rdkm",
    20: "ryf",
    21: "rdh",
    22: "rpg",
    23: "rds",
    24: "rmc3",
    25: "rgv2",
    26: "rmr",
    27: "rsl",
    28: "rbc",
    29: "rdkjp",
    30: "rbc3",
    31: "rsgb",
}


VEHICLE_NAMES = {
    0: "standard_kart_s",
    1: "standard_kart_m",
    2: "standard_kart_l",
    3: "baby_booster",
    4: "classic_dragster",
    5: "offroader",
    6: "mini_beast",
    7: "wild_wing",
    8: "flame_flyer",
    9: "cheep_charger",
    10: "super_blooper",
    11: "piranha_prowler",
    12: "tiny_titan",
    13: "daytripped",
    14: "jetsetter",
    15: "blue_falcon",
    16: "sprinter",
    17: "honeycoupe",
    18: "standard_bike_s",
    19: "standard_bike_m",
    20: "standard_bike_l",
    21: "bullet_bike",
    22: "mach_bike",
    23: "flame_runner",
    24: "bit_bike",
    25: "sugarscoot",
    26: "wario_bike",
    27: "quacker",
    28: "zip_zip",
    29: "shooting_star",
    30: "magikruiser",
    31: "sneakster",
    32: "spear",
    33: "jet_bubble",
    34: "dolphin_dasher",
    35: "phantom",
}


# See https://wiki.tockdom.com/wiki/List_of_Identifiers
CHARACTER_NAMES = {
    0: "mr",
    1: "bpc",
    2: "wl",
    3: "kp",
    4: "bds",
    5: "ka",
    6: "bmr",
    7: "lg",
    8: "ko",
    9: "dk",
    10: "ys",
    11: "wr",
    12: "blg",
    13: "kk",
    14: "nk",
    15: "ds",
    16: "pc",
    17: "ca",
    18: "dd",
    19: "kt",
    20: "jr",
    21: "bk",
    22: "fk",
    23: "rs",
    24: "sa_mii_m",
    25: "sa_mii_f",
    26: "sb_mii_m",
    27: "sb_mii_f",
    28: "sc_mii_m",
    29: "sc_mii_f",
    30: "ma_mii_m",
    31: "ma_mii_f",
    32: "mb_mii_m",
    33: "mb_mii_f",
    34: "mc_mii_m",
    35: "mc_mii_f",
    36: "la_mii_m",
    37: "la_mii_f",
    38: "lb_mii_m",
    39: "lb_mii_f",
    40: "lc_mii_m",
    41: "lc_mii_f",
    42: "m_mii",
    43: "s_mii",
    44: "l_mii",
}


CATEGORY_NAMES = {
    0: "ng",
    1: "glitch",
    2: "nosc",
    3: "tas",
    4: "ng_200cc",
    5: "glitch_200cc",
    6: "nosc_200cc",
}


@dataclass
class Timer:
    min: int
    sec: int
    mil: int

    def __str__(self):
        return f"{self.min}-{self.sec:02}-{self.mil:03}"


@dataclass
class Metadata:
    raceTime: Timer
    track: str
    vehicle: str
    character: str
    category: int

    def __str__(self):
        return f"{self.track}-{self.category}-{self.character}-{self.vehicle}-{self.raceTime}"


def parse_timer(bytes: bytes) -> Timer:
    # Timer is 3 bytes, split into minutes seconds and milliseconds
    timer_bytes = int.from_bytes(bytes, byteorder="big", signed=False)

    min = (timer_bytes >> 0x11)
    sec = (timer_bytes >> 0xA) & 0x7F
    mil = timer_bytes & 0x3FF

    return Timer(min, sec, mil)


def parse_course(bytes: bytes) -> str:
    # Course ID is upper 6 bits
    course_bytes = int.from_bytes(bytes, byteorder="big", signed=False)

    return COURSE_NAMES[course_bytes >> 2]


def parse_vehicle_and_character(bytes: bytes) -> Tuple[str, str]:
    # Vehicle ID is upper 6 bits, Character ID is the following 6 bits
    vehicle_character_bytes = int.from_bytes(bytes, byteorder="big", signed=False)

    vehicle = vehicle_character_bytes >> 0xA
    character = (vehicle_character_bytes >> 4) & 0x3F

    return VEHICLE_NAMES[vehicle], CHARACTER_NAMES[character]


def parse_category_bytes(bytes: bytes) -> str:
    category_bytes = int.from_bytes(bytes, byteorder="big", signed=False)
    tas_category = category_bytes >> 0x4
    category = category_bytes & 0xF

    if (category == 3):
        print("TAS category not handled yet!")
        exit(-1)

    return CATEGORY_NAMES[category]


def parse_category(f: BinaryIO) -> str:
    # Validate that CTGP metadata is present. This is not the case for old ghosts on the DB
    f.seek(-8, 2)
    if (f.read(4) != b'CKGD'):
        return "unknown"
    
    # Category is 13th byte from end of CTGP KRKG
    f.seek(-14, 2)

    return parse_category_bytes(f.read(1))


def parse_metadata(f: BinaryIO) -> Metadata:
    assert(f.read(4) == b'RKGD')

    time = parse_timer(f.read(3))
    course = parse_course(f.read(1))
    vehicle, character = parse_vehicle_and_character(f.read(2))
    category = parse_category(f)
    
    return Metadata(time, course, vehicle, character, category)


def rkg_files(dir: str):
    yield from os.listdir(dir)


def main():
    if len(sys.argv) < 2:
        print("Missing directory argument!")
        exit(-1)

    inDir = sys.argv[1]

    if not os.path.isdir(inDir):
        print(f"Could not find path: {inDir}")

    # For now, the output directory will just be a subfolder
    outDir = os.path.join(inDir, "output")
    os.makedirs(outDir, exist_ok=True)

    for file in rkg_files(inDir):
        filepath = os.path.join(inDir, file)

        if os.path.isdir(filepath):
            continue

        output_filename = ""

        with open(filepath, 'rb') as f:
            try:
                output_filename = f"{parse_metadata(f)}-{file}"
            except:
                print(f"Failed to parse metadata for {file}")
        
        shutil.copy2(filepath, os.path.join(outDir, output_filename))

if __name__ == '__main__':
    main()