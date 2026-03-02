from dataclasses import dataclass
import discord
from typing import BinaryIO, List, Optional, Tuple


RKG_MIN_SIZE = 0x8C
RKG_MAX_SIZE = 0x2800


COURSE_NAMES = {
    0: "Mario Circuit",
    1: "Moo Moo Meadows",
    2: "Mushroom Gorge",
    3: "Grumble Volcano",
    4: "Toad's Factory",
    5: "Coconut Mall",
    6: "DK Summit",
    7: "Wario's Gold Mine",
    8: "Luigi Circuit",
    9: "Daisy Circuit",
    10: "Moonview Highway",
    11: "Maple Treeway",
    12: "Bowser's Castle",
    13: "Rainbow Road",
    14: "Dry Dry Ruins",
    15: "Koopa Cape",
    16: "GCN Peach Beach",
    17: "GCN Mario Circuit",
    18: "GCN Waluigi Stadium",
    19: "GCN DK Mountain",
    20: "DS Yoshi Falls",
    21: "DS Desert Hills",
    22: "DS Peach Gardens",
    23: "DS Delfino Square",
    24: "SNES Mario Circuit 3",
    25: "SNES Ghost Valley 2",
    26: "N64 Mario Raceway",
    27: "N64 Sherbet Land",
    28: "N64 Bowser's Castle",
    29: "N64 DK's Jungle Parkway",
    30: "GBA Bowser Castle 3",
    31: "GBA Shy Guy Beach",
}


VEHICLE_NAMES = {
    0: "Standard Kart S",
    1: "Standard Kart M",
    2: "Standard Kart L",
    3: "Baby Booster",
    4: "Classic Dragster",
    5: "Offroader",
    6: "Mini Beast",
    7: "Wild Wing",
    8: "Flame Flyer",
    9: "Cheep Charger",
    10: "Super Blooper",
    11: "Piranha Prowler",
    12: "Tiny Titan",
    13: "Daytripped",
    14: "Jetsetter",
    15: "Blue Falcon",
    16: "Sprinter",
    17: "Honeycoupe",
    18: "Standard Bike S",
    19: "Standard Bike M",
    20: "Standard Bike L",
    21: "Bullet Bike",
    22: "Mach Bike",
    23: "Flame Runner",
    24: "Bit Bike",
    25: "Sugarscoot",
    26: "Wario Bike",
    27: "Quacker",
    28: "Zip Zip",
    29: "Shooting Star",
    30: "Magikruiser",
    31: "Sneakster",
    32: "Spear",
    33: "Jet Bubble",
    34: "Dolphin Dasher",
    35: "Phantom",
}


CHARACTER_NAMES = {
    0: "Mario",
    1: "Baby Peach",
    2: "Waluigi",
    3: "Bowser",
    4: "Baby Daisy",
    5: "Dry Bones",
    6: "Baby Mario",
    7: "Luigi",
    8: "Toad",
    9: "Donkey Kong",
    10: "Yoshi",
    11: "Wario",
    12: "Baby Luigi",
    13: "Toadette",
    14: "Koopa Troopa",
    15: "Daisy",
    16: "Peach",
    17: "Birdo",
    18: "Diddy Kong",
    19: "King Boo",
    20: "Bowser Jr.",
    21: "Dry Bowser",
    22: "Funky Kong",
    23: "Rosalina",
    24: "Small Mii Outfit A (Male)",
    25: "Small Mii Outfit A (Female)",
    26: "Small Mii Outfit B (Male)",
    27: "Small Mii Outfit B (Female)",
    28: "Small Mii Outfit C (Male)",
    29: "Small Mii Outfit C (Female)",
    30: "Medium Mii Outfit A (Male)",
    31: "Medium Mii Outfit A (Female)",
    32: "Medium Mii Outfit B (Male)",
    33: "Medium Mii Outfit B (Female)",
    34: "Medium Mii Outfit C (Male)",
    35: "Medium Mii Outfit C (Female)",
    36: "Large Mii Outfit A (Male)",
    37: "Large Mii Outfit A (Female)",
    38: "Large Mii Outfit B (Male)",
    39: "Large Mii Outfit B (Female)",
    40: "Large Mii Outfit C (Male)",
    41: "Large Mii Outfit C (Female)",
    42: "Medium Mii",
    43: "Small Mii",
    44: "Large Mii",
}


CATEGORY_NAMES = {
    0: "No-Glitch",
    1: "Glitch",
    2: "No-SC",
    3: "TAS",
    4: "200CC No-Glitch",
    5: "200cc Glitch",
    6: "200cc No-SC",
}


CONTROLLER_NAMES = {
    0: "Wii Wheel",
    1: "WiiMote + Nunchuck",
    2: "Classic Controller",
    3: "GameCube Controller",
}


@dataclass
class Timer:
    min: int
    sec: int
    mil: int

    def __str__(self):
        time = f"{self.min}:{self.sec:02}.{self.mil:03}"
        if self.min == 0:
            time = time[2:]
        return time


@dataclass
class Metadata:
    race_time: Timer
    track: str
    vehicle: str
    character: str
    controller: str
    lap_times: List[Timer]
    drift: str
    player_name: str
    ctgp_ghost: bool


async def parse_timer(ghost: bytes, lap: int) -> Timer:
    """
    Returns a Timer representation for the provided lap. If lap is -1, then it's the race time.
    """
    bytes = ghost[4:7] if lap == -1 else ghost[17 + (lap * 3): 20 + (lap * 3)]
    # Timer is 3 bytes, split into minutes seconds and milliseconds
    timer_bytes = int.from_bytes(bytes, byteorder="big", signed=False)

    min = (timer_bytes >> 0x11)
    sec = (timer_bytes >> 0xA) & 0x7F
    mil = timer_bytes & 0x3FF

    return Timer(min, sec, mil)


async def parse_lap_count(ghost: bytes) -> int:
    return ghost[16]


async def parse_course(ghost: bytes) -> str:
    # Course ID is upper 6 bits
    return COURSE_NAMES[ghost[7] >> 2]


async def parse_vehicle_and_character(ghost: bytes) -> Tuple[str, str]:
    # Vehicle ID is upper 6 bits, Character ID is the following 6 bits
    vehicle_character_bytes = int.from_bytes(
        ghost[8:10], byteorder="big", signed=False)

    vehicle = vehicle_character_bytes >> 0xA
    character = (vehicle_character_bytes >> 4) & 0x3F

    return VEHICLE_NAMES[vehicle], CHARACTER_NAMES[character]


async def parse_controller(ghost: bytes) -> str:
    # Controller ID is upper 4 bits
    controller_bytes = ghost[11]
    controller = controller_bytes >> 4

    return CONTROLLER_NAMES[controller]


async def parse_drift_type(ghost: bytes) -> int:
    # 1 for automatic, 0 for manual
    drift_bytes = ghost[13]
    drift = (drift_bytes >> 1) & 0x1

    return "Automatic" if drift == 1 else "Manual"


async def parse_player_name(ghost: bytes) -> int:
    MII_NAME_LENGTH = 10

    mii_data = ghost[60:134]
    mii_name_bytes = mii_data[2:22]

    return mii_name_bytes.decode("utf-16be")


async def is_ctgp_ghost(ghost: bytes) -> bool:
    """
    Checks if the provided ghost was created using CTGP.
    We can tell based on the presence of the "CKGD" magic bytes near the end of the file.
    """
    return ghost[-8:-4] == b'CKGD'


async def parse_metadata(ghost: bytes) -> Metadata:
    race_time = await parse_timer(ghost, -1)
    course = await parse_course(ghost)
    vehicle, character = await parse_vehicle_and_character(ghost)
    controller = await parse_controller(ghost)
    lap_times = [await parse_timer(ghost, i) for i in range(await parse_lap_count(ghost))]
    drift_type = await parse_drift_type(ghost)
    player_name = await parse_player_name(ghost)
    ctgp_ghost = await is_ctgp_ghost(ghost)

    return Metadata(race_time, course, vehicle, character, controller, lap_times, drift_type, player_name, ctgp_ghost)


async def embed_ghost_info(ghost: bytes) -> Optional[discord.Embed]:
    metadata = await parse_metadata(ghost)

    TITLE = "Ghost Overview"

    description = "" \
        f"**{metadata.track} - {metadata.race_time} by {metadata.player_name}**\n" \
        "\n" \
        f"**Character:** {metadata.character}\n" \
        f"**Vehicle:** {metadata.vehicle}\n" \
        f"**Drift:** {metadata.drift}\n" \
        "\n" \
        "**Lap Times**\n"

    for i in range(len(metadata.lap_times)):
        description += f"Lap {i+1}: {metadata.lap_times[i]}\n"

    return discord.Embed(color=0xFFFFFF, title=TITLE, description=description)
