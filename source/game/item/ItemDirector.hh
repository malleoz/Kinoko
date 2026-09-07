#pragma once

#include "game/item/KartItem.hh"

#include "game/system/RaceConfig.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

/// @brief Pertains to item handling
namespace Item {

/// @brief Singleton class that manages item state for all karts in the game
/// @details Maintains an array of @ref KartItem objects, indexed based on player id.
class ItemDirector : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /// @addr{0x80799794}
    /// @brief Initializes the item state for all players in the race
    /// @details For the purposes of Kinoko, we simplify the logic here and just give everyone a
    /// triple mushroom at the start of the race to match time trial behavior.
    void init() {
        for (auto &kart : m_karts) {
            kart.inventory().setItem(ItemId::TRIPLE_MUSHROOM);
        }
    }

    /// @addr{0x80799850}
    /// @brief Checks for item usage and updates the item state for all players in the race
    void calc() {
        for (auto &kart : m_karts) {
            kart.calc();
        }
    }

    /// @brief Fetches the @ref KartItem object for the given player index
    /// @param idx The index of the player to retrieve the @ref KartItem for
    /// @return A reference to the @ref KartItem object for that player
    [[nodiscard]] KartItem &kartItem(size_t idx) {
        ASSERT(idx < m_karts.size());
        return m_karts[idx];
    }

    /// @brief Fetches the @ref ItemInventory object for the given player index
    /// @param idx The index of the player to retrieve the @ref ItemInventory for
    /// @return A const reference to the @ref ItemInventory object for that player
    [[nodiscard]] const ItemInventory &itemInventory(s16 idx) const {
        return m_karts[idx].inventory();
    }

    /// @addr{0x80799138}
    /// @brief Creates the singleton instance of @ref ItemDirector
    /// @return A pointer to the newly created singleton instance of @ref ItemDirector
    static ItemDirector *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<ItemDirector>();
        return s_instance;
    }

    /// @addr{0x80799188}
    /// @brief Destroys the singleton instance of the @ref ItemDirector
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @addr{0x809C3618}
    /// @brief Returns the singleton instance of the @ref ItemDirector
    /// @return A pointer to the singleton instance of the @ref ItemDirector
    [[nodiscard]] static ItemDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    ItemDirector();
    ~ItemDirector() override;

    owning_span<KartItem> m_karts; ///< Array of @ref KartItem objects, indexed by player id

    static ItemDirector *s_instance; ///< @addr{0x809C3618}
};

} // namespace Item

} // namespace Kinoko
