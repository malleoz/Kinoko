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
    /// @addr{0x805238F0}
    void calc() {
        calcPads();
        m_playerInput.calc();
    }

    /// @addr{0x805237E8}
    void calcPads() {
        m_ghostController->calc();
        m_hostController->calc();
    }

    /// @addr{0x80523724}
    void clear() {}

    /// @addr{0x80523690}
    void reset() {
        m_playerInput.reset();
    }

    /// @addr{0x80524580}
    void startGhostProxies() {
        m_playerInput.startGhostProxy();
    }

    /// @addr{0x805245DC}
    void endGhostProxies() {
        m_playerInput.endGhostProxy();
    }

    /// @addr{0x8052453C}
    void setGhostPad(const u8 *inputs, bool driftIsAuto) {
        m_playerInput.setGhostController(m_ghostController, inputs, driftIsAuto);
    }

    void setHostPad(bool driftIsAuto) {
        m_playerInput.setHostController(m_hostController, driftIsAuto);
    }

    [[nodiscard]] const KPadPlayer &playerInput() const {
        return m_playerInput;
    }

    [[nodiscard]] KPadHostController *hostController() {
        return m_hostController;
    }

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
