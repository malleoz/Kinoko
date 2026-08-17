#pragma once

#include "game/field/jugem/JugemUnit.hh"

namespace Kinoko::Field {

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
    friend class Host::Context;

public:
    /// @addr{0x8071E638}
    /// @brief Creates the @ref JugemUnit for the player and initializes it
    void init() {
        createUnits();
        m_unit->init();
    }

    /// @addr{8071E6C0}
    /// @brief Updates the @ref JugemUnit for the player
    void calc() {
        m_unit->calc();
    }

    static JugemDirector *CreateInstance();
    [[nodiscard]] static JugemDirector *Instance();
    static void DestroyInstance();

private:
    EGG_NEW_DELETE_FRIEND

    JugemDirector();
    ~JugemDirector();

    void createUnits();

    JugemUnit *m_unit; ///< Pointer to the @ref JugemUnit for the player

    static JugemDirector *s_instance; ///< @addr{0x809C28B8}
};

} // namespace Kinoko::Field
