#pragma once

#include <Common.hh>

namespace Field {

enum class ObjectId {
    DummyPole = 0x066,
    DokanSFC = 0x12e,
    CastleTree1c = 0x130,
    PalmTree = 0x145,
    DKtreeA64c = 0x158,
    TownTreeDsc = 0x15b,
    OilSFC = 0x15d,
    ParasolR = 0x16e,
    PuchiPakkun = 0x1aa,
    Aurora = 0x204,
    Mdush = 0x217,
};

enum class BlacklistedObjectId {
    Itembox = 0x65,
    Hanabi = 0x16a,
};

static constexpr bool IsObjectBlacklisted(u16 id) {
    BlacklistedObjectId objectId = static_cast<BlacklistedObjectId>(id);
    switch (objectId) {
    // Disabled collision
    case BlacklistedObjectId::Itembox:
        return true;

    // No collision
    case BlacklistedObjectId::Hanabi:
        return true;

    default:
        return false;
    }
}

} // namespace Field
