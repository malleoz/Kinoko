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
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
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

    /// @addr{0x8071E270}
    /// @brief Creates the singleton instance of @ref JugemDirector
    /// @return A pointer to the newly created singleton instance of @ref JugemDirector
    static JugemDirector *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<JugemDirector>();
        return s_instance;
    }

    /// @addr{0x8071E2FC}
    /// @brief Destroys the singleton instance of @ref JugemDirector
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @addr{0x809C28B8}
    /// @brief Returns the singleton instance of the @ref JugemDirector
    /// @return A pointer to the singleton instance of the @ref JugemDirector
    [[nodiscard]] static JugemDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    JugemDirector();
    ~JugemDirector();

    void createUnits();

    JugemUnit *m_unit; ///< Pointer to the @ref JugemUnit for the player

    static JugemDirector *s_instance; ///< @addr{0x809C28B8}
};

} // namespace Kinoko::Field
