#include "ItemInventory.hh"

namespace Kinoko::Item {

ItemInventory::ItemInventory() = default;

ItemInventory::~ItemInventory() = default;

/// @addr{0x807BC940}
/// @brief Adds the specified @ref ItemId to the player's inventory
/// @details For the purposes of Kinoko, we simplify logic in this function to just assign a count
/// of 3 for the triple mushroom item.
void ItemInventory::setItem(ItemId id) {
    constexpr int MUSHROOM_COUNT = 3;

    m_currentId = id;
    m_currentCount = MUSHROOM_COUNT;
}

/// @addr{0x807BC97C}
/// @brief Updates the count to reflect an item being used, clearing the id if the count reaches 0
void ItemInventory::useItem(int count) {
    m_currentCount -= count;
    if (m_currentCount > 0) {
        return;
    }

    clear();
}

/// @addr{0x807BC9C0}
/// @brief Clears the item slot and resets the count to 0
/// @details This is called when the player either uses up the item in their slot, fall
/// out-of-bounds, or when they hit an object whose reaction causes the player to lose their item.
void ItemInventory::clear() {
    m_currentId = ItemId::NONE;
    m_currentCount = 0;
}

} // namespace Kinoko::Item
