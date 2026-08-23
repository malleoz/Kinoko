#pragma once

#include "game/item/ItemId.hh"

namespace Kinoko::Item {

/// @brief Represents the item slot for a particular player
/// @details Tracks the current item id in the slot. For items such as triple mushrooms, it also
/// tracks the number of uses remaining.
class ItemInventory {
public:
    ItemInventory() = default;

    ~ItemInventory() = default;

    /// @addr{0x807BC940}
    /// @brief Adds the specified @ref ItemId to the player's inventory
    /// @details For the purposes of Kinoko, we simplify logic in this function to just assign a
    /// count of 3 for the triple mushroom item.
    void setItem(ItemId id) {
        constexpr int MUSHROOM_COUNT = 3;

        m_currentId = id;
        m_currentCount = MUSHROOM_COUNT;
    }

    /// @addr{0x807BC97C}
    /// @brief Updates the count to reflect using an item, clearing the id if the count reaches 0
    void useItem(int count) {
        m_currentCount -= count;
        if (m_currentCount > 0) {
            return;
        }

        clear();
    }

    /// @addr{0x807BC9C0}
    /// @brief Clears the item slot and resets the count to 0
    /// @details This is called when the player either uses up the item in their slot, fall
    /// out-of-bounds, or when they hit an object whose reaction causes the player to lose their
    /// item.
    void clear() {
        m_currentId = ItemId::NONE;
        m_currentCount = 0;
    }

    /// @beginGetters
    [[nodiscard]] int currentCount() const {
        return m_currentCount;
    }

    [[nodiscard]] ItemId id() const {
        return m_currentId;
    }
    /// @endGetters

private:
    ItemId m_currentId;
    int m_currentCount;
};

} // namespace Kinoko::Item
