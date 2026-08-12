#pragma once

#include "game/item/ItemId.hh"

namespace Kinoko::Item {

/// @brief Represents the item slot for a particular player
/// @details Tracks the current item id in the slot. For items such as triple mushrooms, it also
/// tracks the number of uses remaining.
class ItemInventory {
public:
    ItemInventory();
    ~ItemInventory();

    /// @beginSetters
    void setItem(ItemId id);
    void useItem(int count);
    void clear();
    /// @endSetters

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
