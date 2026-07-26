#pragma once

#include "game/field/obj/ObjectCollidable.hh"

#include <array>

namespace Kinoko::Field {

class ObjectDossunTsuibi;

/// @brief The manager class for the pair of Thwomps in the long hallway on rBC.
/// @details Shares the same rail interpolator between the two Thwomps and coordinates their
/// movement and stomping so they are synchronized.
class ObjectDossunTsuibiHolder final : public ObjectCollidable {
public:
    ObjectDossunTsuibiHolder(const System::MapdataGeoObj &params);
    ~ObjectDossunTsuibiHolder() override;

    void init() override;
    void calc() override;

    /// @addr{0x80764A1C}
    void calcModel() override {}

    /// @addr{0x80764A30}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x80764A2C}
    void loadGraphics() override {}

    /// @addr{0x80764A24}
    void createCollision() override {}

    /// @brief Runs once when the Thwomps enter the Still state at their home position
    void startStill() {
        m_state = State::Backward;
        m_railInterpolator->setCurrVel(m_vel);
    }

private:
    /// @addr{0x807623AC}
    /// @brief Runs every frame while the Thwomps remain still at their home position
    void calcStill() {
        if (--m_stillTimer == 0) {
            m_state = State::Forward;
            m_movingSideways = false;
        }
    }

    void enterStartStomp();

    void calcForward();
    void calcStartStomp();
    void calcStomp();
    void calcBackwards();

    /// @addr{0x807630D0}
    /// @brief Nudges the Thwomps' Z-pos back to their home before moving forward
    void calcResetZ() {
        updatePos(EGG::Vector3f(pos().x, pos().y, pos().z - m_resetZVel));
    }

    void calcRot();
    void calcForwardRail();
    void calcForwardOscillation();

    void updatePos(const EGG::Vector3f &pos);
    void updateRot(f32 yaw);

    /// @brief Describes the current movement of the Thwomps
    enum class State {
        Still = 0,         ///< At their home position, not moving
        Forward = 1,       ///< Moving forward down the hallway
        StartStomp = 2,    ///< One frame in which the holder activates the Thwomp stomp
        Stomping = 3,      ///< Thwomps are stomping downwards
        Backward = 4,      ///< Moving backwards up the hallway towards home
        StillRotating = 5, ///< Returned back to home, but rotating to face forward
    };

    std::array<ObjectDossunTsuibi *, 2> m_dossuns; ///< Pointers to the two Thwomps
    State m_state;                                 ///< Current movement of the Thwomps
    EGG::Vector3f m_initPos;                       ///< "Home" position of the Thwomps
    u32 m_stillTimer;       ///< Frames remaining before entering the Forward state
    bool m_movingForward;   ///< Set once the Thwomps start moving forward
    u32 m_forwardTimer;     ///< Frames elapsed since entering the Forward state
    bool m_movingSideways;  ///< Whether the Thwomps should move along the z-axis
    u32 m_flipSideways;     ///< Inverts which direction the thwomps move along the z-axis
    u32 m_sidewaysPhase;    ///< Accumulating angle for sinusoidal sideways oscillation
    bool m_facingBackwards; ///< Set when the Thwomps are facing backwards
    f32 m_lastStompZ;       ///< Z-position of the last stomp
    f32 m_resetZVel;   ///< Speed required to move from m_lastStompZ to the home's Z in 36 frames
    f32 m_vel;         ///< Rail velocity
    f32 m_initYaw;     ///< Initial rotation about the Y-axis
    f32 m_resetAngVel; ///< Rotational speed of Thwomps after returning home
    u32 m_backwardsCounter; ///< Ticks up every frame the Thwomps are facing backwards, or are
                            ///< resetting after returning home.

    // These members are not in the base game but we add them to prevent dereferencing m_mapObj.
    const f32 m_forwardVel;    ///< Rail velocity
    const u32 m_stillDuration; ///< How long Thwomps remain at their home for

    /// @brief How long Thwomps take to reset after returning to home
    static constexpr u32 HOME_RESET_FRAMES = 36;

    /// @brief Offset applied to both Thwomps' Z-axis positions in opposite directions
    static constexpr f32 DOSSUN_POS_OFFSET = 500.0f;
};

} // namespace Kinoko::Field
