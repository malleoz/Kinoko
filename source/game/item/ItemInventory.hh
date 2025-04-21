#pragma once

#include "game/item/ItemId.hh"

namespace Item {

/// @nosubgrouping
class ItemInventory {
public:
    ItemInventory();
    ~ItemInventory();

    /// @addr{0x807BC610}
    void FUN_807BC610() {
        clear();
    }

    /// @beginSetters
    void setItem(ItemId id);
    void useItem(int count);
    void clear();
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] ItemId id() const {
        return m_currentId;
    }
    /// @endGetters

private:
    ItemId m_currentId;
    int m_currentCount;
};

} // namespace Item
