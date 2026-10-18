#pragma once

#include <egg/core/Heap.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

class JugemUnit;

/// @brief Manager class for the lifecycle of Lakitu objects for players
/// @desync Lakitu performs a collision check with the floor to make sure that it does not clip into
/// the floor. If it performs a collision check against the DS Delfino Square drawbridge (@ref
/// ObjectTownBridge), then it will result in the bridge's collision transformation matrix
/// being updated. Because of the reasons described in @ref ObjectTownBridge, this
/// collision check can lead to desyncs. Time trial ghosts have a Lakitu if you are watching the
/// replay, but when racing against a ghost, only the player has a Lakitu. Thus, this can result in
/// ghosts desyncing when racing against them. In terms of ghost replays, this doesn't cause
/// desyncs, but must be implemented in Kinoko in order to make sure that the bridge's
/// transformation matrix is accurately updated.
class JugemDirector : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    void init();
    void calc();

    static JugemDirector *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static JugemDirector *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    JugemDirector();
    ~JugemDirector() override;

    void createUnits();

    JugemUnit *m_unit; ///< Pointer to the @ref JugemUnit for the player

    static JugemDirector *s_instance;
};

} // namespace Field

} // namespace Kinoko
