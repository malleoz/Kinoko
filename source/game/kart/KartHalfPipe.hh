#pragma once

#include "game/kart/KartObjectProxy.hh"

#include "game/system/KPadController.hh"

namespace Kinoko::Kart {

/// @brief Handles the physics and boosts associated with zippers.
/// @details Responsible for managing collision checks against half-pipe zippers, deriving the
/// duration of airtime based off initial direction and speed, calculating the rotation of the kart
/// while performing a trick, and applying variable length boosts after landing from the zipper.
class KartHalfPipe : public KartObjectProxy {
public:
    KartHalfPipe();
    ~KartHalfPipe();

    /// @addr{0x805741B0}
    /// @brief Resets the half-pipe state to its default values
    void reset() {
        m_stunt = StuntType::None;
        m_touchingZipper = false;
        m_landingBoostDelayTimer = 0;
    }

    void calc();
    void calcTrick();
    void calcTrickRot();
    void calcCollision(bool notAirborne);
    void activateTrick(s32 duration, System::Trick trick);
    void end(bool boost);

    /// @addr{0x80574108}
    /// @brief Returns the terminal velocity while mid-air from a half-pipe zipper
    static consteval f32 TerminalVelocity() {
        return 65.0f;
    }

private:
    /// @brief The type of trick being performed
    enum class StuntType {
        None = -1,     ///< No trick is being performed
        Backflip = 0,  ///< The kart is performing a backflip
        Frontflip = 1, ///< The kart is performing a frontflip
        Side360 = 2,   ///< The kart is performing a 360-degree side trick
        Backside = 3,  ///< The kart rotates along the backside of the halfpipe
        Frontside = 4, ///< The kart rotates along the frontside of the halfpipe
        Side720 = 5,   ///< The kart is performing a 720-degree side trick
    };

    /// @brief Angle properties corresponding with the stunts
    struct StuntProperties {
        f32 initRotSpeed;        ///< The initial change in angle per frame
        f32 minRotSpeed;         ///< The minimum change in angle per frame
        f32 initDecayRate;       ///< Initial rate at which the rotational speed decays per frame
        f32 decayRateDelta;      ///< Amount subtracted from the current decay rate every frame
        f32 angleDecayThreshold; ///< The % threshold at which the rotational speed begins to decay
        f32 targetRot;           ///< Total angle to rotate for the stunt (in degrees)
    };

    /// @brief Stores the state of a trick's rotation
    /// @details Exposes an interface to select the properties of a trick and calculate the current
    /// rotation angle based on the properties. Once the angle decay threshold has been reached,
    /// begins decaying rotation towards the target rotation angle.
    struct StuntManager {
        void calcAngle();
        void setProperties(size_t idx);

        f32 angle;                  ///< Accumulated rotation angle from the current trick
        f32 rotSpeed;               ///< Current change in angle per frame
        f32 decayRate;              ///< Current rate at which the rotational speed decays per frame
        StuntProperties properties; ///< Properties of the current trick's rotation
    };

    bool m_touchingZipper;        ///< True if the kart is currently touching a half-pipe zipper
    s16 m_landingBoostDelayTimer; ///< Delay timer for applying a boost after landing from a zipper
    f32 m_nextSign;               ///< The rotation direction sign to use for an upcoming trick
    s32 m_attemptedTrickTimer; ///< When attempting a trick, tracks how long the animation would be.
    EGG::Quatf m_launchRot;    ///< Rotation of the kart on the zipper before gaining airtime
    EGG::Vector3f m_prevPos;   ///< The frame of the kart on the previous frame
    StuntType m_stunt;         ///< The type of trick being performed (or -1 if no trick)
    f32 m_rotSign;             ///< Direction of rotation (1 - Counter-clockwise, -1 - Clockwise)
    s16 m_leniencyTimer;       ///< Frames remaining before the buffered trick input will be dropped
    System::Trick m_trick;     ///< The trick input type
    EGG::Quatf m_stuntRot;     ///< The rotation quaternion for the current state of a trick
    StuntManager m_stuntManager; ///< Manages the current state of a trick's rotation
};

} // namespace Kinoko::Kart
