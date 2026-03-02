import asyncio
from dolphin import *
import os
from responses import *
from typing import Optional

KINOKO_PATH = os.getenv("kinoko_path")
KINOKO_EXEC = os.getenv("kinoko_exec")

# Paths
GHOST_FILENAME = "ghost.rkg"
GHOST_PATH = os.path.join(KINOKO_PATH, GHOST_FILENAME)
RESULTS_PATH = os.path.join(KINOKO_PATH, "results.txt")

# ================================
#     REPLAY
# ================================


async def replay_clear_io():
    """
    Removes the last replay's ghost rkg and results files.
    """
    if os.path.exists(GHOST_PATH):
        os.remove(GHOST_PATH)

    if os.path.exists(RESULTS_PATH):
        os.remove(RESULTS_PATH)


async def replay_set_ghost(ghost: bytes):
    """
    Writes the provided ghost input bytes to a rkg file.
    """
    with open(GHOST_PATH, "wb") as f:
        f.write(ghost)


async def replay_run() -> int:
    """
    Executes an instance of Kinoko which replays the ghost file provided in replay_set_ghost.
    """
    args = ["replay", "-g", GHOST_FILENAME]
    proc = await asyncio.create_subprocess_exec(
        os.path.join(KINOKO_PATH, KINOKO_EXEC), *args, cwd=KINOKO_PATH
    )
    await proc.communicate()
    return proc.returncode


async def replay_results() -> Optional[str]:
    """
    Returns the contents of results.txt, if present.
    """
    try:
        with open(RESULTS_PATH, "r") as f:
            return f.read()
    except FileNotFoundError:
        return None


async def replay_exec(ghost: bytes, interaction: discord.Interaction):
    """
    Top-level function which does the following:
    - Removes previous replay's ghost rkg and results files
    - Writes the provided ghost input bytes to an rkg file
    - Launches a Kinoko instance which tries to replay the provided ghost inputs
    - Gives the user a response based off of Kinoko success/failure
    """
    await replay_clear_io()
    await replay_set_ghost(ghost)

    return_code = await replay_run()

    if return_code == 0:
        await respond_generic_success(
            interaction,
            "Run synced successfully!",
        )
        return

    if return_code != 1:
        await respond_bug_error(interaction, f"Unknown error! Return code {return_code}. Possibly a segfault")
        return

    fail = await replay_results()
    if fail:
        # Desync occurred! Generate a KRKG and display the KRKG desync information
        await dolphin_compare_krkg(ghost, interaction)
        return
    # Empty string is falsey, leading to two situations where fail is False
    elif os.path.exists(os.path.join(KINOKO_PATH, "results.txt")):
        await respond_bug_error(
            interaction, "Kinoko closed, but there's no explanation"
        )
        return

    await respond_bug_error(interaction, "Kinoko returned nothing")
