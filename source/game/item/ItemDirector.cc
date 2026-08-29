#include "ItemDirector.hh"

namespace Kinoko::Item {

/// @addr{0x807992D8}
/// @brief Private constructor that constructs and initializes the array of @ref KartItem objects
/// based on the number of players in the race
ItemDirector::ItemDirector() {
    size_t playerCount = System::RaceConfig::Instance()->raceScenario().playerCount;
    m_karts = owning_span<KartItem>(playerCount);

    for (size_t i = 0; i < playerCount; ++i) {
        m_karts[i].init(i);
    }
}

/// @addr{0x80798F9C}
/// @brief Private destructor
ItemDirector::~ItemDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("ItemDirector instance not explicitly handled!");
    }
}

ItemDirector *ItemDirector::s_instance = nullptr; ///< @addr{0x809C3618}

} // namespace Kinoko::Item
