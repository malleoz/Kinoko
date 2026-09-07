#include "KPadDirector.hh"

namespace Kinoko::System {

/// @addr{0x805232F0}
/// @brief Private constructor that initializes the ghost and host controllers
KPadDirector::KPadDirector() {
    m_ghostController = EGG::egg_new<KPadGhostController>();
    m_hostController = EGG::egg_new<KPadHostController>();
}

/// @addr{0x805231DC}
/// @brief Private destructor
KPadDirector::~KPadDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KPadDirector instance not explicitly handled!");
    }
}

KPadDirector *KPadDirector::s_instance = nullptr;

} // namespace Kinoko::System
