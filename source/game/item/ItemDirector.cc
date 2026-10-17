#include "ItemDirector.hh"

#include "game/item/KartItem.hh"

#include "game/system/RaceConfig.hh"

namespace Kinoko::Item {

/// @addr{0x80799794}
/// @brief Initializes the item state for all players in the race
/// @details For the purposes of Kinoko, we simplify the logic here and just give everyone a
/// triple mushroom at the start of the race to match time trial behavior.
void ItemDirector::init() {
    for (auto &kart : m_karts) {
        kart.inventory().setItem(ItemId::TRIPLE_MUSHROOM);
    }
}

/// @addr{0x80799850}
/// @brief Checks for item usage and updates the item state for all players in the race
void ItemDirector::calc() {
    for (auto &kart : m_karts) {
        kart.calc();
    }
}

/// @brief Fetches the @ref KartItem object for the given player index
/// @param idx The index of the player to retrieve the @ref KartItem for
/// @return A reference to the @ref KartItem object for that player
/// @pre The index must be within the valid range of player indices.
[[nodiscard]] KartItem &ItemDirector::kartItem(size_t idx) {
    ASSERT(idx < m_karts.size());
    return m_karts[idx];
}

/// @brief Fetches the @ref ItemInventory object for the given player index
/// @param idx The index of the player to retrieve the @ref ItemInventory for
/// @return A const reference to the @ref ItemInventory object for that player
/// @pre The index must be within the valid range of player indices.
[[nodiscard]] const ItemInventory &ItemDirector::itemInventory(size_t idx) const {
    ASSERT(idx < m_karts.size());
    return m_karts[idx].inventory();
}

/// @addr{0x80799138}
/// @brief Creates the singleton instance of @ref ItemDirector
/// @return A pointer to the newly created singleton instance of @ref ItemDirector
/// @pre The singleton instance of @ref ItemDirector must not already exist.
ItemDirector *ItemDirector::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<ItemDirector>();
    return s_instance;
}

/// @addr{0x80799188}
/// @brief Destroys the singleton instance of the @ref ItemDirector
/// @pre The singleton instance of @ref ItemDirector must already exist.
void ItemDirector::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @addr{0x809C3618}
/// @brief Returns the singleton instance of the @ref ItemDirector
/// @return A pointer to the singleton instance of the @ref ItemDirector
ItemDirector *ItemDirector::Instance() {
    return s_instance;
}

/// @addr{0x807992D8}
/// @brief Private constructor that constructs and initializes the array of @ref KartItem
/// objects based on the number of players in the race
ItemDirector::ItemDirector() {
    size_t playerCount = System::RaceConfig::Instance()->raceScenario().playerCount;
    m_karts = owning_span<KartItem>(playerCount);

    for (size_t i = 0; i < playerCount; ++i) {
        m_karts[i].init(i);
    }
}

/// @addr{0x80798F9C}
/// @brief Private virtual destructor
ItemDirector::~ItemDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("ItemDirector instance not explicitly handled!");
    }
}

ItemDirector *ItemDirector::s_instance = nullptr;

} // namespace Kinoko::Item
