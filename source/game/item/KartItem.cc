#include "KartItem.hh"

#include "game/kart/KartMove.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Item {

/// @addr{0x80797928}
/// @brief Calculates item activation based on the controller input state
/// @details If the player is pressing the item button and was not pressing it on the previous
/// frame, then sets the @ref eFlags::ItemButtonActivation flag. If the @ref eFlags::Lockout flag
/// was set, checks if the flag should be cleared (e.g. after a respawn or after landing from a
/// cannon). If @ref eFlags::Lockout was not set this frame, then checks to see if the flag should
/// be set (e.g. starting a respawn or entering a cannon). Otherwise, checks to see if the player
/// can use an item this frame, requiring that @ref eFlags::ItemButtonActivation is set, we are past
/// the race countdown, the kart is not in action or burnout, and the inventory is not empty. If
/// those conditions are met, then calls @ref useMushroom().
void KartItem::calc() {
    bool prevButton = m_flags.onBit(eFlags::ItemButtonHold);
    m_flags.resetBit(eFlags::ItemButtonHold, eFlags::ItemButtonActivation);

    const auto &currentInputs = inputs()->currentState();
    if (currentInputs.item()) {
        m_flags.setBit(eFlags::ItemButtonHold).changeBit(!prevButton, eFlags::ItemButtonActivation);
    }

    auto &status = KartObjectProxy::status();

    if (m_flags.onBit(eFlags::Lockout)) {
        if (status.offBit(Kart::eStatus::BeforeRespawn, Kart::eStatus::InAction,
                    Kart::eStatus::TriggerRespawn, Kart::eStatus::CannonStart,
                    Kart::eStatus::InCannon, Kart::eStatus::AfterCannon)) {
            m_flags.resetBit(eFlags::Lockout);
        }
    } else {
        if (status.onBit(Kart::eStatus::InRespawn, Kart::eStatus::InAction,
                    Kart::eStatus::TriggerRespawn, Kart::eStatus::CannonStart,
                    Kart::eStatus::InCannon)) {
            m_flags.setBit(eFlags::Lockout);
        } else {
            const auto *raceMgr = System::RaceManager::Instance();
            bool canUse = m_flags.onBit(eFlags::ItemButtonActivation);
            canUse = canUse && raceMgr->isStageReached(System::RaceManager::Stage::Race);
            canUse = canUse && status.offBit(Kart::eStatus::InAction, Kart::eStatus::Burnout);
            canUse = canUse && (m_inventory.id() != ItemId::NONE);

            if (canUse) {
                // For now, assume only time trials are valid and use a mushroom
                useMushroom();
            }
        }
    }
}

/// @addr{0x8079864C}
/// @brief Applies a mushroom boost to the kart's movement
void KartItem::activateMushroom() {
    move()->activateMushroom();
}

} // namespace Kinoko::Item
