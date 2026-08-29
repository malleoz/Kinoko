#include "KPadDirector.hh"

namespace Kinoko::System {

/// @addr{0x805232F0}
KPadDirector::KPadDirector() {
    m_ghostController = EGG::egg_new<KPadGhostController>();
    m_hostController = EGG::egg_new<KPadHostController>();
}

/// @addr{0x805231DC}
KPadDirector::~KPadDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KPadDirector instance not explicitly handled!");
    }
}

KPadDirector *KPadDirector::s_instance = nullptr; ///< @addr{0x809BD70C}

} // namespace Kinoko::System
