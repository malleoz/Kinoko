# ================================
#     RESPONSES
# ================================


async def respond_generic_error(
    interaction: discord.Interaction, error: str, tip: Optional[str] = None
):
    embed = discord.Embed(color=0xFF595E, title="Error!",
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
    embed = discord.Embed(color=0x5EFF59, title="Success!",
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
    embed = discord.Embed(color=0xFF595E, title="Fail!",
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
    embed = discord.Embed(color=0xFFCA3A, title="BUG!",
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