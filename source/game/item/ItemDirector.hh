#pragma once

#include <Types.hh>

#include <egg/core/Heap.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

/// @brief Pertains to item handling
namespace Item {

class ItemInventory;
class KartItem;

/// @brief Singleton class that manages item state for all karts in the game
/// @details Maintains an array of @ref KartItem objects, indexed based on player id.
class ItemDirector : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    void init();
    void calc();

    /// @beginGetters

    [[nodiscard]] KartItem &kartItem(size_t idx);
    [[nodiscard]] const ItemInventory &itemInventory(size_t idx) const;

    /// @endGetters

    static ItemDirector *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static ItemDirector *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    ItemDirector();
    ~ItemDirector() override;

    owning_span<KartItem> m_karts; ///< Array of @ref KartItem objects, indexed by player id

    /// @addr{0x809C3618}
    /// @brief Singleton instance of the @ref ItemDirector
    static ItemDirector *s_instance;
};

} // namespace Item

} // namespace Kinoko
