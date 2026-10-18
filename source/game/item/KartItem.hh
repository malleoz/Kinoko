#pragma once

#include "game/item/ItemInventory.hh"

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Item {

/// @brief State management for a kart's item usage
/// @details Checks controller inputs to see if the user is attempting to use an item. Checks
/// whether the user is able to use an item (countdown, burnout, in cannon, etc).
class KartItem final : Kart::KartObjectProxy {
public:
    KartItem();
    ~KartItem() override;

    void init(size_t playerIdx);
    void calc();
    void clear();
    void useMushroom();
    void activateMushroom();

    /// @beginGetters

    [[nodiscard]] ItemInventory &inventory();
    [[nodiscard]] const ItemInventory &inventory() const;

    /// @endGetters

private:
    /// @brief Flags pertaining to the ability to use an item and the controller input state
    enum class eFlags {
        Lockout = 10,              ///< The player cannot currently use an item
        ItemButtonHold = 12,       ///< The item button is currently being held down
        ItemButtonActivation = 14, ///< The item button was pressed this frame
    };

    /// @brief A bitfield of @ref eFlags that represents the state of item usage
    typedef EGG::TBitFlag<u16, eFlags> Flags;

    Flags m_flags;             ///< Bit flags for item usage state
    ItemInventory m_inventory; ///< The current item inventory for the kart
};

} // namespace Kinoko::Item
