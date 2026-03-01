import asyncio
import discord
from dotenv import load_dotenv
import os
from responses import *

DOLPHIN_PATH = os.getenv("dolphin_path")
MKW_PATH = os.getenv("mkw_path")
SP_PATH = os.getenv("sp_path")
NAND_PATH = os.getenv("nand_path")

DOLPHIN_TIMEOUT = 70.0  # Max time Dolphin will run
RKG_FILENAME = "test.rkg"  # The .rkg file to be replayed
KRKG_FILE_NAME = "test.krkg"  # The .krkg filename to be generated

# Paths
RKG_PATH = os.path.join(NAND_PATH, RKG_FILENAME)
KRKG_PATH = os.path.join(NAND_PATH, KRKG_FILE_NAME)
OK_PATH = os.path.join(NAND_PATH, "ok")
FAIL_PATH = os.path.join(NAND_PATH, "fail")  # if exists, error playing ghost
TIMEOUT_PATH = os.path.join(NAND_PATH, "timeout")  # Dolphin ran too long

# ================================
#     DOLPHIN
# ================================


async def dolphin_clear_io() -> None:
    """
    Removes files in the NAND that pertain to KRKG generation and success/failure validation.
    """
    files_to_remove = [
        RKG_PATH,
        KRKG_PATH,
        OK_PATH,
        FAIL_PATH,
        TIMEOUT_PATH,
    ]

    for file in files_to_remove:
        if os.path.exists(file):
            os.remove(file)

    if os.path.exists(TIMEOUT_PATH):
        os.remove(TIMEOUT_PATH)


async def dolphin_write_rkg(ghost: bytes):
    """
    Writes the provided array of bytes to RKG_PATH.
    """
    with open(RKG_PATH, "wb") as f:
        f.write(ghost)


async def dolphin_run():
    """
    Spawns a Dolphin subprocess which executes dolphin-emu-nogui with the following settings:
      - EmulationSpeed=0 ==> Unlimited Speed
      - Null Renderer ==> No graphics output
      - Headless Profile ==> No GUI
      - No Audio Output
      - Runs SP_PATH (the MKW-SP patch .dol)
      - Default ISO ==> Boots into MKW after patch is loaded
    Enforces a max runtime duration of DOLPHIN_TIMEOUT to avoid getting stuck indefinitely.
    """
    uncap_speed = ["-C", "Dolphin.Core.EmulationSpeed=0"]
    null_renderer = ["-v", "Null"]
    headless_profile = ["-p", "headless"]
    no_audio = ["-C", 'Dolphin.DSP.Backend="No Audio Output"']
    default_iso = ["-C", f"Dolphin.Core.DefaultISO={MKW_PATH}"]
    mmu = ["-C", "Dolphin.Core.MMU=True"]

    try:
        proc = await asyncio.create_subprocess_exec(
            DOLPHIN_PATH,
            "-e",
            SP_PATH,
            *uncap_speed,
            *headless_profile,
            *null_renderer,
            *no_audio,
            *default_iso,
            *mmu,
        )

        await asyncio.wait_for(
            proc.communicate(),
            timeout=DOLPHIN_TIMEOUT
        )
    except asyncio.TimeoutError:
        # Write a timeout file to nand
        timeout_path = os.path.join(NAND_PATH, "timeout")
        with open(timeout_path, 'w'):
            os.utime(timeout_path, None)


async def dolphin_ok() -> bool:
    """
    Checks for the presence of the "ok" file.
    If present, then the ghost replay succeeded and the krkg was generated.
    """
    return os.path.exists(OK_PATH)


async def dolphin_fail() -> Optional[str]:
    """
    If "fail" file exists, returns the contents of the file, otherwise None.
    """
    try:
        with open(FAIL_PATH, "r") as f:
            return f.read()
    except FileNotFoundError:
        return None


async def dolphin_krkg() -> bool:
    """
    Checks for the presence of the .krkg file.
    """
    return os.path.exists(KRKG_PATH)


async def dolphin_generate_krkg(ghost: bytes, interaction: discord.Interaction):
    """
    Deletes all test-related files, writes the provided ghost input bytes to test.rkg,
    runs the Dolphin process, and checks for success or failure.
    """
    await dolphin_clear_io()
    await dolphin_write_rkg(ghost)

    await dolphin_run()

    if await dolphin_ok():
        if await dolphin_krkg():
            await respond_krkg_success(
                interaction,
                discord.File(KRKG_PATH),
            )
        else:
            await respond_bug_error(
                interaction, "Dolphin returned OK, but there's no KRKG"
            )
        return

    # If "ok" wasn't present, something went wrong
    fail = await dolphin_fail()
    if fail:
        await respond_fail_error(interaction, fail)
        return
    # Empty string is falsey, leading to two situations where fail is False
    elif os.path.exists(FAIL_PATH):
        await respond_bug_error(
            interaction, "Dolphin returned fail, but there's no explanation"
        )
        return
    elif os.path.exists(TIMEOUT_PATH):
        await respond_fail_error(
            interaction, f"Ghost replay exceeded {DOLPHIN_TIMEOUT:.0f} second limit. \
                It's likely the ghost doesn't sync on console."
        )
        return

    await respond_bug_error(interaction, "Dolphin returned nothing")