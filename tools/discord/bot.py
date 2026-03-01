from . import responses

import asyncio
import discord
from dotenv import load_dotenv
import os
from typing import Optional

# Global variable management
load_dotenv()
TOKEN = os.getenv("token")
DOLPHIN_PATH = os.getenv("dolphin_path")
MKW_PATH = os.getenv("mkw_path")
SP_PATH = os.getenv("sp_path")
NAND_PATH = os.getenv("nand_path")
KINOKO_PATH = os.getenv("kinoko_path")
KINOKO_EXEC = os.getenv("kinoko_exec")
IS_GENERATING_KRKG = False
IS_REPLAYING_GHOST = False

DOLPHIN_TIMEOUT = 70.0  # Max time Dolphin will run
RKG_FILENAME = "test.rkg"  # The .rkg file to be replayed
KRKG_FILE_NAME = "test.krkg"  # The .krkg filename to be generated

# Paths
RKG_PATH = os.path.join(NAND_PATH, RKG_FILENAME)
KRKG_PATH = os.path.join(NAND_PATH, KRKG_FILE_NAME)
OK_PATH = os.path.join(NAND_PATH, "ok")
FAIL_PATH = os.path.join(NAND_PATH, "fail")  # if exists, error playing ghost
TIMEOUT_PATH = os.path.join(NAND_PATH, "timeout")  # Dolphin ran too long

client = discord.Client(intents=discord.Intents.default())
tree = discord.app_commands.CommandTree(client)

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

    if os.path.exists(timeout_path):
        os.remove(timeout_path)


async def dolphin_set_ghost(ghost: bytes):
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


async def dolphin_krkg() -> Optional[bytes]:
    """
    If krkg file exists, returns the contents of the file, otherwise None
    """
    try:
        with open(KRKG_PATH, "rb") as f:
            return f.read()
    except FileNotFoundError:
        return None


async def dolphin_generate_krkg(ghost: bytes, interaction: discord.Interaction):
    await dolphin_clear_io()
    await dolphin_set_ghost(ghost)

    await dolphin_run()

    if await dolphin_ok():
        krkg = await dolphin_krkg()
        if not krkg:
            await responses.respond_bug_error(
                interaction, "Dolphin returned OK, but there's no KRKG"
            )
            return

        await responses.respond_krkg_success(
            interaction,
            discord.File(KRKG_PATH),
        )
        return

    fail = await dolphin_fail()
    if fail:
        await responses.respond_fail_error(interaction, fail)
        return
    # Empty string is falsey, leading to two situations where fail is False
    elif os.path.exists(os.path.join(NAND_PATH, "fail")):
        await responses.respond_bug_error(
            interaction, "Dolphin returned fail, but there's no explanation"
        )
        return
    elif os.path.exists(os.path.join(NAND_PATH, "timeout")):
        await responses.respond_fail_error(
            interaction, f"Ghost replay exceeded {DOLPHIN_TIMEOUT:.0f} second limit. \
                It's likely the ghost doesn't sync on console."
        )
        return

    await responses.respond_bug_error(interaction, "Dolphin returned nothing")


# ================================
#     REPLAY
# ================================


async def replay_clear_io():
    ghost_path = os.path.join(KINOKO_PATH, "ghost.rkg")
    results_path = os.path.join(KINOKO_PATH, "results.txt")

    if os.path.exists(ghost_path):
        os.remove(ghost_path)

    if os.path.exists(results_path):
        os.remove(results_path)


async def replay_set_ghost(ghost: bytes):
    with open(os.path.join(KINOKO_PATH, "ghost.rkg"), "wb") as f:
        f.write(ghost)


async def replay_run() -> int:
    args = ["replay", "-g", "ghost.rkg"]
    proc = await asyncio.create_subprocess_exec(
        os.path.join(KINOKO_PATH, KINOKO_EXEC), *args, cwd=KINOKO_PATH
    )
    await proc.communicate()
    return proc.returncode


async def replay_results() -> Optional[str]:
    results_path = os.path.join(KINOKO_PATH, "results.txt")
    if os.path.exists(results_path):
        with open(results_path, "r") as f:
            return f.read()
    else:
        return None


async def replay_exec(ghost: bytes, interaction: discord.Interaction):
    await replay_clear_io()
    await replay_set_ghost(ghost)

    print("About to replay run")

    return_code = await replay_run()

    if return_code == 0:
        await responses.respond_generic_success(
            interaction,
            "Run synced successfully!",
        )
        return

    if return_code != 1:
        await responses.respond_bug_error(interaction, f"Unknown error! Return code {return_code}. Possibly a segfault")
        return

    fail = await replay_results()
    if fail:
        await responses.respond_fail_error(
            interaction,
            fail.split("\n")[1],
            tip="This is a desync! Send the ghost in the Kinoko Discord server!",
        )
        return
    # Empty string is falsey, leading to two situations where fail is False
    elif os.path.exists(os.path.join(KINOKO_PATH, "results.txt")):
        await responses.respond_bug_error(
            interaction, "Kinoko closed, but there's no explanation"
        )
        return

    await responses.respond_bug_error(interaction, "Kinoko returned nothing")


# ================================
#     EVENTS
# ================================


@client.event
async def on_ready():
    await tree.sync()
    print(f"Synced command tree and logged in as {client.user}")


# ================================
#     COMMANDS
# ================================


@tree.command(name="ping")
async def command_ping(interaction: discord.Interaction):
    await interaction.response.send_message("Pong!", ephemeral=True)


@tree.command(name="generate_krkg")
async def command_generate_krkg(
    interaction: discord.Interaction, ghost: discord.Attachment
):
    global IS_GENERATING_KRKG

    # Log when people generate KRKGs
    print(f"{interaction.user.name} is requesting to generate a KRKG!")

    # Check if the file claims to be a ghost
    file_extension = ghost.filename.split(".")[-1]
    if file_extension != "rkg":
        await responses.respond_generic_error(
            interaction,
            "File is not an RKG",
            "Double check that the file extension is .rkg!",
        )
        return

    # Check if the size is too small or big to be a ghost
    if ghost.size < 0x8C or ghost.size > 0x2800:
        await responses.respond_generic_error(
            interaction,
            f"File is too {"small" if ghost.size < 0x8c else "big"} to be an RKG",
        )
        return

    # We can only generate one KRKG at a time, due to how we I/O with Dolphin
    # This theoretically creates a race condition, but with low traffic in mind,
    # the chances of a collision are incredibly unlikely
    # TODO: This command should use a mutex to add to a queue that another thread reads from
    if IS_GENERATING_KRKG:
        await responses.respond_generic_error(
            interaction,
            "Already generating a KRKG",
            "I can only handle one ghost at a time. Try again later!",
        )
        return

    IS_GENERATING_KRKG = True
    await interaction.response.defer(ephemeral=True, thinking=True)
    await dolphin_generate_krkg(await ghost.read(), interaction)
    IS_GENERATING_KRKG = False


@tree.command(name="replay_ghost")
async def command_replay_ghost(
    interaction: discord.Interaction, ghost: discord.Attachment
):
    global IS_REPLAYING_GHOST

    # Log when people generate KRKGs
    print(f"{interaction.user.name} is requesting to replay a ghost!")

    # Check if the file claims to be a ghost
    file_extension = ghost.filename.split(".")[-1]
    if file_extension != "rkg":
        await responses.respond_generic_error(
            interaction,
            "File is not an RKG",
            "Double check that the file extension is .rkg!",
        )
        return

    # Check if the size is too small or big to be a ghost
    if ghost.size < 0x8C or ghost.size > 0x2800:
        await responses.respond_generic_error(
            interaction,
            f"File is too {"small" if ghost.size < 0x8c else "big"} to be an RKG",
        )
        return

    # We can only replay one ghost at a time, due to how we I/O with Kinoko
    # This theoretically creates a race condition, but with low traffic in mind,
    # the chances of a collision are incredibly unlikely
    # TODO: This command should use a mutex to add to a queue that another thread reads from
    if IS_REPLAYING_GHOST:
        await responses.respond_generic_error(
            interaction,
            "Already replaying a ghost",
            "I can only handle one ghost at a time. Try again later!",
        )
        return

    IS_REPLAYING_GHOST = True
    await interaction.response.defer(ephemeral=True, thinking=True)
    await replay_exec(await ghost.read(), interaction)
    IS_REPLAYING_GHOST = False


if __name__ == "__main__":
    assert TOKEN, 'Missing token for Discord bot, ensure "token" exists in environment'
    assert (
        DOLPHIN_PATH
    ), 'Missing path for Dolphin, ensure "dolphin_path" exists in environment'
    assert (
        MKW_PATH
    ), 'Missing path for MKW ISO, ensure "mkw_path" exists in environment'
    assert SP_PATH, 'Missing path for SP DOL, ensure "sp_path" exists in environment'
    assert (
        NAND_PATH
    ), 'Missing path for save data, ensure "nand_path" exists in environment'
    assert (
        KINOKO_PATH
    ), 'Missing path for Kinoko directory, ensure "kinoko_path" exists in environment'
    assert (
        KINOKO_EXEC
    ), 'Missing path for Kinoko executable, ensure "kinoko_exec" exists in environment'

    client.run(TOKEN)
