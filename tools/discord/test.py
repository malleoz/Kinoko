import asyncio
import discord
import os
from responses import *
from save import *
from typing import Tuple

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


async def test_run() -> Tuple[int, str]:
    """
    Executes an instance of Kinoko which tests a ghost file against a krkg.
    Returns the status code and stdout output
    """
    args = ["test", "-g", GHOST_FILENAME, "-k", KRKG_FILENAME]

    try:
        proc = await asyncio.create_subprocess_exec(
            os.path.join(KINOKO_PATH, KINOKO_EXEC),
            *args,
            cwd=KINOKO_PATH,
            stdout=asyncio.subprocess.PIPE
        )

        stdout, _ = await asyncio.wait_for(
            proc.communicate(),
            timeout=KINOKO_TIMEOUT
        )
        result = stdout.decode()
    except TimeoutError:
        await respond_fail_error(f"Run desynced in replay mode, but upon running in test mode, \
                                 it exceeds the {KINOKO_TIMEOUT:.0} second limit")
        result = ""

    return (proc.returncode, result)


async def kinoko_test_rkg(ghost: bytes, krkg: bytes, interaction: discord.Interaction):
    await test_clear_io()
    await test_set_ghost_krkg(ghost, krkg)

    return_code, stdout = await test_run()

    if (return_code == 0):
        await respond_bug_error(
            interaction,
            "Run desynced in replay mode, but upon running in test mode, it syncs!",
        )
        return
    else:
        # Save ghost and krkg so that it can be analyzed later
        await save_to_folder(folder_name="desyncs", ghost=ghost, krkg=krkg)

        # Format the stdout output so that it just has desync information
        # Remove "Test Cast Failed" line
        stdout = stdout.splitlines()
        frames = stdout[0].split('[')[-1][:-1]
        stdout = stdout[1:]

        # Lines go name, expected, observed
        desyncs = []
        for i in range(0, len(stdout), 3):
            name = ''.join(stdout[i].split(' ')[4:])
            expected = ' '.join(stdout[i + 1].split(' ')[3:])
            observed = ' '.join(stdout[i + 2].split(' ')[3:])

            desyncs.append((name, expected, observed))

        description = "" \
            f"**Desync Frame**: {frames}\n" \
            "\n" \
            "### Variable-Specific Desyncs:\n"

        for desync in desyncs:
            description += "" \
                f"**Name:** {desync[0]}\n" \
                f"**Expected:** {desync[1]}\n" \
                f"**Observed:** {desync[2]}\n\n" \

        await respond_desync_error(
            ghost,
            interaction,
            description,
        )
        return
