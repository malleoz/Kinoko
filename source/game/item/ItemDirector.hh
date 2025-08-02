#pragma once

#include "game/item/KartItem.hh"

#include <span>

/// @brief Pertains to item handling.
namespace Item {

/// @addr{0x809C3618}
class ItemDirector : EGG::Disposer {
public:
    void init();
    void calc();

    [[nodiscard]] KartItem &kartItem(size_t idx) {
        ASSERT(idx < m_karts.size());
        return m_karts[idx];
    }

    static ItemDirector *CreateInstance();
    void DestroyInstance();

private:
    ItemDirector();
    ~ItemDirector() override;

    std::span<KartItem> m_karts;
};

} // namespace Item
