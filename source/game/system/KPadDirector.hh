#pragma once

#include "game/system/KPadController.hh"

#include <egg/core/Heap.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace System {

class KPadPlayer;
class KPadGhostController;
class KPadHostController;

/// @brief The highest level abstraction for controller input processing
class KPadDirector : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    void calc();
    void calcPads();
    void clear();
    void reset();

    void startGhostProxies();
    void endGhostProxies();

    void setGhostPad(const u8 *inputs, bool driftIsAuto);
    void setHostPad(bool driftIsAuto);

    /// @beginGetters

    [[nodiscard]] const KPadPlayer &playerInput() const;
    [[nodiscard]] KPadHostController *hostController();

    /// @endGetters

    static KPadDirector *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static KPadDirector *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    KPadDirector();
    ~KPadDirector() override;

    KPadPlayer m_playerInput;               ///< The player's pad state
    KPadGhostController *m_ghostController; ///< Controller object for ghost input playback
    KPadHostController *m_hostController;   ///< Controller object for external sources

    /// @addr{0x809BD70C}
    ///< @brief Singleton instance of the @ref KPadDirector
    static KPadDirector *s_instance;
};

} // namespace System

} // namespace Kinoko
