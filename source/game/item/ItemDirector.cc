#include "ItemDirector.hh"

#include "Singleton.hh"

#include "game/system/RaceConfig.hh"

namespace Item {

/// @addr{0x80799794}
void ItemDirector::init() {
    for (auto &kart : m_karts) {
        kart.inventory().setItem(ItemId::TRIPLE_MUSHROOM);
    }
}

/// @addr{0x80799850}
void ItemDirector::calc() {
    for (auto &kart : m_karts) {
        kart.calc();
    }
}

/// @addr{0x80799138}
ItemDirector *ItemDirector::CreateInstance() {
    return new ItemDirector;
}

/// @addr{0x80799188}
void ItemDirector::DestroyInstance() {
    delete this;
}

/// @addr{0x807992D8}
ItemDirector::ItemDirector() {
    size_t playerCount = Singleton<System::RaceConfig>::Instance()->raceScenario().playerCount;
    m_karts = std::span<KartItem>(new KartItem[playerCount], playerCount);

    for (size_t i = 0; i < playerCount; ++i) {
        m_karts[i].init(i);
    }
}

/// @addr{0x80798F9C}
ItemDirector::~ItemDirector() {
    delete[] m_karts.data();
}

} // namespace Item
