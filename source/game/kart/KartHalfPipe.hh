#pragma once

#include "game/kart/KartObjectProxy.hh"

#include "game/system/KPadController.hh"

namespace Kart {

/// @brief Handles the physics and boosts associated with zippers.
/// @nosubgrouping
class KartHalfPipe : public KartObjectProxy {
public:
    KartHalfPipe();
    ~KartHalfPipe();

    void reset();
    void calc();
    void calcTrick();
    void calcRot();
    void calcLanding(bool);
    void activateTrick(s32 param_2, System::Trick trick);
    void end(bool boost);

    /// @addr{0x80574108}
    static consteval f32 TerminalVelocity() {
        return 65.0f;
    }

private:
    enum class TrickType {
        None = -1,
        Backflip = 0,
        Frontflip = 1,
        Side360 = 2,
        Backside = 3,
        Frontside = 4,
        Side720 = 5,
    };

    struct TrickProperties {
        f32 angleDelta;
        f32 angleDeltaMin;
        f32 angleDeltaFactorMin;
        f32 angleDeltaFactorDecr;
        f32 finalAngleScalar;
        f32 finalAngle;
    };

    struct TrickPropertiesHolder {
        f32 angle;
        f32 angleDelta;
        f32 angleDeltaFactor;
        f32 angleDeltaFactorDecr;
        f32 finalAngle;
        TrickProperties properties;
    };

    bool m_touchingZipper;
    s16 m_timer;
    f32 m_nextSign;
    s32 m_attemptedTrickTimer; ///< When attempting a trick, tracks how long the animation would be.
    EGG::Quatf m_rot;
    EGG::Vector3f m_prevPos;
    TrickType m_type;
    f32 m_rotSign;
    s16 m_nextTimer;
    System::Trick m_trick;
    EGG::Quatf m_stuntRot;
    TrickPropertiesHolder m_sub;
};

} // namespace Kart
