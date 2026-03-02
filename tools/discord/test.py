import asyncio
import discord
import os
from responses import *
from typing import Optional

KINOKO_PATH = os.getenv("kinoko_path")
KINOKO_EXEC = os.getenv("kinoko_exec")

# Paths
GHOST_FILENAME = "ghost.rkg"
KRKG_FILENAME = "ghost.krkg"
GHOST_PATH = os.path.join(KINOKO_PATH, GHOST_FILENAME)
KRKG_PATH = os.path.join(KINOKO_PATH, KRKG_FILENAME)
RESULTS_PATH = os.path.join(KINOKO_PATH, "results.txt")

KINOKO_TIMEOUT = 10.0

# ================================
#     TEST
# ================================


async def test_clear_io():
    """
    Removes the last test's ghost rkg and results files.
    """
    if os.path.exists(GHOST_PATH):
        os.remove(GHOST_PATH)

    if os.path.exists(KRKG_PATH):
        os.remove(KRKG_PATH)

    if os.path.exists(RESULTS_PATH):
        os.remove(RESULTS_PATH)


async def test_set_ghost_krkg(ghost: bytes, krkg: bytes):
    """
    Writes the provided ghost input bytes to a rkg file,
    and writes the provided krkg bytes to a krkg file
    """
    with open(GHOST_PATH, "wb") as f:
        f.write(ghost)

    with open(KRKG_PATH, "wb") as f:
        f.write(krkg)


async def test_run() -> int:
    """
    Executes an instance of Kinoko which tests a ghost file against a krkg.
    """
    args = ["test", "-g", GHOST_FILENAME, "-k", KRKG_FILENAME]

    try:
        proc = await asyncio.create_subprocess_exec(
            os.path.join(KINOKO_PATH, KINOKO_EXEC), *args, cwd=KINOKO_PATH
        )

        await asyncio.wait_for(
            proc.communicate(),
            timeout=KINOKO_TIMEOUT
        )
    except TimeoutError:
        await respond_fail_error(f"Run desynced in replay mode, but upon running in test mode, \
                                 it exceeds the {KINOKO_TIMEOUT:.0} second limit")

    return proc.returncode


async def kinoko_test_rkg(ghost: bytes, krkg: bytes, interaction: discord.Interaction):
    await test_clear_io()
    await test_set_ghost_krkg(ghost, krkg)

    return_code = await test_run()

    if (return_code == 0):
        await respond_bug_error(
            interaction,
            "Run desynced in replay mode, but upon running in test mode, it syncs!",
        )
        return
    else:
        await respond_bug_error(
            interaction,
            "This ghost doesn't sync! TODO: Add more info",
        )
        return
