from dolphin import *
from info import *
from replay import *
from responses import *

import discord
from dotenv import load_dotenv
import os

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

RKG_MIN_SIZE = 0x8C
RKG_MAX_SIZE = 0x2800

client = discord.Client(intents=discord.Intents.default())
tree = discord.app_commands.CommandTree(client)


async def valid_rkg(ghost: discord.Attachment, interaction: discord.Interaction) -> bool:
    # Check if the file claims to be a ghost
    file_extension = ghost.filename.split(".")[-1]
    if file_extension != "rkg":
        await respond_generic_error(
            interaction,
            "File is not an RKG",
            "Double check that the file extension is .rkg!",
        )
        return False

    # Check if the size is too small or big to be a ghost
    if ghost.size < RKG_MIN_SIZE or ghost.size > RKG_MAX_SIZE:
        await respond_generic_error(
            interaction,
            f"File is too {"small" if ghost.size < RKG_MIN_SIZE else "big"} to be an RKG",
        )
        return False

    # Check that the first 4 bytes are 'RKGD'
    data = await ghost.read()
    if data[:4] != b'RKGD':
        await respond_generic_error(
            interaction,
            "File is not an RKG",
            "Double check the contents of the file",
        )
        return False

    return True


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

    if not await valid_rkg(ghost, interaction):
        return

    # We can only generate one KRKG at a time, due to how we I/O with Dolphin
    # This theoretically creates a race condition, but with low traffic in mind,
    # the chances of a collision are incredibly unlikely
    # TODO: This command should use a mutex to add to a queue that another thread reads from
    if IS_GENERATING_KRKG:
        await respond_generic_error(
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

    # Log when people replay ghosts
    print(f"{interaction.user.name} is requesting to replay a ghost!")

    if not await valid_rkg(ghost, interaction):
        return

    # We can only replay one ghost at a time, due to how we I/O with Kinoko
    # This theoretically creates a race condition, but with low traffic in mind,
    # the chances of a collision are incredibly unlikely
    # TODO: This command should use a mutex to add to a queue that another thread reads from
    if IS_REPLAYING_GHOST:
        await respond_generic_error(
            interaction,
            "Already replaying a ghost",
            "I can only handle one ghost at a time. Try again later!",
        )
        return

    IS_REPLAYING_GHOST = True
    await interaction.response.defer(ephemeral=True, thinking=True)
    await replay_exec(await ghost.read(), interaction)
    IS_REPLAYING_GHOST = False


@tree.command(name="get_ghost_info")
async def command_get_ghost_info(
        interaction: discord.Interaction, ghost: discord.Attachment):
    # Log when people request ghost info
    print(f"{interaction.user.name} is requesting ghost info!")

    if not await valid_rkg(ghost, interaction):
        return

    embed = await embed_ghost_info(await ghost.read())
    await interaction.response.send_message(embed=embed, ephemeral=True)

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
