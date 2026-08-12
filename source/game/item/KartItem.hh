#pragma once

#include "game/item/ItemInventory.hh"

#include "game/kart/KartObjectProxy.hh"

#include <egg/core/BitFlag.hh>

namespace Kinoko::Item {

/// @brief State management for a kart's item usage
/// @details Checks controller inputs to see if the user is attempting to use an item. Checks
/// whether the user is able to use an item (countdown, burnout, in cannon, etc).
class KartItem : Kart::KartObjectProxy {
public:
    KartItem();
    ~KartItem();

    void init(size_t playerIdx);
    void calc();

    /// @addr{0x80798848}
    /// @brief Clears the current item inventory if it is not empty
    void clear() {
        if (m_inventory.id() != ItemId::NONE) {
            m_inventory.clear();
        }
    }

    /// @addr{0x8079864C}
    /// @brief Applies a mushroom boost to the kart's movement
    void activateMushroom() {
        move()->activateMushroom();
    }

    /// @addr{0x807A9D3C}
    /// @brief Actives a mushroom boost and decrements the item count in the inventory slot
    void useMushroom() {
        activateMushroom();
        m_inventory.useItem(1);
    }

    /// @beginGetters
    [[nodiscard]] ItemInventory &inventory() {
        return m_inventory;
    }

    [[nodiscard]] const ItemInventory &inventory() const {
        return m_inventory;
    }
    /// @endGetters

private:
    /// @brief Flags pertaining to the ability to use an item and the controller input state
    enum class eFlags {
        Lockout = 10,              ///< The player cannot currently use an item
        ItemButtonHold = 12,       ///< The item button is currently being held down
        ItemButtonActivation = 14, ///< The item button was pressed this frame
    };
    typedef EGG::TBitFlag<u16, eFlags> Flags;

    Flags m_flags;             ///< Bit flags for item usage state
    ItemInventory m_inventory; ///< The current item inventory for the kart
};

} // namespace Kinoko::Item
