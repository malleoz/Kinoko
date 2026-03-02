import discord
from info import *
from typing import Optional

# ================================
#     RESPONSES
# ================================

ERROR_COLOR = 0xFF595E
BUG_COLOR = 0xFFCA3A
SUCCESS_COLOR = 0x5EFF59


async def respond_generic_error(
    interaction: discord.Interaction, error: str, tip: Optional[str] = None
):
    """
    Used when an error occurs not related to actual replay or krkg generation, such as:
    - File provided is not an rkg
    - Someone is already generating a KRKG
    """
    embed = discord.Embed(color=ERROR_COLOR, title="Error!",
                          description=f"### {error}")
    embed.set_footer(text=tip)
    if (
        interaction.response.type
        == discord.InteractionResponseType.deferred_channel_message
    ):
        await interaction.followup.send(embed=embed, ephemeral=True)
    else:
        await interaction.response.send_message(embed=embed, ephemeral=True)


async def respond_generic_success(
    interaction: discord.Interaction, msg: str, tip: Optional[str] = None
):
    embed = discord.Embed(color=SUCCESS_COLOR, title="Success!",
                          description=f"### {msg}")
    embed.set_footer(text=tip)

    if (
        interaction.response.type
        == discord.InteractionResponseType.deferred_channel_message
    ):
        await interaction.followup.send(embed=embed, ephemeral=True)
    else:
        await interaction.response.send_message(embed=embed, ephemeral=True)


async def respond_fail_error(
    interaction: discord.Interaction, error: str, tip: Optional[str] = None
):
    embed = discord.Embed(color=ERROR_COLOR, title="Fail!",
                          description=f"### {error}")
    embed.set_footer(text=tip)

    if (
        interaction.response.type
        == discord.InteractionResponseType.deferred_channel_message
    ):
        await interaction.followup.send(embed=embed, ephemeral=True)
    else:
        await interaction.response.send_message(embed=embed, ephemeral=True)


async def respond_bug_error(interaction: discord.Interaction, error: str):
    embed = discord.Embed(color=BUG_COLOR, title="BUG!",
                          description=f"### {error}")
    embed.set_footer(
        text="This should never be visible. "
        "Please report this to a Kinoko developer if you see this!"
    )

    if (
        interaction.response.type
        == discord.InteractionResponseType.deferred_channel_message
    ):
        await interaction.followup.send(embed=embed, ephemeral=True)
    else:
        await interaction.response.send_message(embed=embed, ephemeral=True)


async def respond_krkg_success(interaction: discord.Interaction, file: discord.File):
    if (
        interaction.response.type
        == discord.InteractionResponseType.deferred_channel_message
    ):
        await interaction.followup.send(file=file, ephemeral=True)
    else:
        await interaction.response.send_message(file=file, ephemeral=True)


async def respond_desync_error(ghost: bytes, interaction: discord.Interaction, error: str):
    info_embed = await embed_ghost_info(ghost)
    embed = discord.Embed(color=BUG_COLOR, title="DESYNC!",
                          description=f"### {error}")

    embed.set_footer(
        text="Please send this ghost file to a Kinoko developer if you see this!"
    )

    if (
        interaction.response.type
        == discord.InteractionResponseType.deferred_channel_message
    ):
        await interaction.followup.send(embeds=[info_embed, embed], ephemeral=True)
    else:
        await interaction.response.send_message(embeds=[info_embed, embed], ephemeral=True)
