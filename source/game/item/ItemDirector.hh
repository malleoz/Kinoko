#pragma once

#include "game/item/KartItem.hh"

#include <span>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

/// @brief Pertains to item handling
namespace Item {

/// @brief Singleton class that manages item state for all karts in the game
/// @details Maintains an array of @ref KartItem objects, indexed based on player id.
class ItemDirector : EGG::Disposer {
    friend class Host::Context;

public:
    void init();
    void calc();

    /// @brief Fetches the @ref KartItem object for the given player index
    /// @param idx The index of the player to retrieve the @ref KartItem for
    /// @return A reference to the @ref KartItem object for that player
    [[nodiscard]] KartItem &kartItem(size_t idx) {
        ASSERT(idx < m_karts.size());
        return m_karts[idx];
    }

    static ItemDirector *CreateInstance();
    static void DestroyInstance();

    /// @brief Fetches the @ref ItemInventory object for the given player index
    /// @param idx The index of the player to retrieve the @ref ItemInventory for
    /// @return A const reference to the @ref ItemInventory object for that player
    [[nodiscard]] const ItemInventory &itemInventory(s16 idx) const {
        return m_karts[idx].inventory();
    }

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
