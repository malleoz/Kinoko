#pragma once

#include "game/system/KPadController.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace System {

/// @brief The highest level abstraction for controller processing.
/// @addr{0x809BD70C}
class KPadDirector : EGG::Disposer {
    friend class Host::Context;

public:
    void calc();
    void calcPads();

    /// @addr{0x80523724}
    void clear() {}

    void reset();
    void startGhostProxies();
    void endGhostProxies();

    [[nodiscard]] const KPadPlayer &playerInput() const {
        return m_playerInput;
    }

    [[nodiscard]] KPadHostController *hostController() {
        return m_hostController;
    }

    void setGhostPad(const u8 *inputs, bool driftIsAuto);
    void setHostPad(bool driftIsAuto);

    /// @addr{0x8052313C}
    static KPadDirector *CreateInstance() {
        ASSERT(!s_instance);
        return s_instance = EGG::egg_new<KPadDirector>();
    }

    /// @addr{0x8052318C}
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    [[nodiscard]] static KPadDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    KPadDirector();
    ~KPadDirector() override;

    KPadPlayer m_playerInput;
    KPadGhostController *m_ghostController;
    KPadHostController *m_hostController;

    static KPadDirector *s_instance; ///< @addr{0x809BD70C}
};

} // namespace System

} // namespace Kinoko
