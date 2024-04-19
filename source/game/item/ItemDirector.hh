#pragma once

#include "game/item/KartItem.hh"

#include <span>

/// @brief Pertains to item handling.
namespace Item {

class ItemDirector {
public:
    void init();
    void calc();

    static ItemDirector *CreateInstance();
    static void DestroyInstance();
    static ItemDirector *Instance();

private:
    ItemDirector();
    ~ItemDirector();

    std::span<KartItem> m_karts;

    static ItemDirector *s_instance;
};

} // namespace Item
