#pragma once

#include "game/kart/KartObjectProxy.hh"

#include <egg/math/Quat.hh>

namespace Kinoko {

namespace System {

enum class Trick;

} // namespace System

namespace Kart {

/// @brief Determined by the KCL, represents the variation of the trick that is performed.
/// @details This is also directly used to determine the duration of the trick boost.
enum class TrickVariant {
    StuntTrick = 0,      ///< The kart performs a a small stunt (e.g. Luigi Circuit ramp)
    SingleFlipTrick = 1, ///< The kart performs one backflip (e.g. Bowser's Castle starting ramp)
    DoubleFlipTrick = 2, ///< The kart performs two backflips (e.g. Rainbow Road wavy road ramp)
};

/// @brief Represents the type of trick that are performed based on kart and player input.
/// @details Some of these types are only available for bikes.
enum class TrickType {
    StuntTrickBasic = 0,    ///< A small up/down trick with kart/bike.
    BikeFlipTrickNose = 1,  ///< An up trick with bike.
    BikeFlipTrickTail = 2,  ///< A down trick with bike.
    FlipTrickYLeft = 3,     ///< A left trick with kart/bike.
    FlipTrickYRight = 4,    ///< A right trick with kart/bike.
    KartFlipTrickZ = 5,     ///< An up/down trick with a kart.
    BikeSideStuntTrick = 6, ///< A small side trick with a bike.
};

/// @brief Manages trick inputs and state
/// @details Checks if the user is attempting to perform a trick, calculates rotation during the
/// trick, and applies the trick boost when the trick is completed. This class is NOT used when
/// tricking on zippers (see @ref KartHalfPipe).
class KartJump : protected KartObjectProxy {
public:
    /// @brief Describes the properties of a type of trick being performed
    struct TrickProperties {
        f32 initRotSpeed;       ///< The initial change in angle per frame
        f32 minRotSpeed;        ///< The minimum change in angle per frame
        f32 minDecayRate;       ///< The minimum decay rate
        f32 initDecayRateDelta; ///< Initial amount subtracted from the decay rate every frame
    };

    /// @brief Describes angle-related properties for the trick being performed
    struct AngleProperties {
        f32 maxAngle; ///< Maximum angle above horizontal for the kart's velocity direction
        f32 rotAngle; ///< Pitch correction applied at the start of the trick towards the max angle
    };

    KartJump(KartMove *move);
    ~KartJump() override;

    void reset();
    void tryStart(const EGG::Vector3f &left);
    void calc();
    void calcInput();
    void end();
    void setAngle(const EGG::Vector3f &left);

    /// @beginGetters

    [[nodiscard]] bool isBoostRampEnabled() const;
    [[nodiscard]] TrickType type() const;
    [[nodiscard]] TrickVariant variant() const;
    [[nodiscard]] s16 trickDelay() const;

    /// @endGetters

protected:
    virtual void calcRot();
    virtual void start(const EGG::Vector3f &left);
    virtual void init();

    void setupProperties();

    TrickType m_type;             ///< The type of trick being performed
    TrickVariant m_variant;       ///< The KCL-based variant of the trick being performed
    System::Trick m_nextTrick;    ///< The trick controller input received from the player
    f32 m_rotSign;                ///< Direction of rotation (1 - Counter-clockwise, -1 - Clockwise)
    TrickProperties m_properties; ///< The properties of the trick being performed
    f32 m_angle;                  ///< Accumulated angle of rotation during the trick (in degrees)
    f32 m_rotSpeed;               ///< The current change in angle per frame (in degrees)
    f32 m_decayRate;              ///< Decay factor for the kart's rotation speed during the trick
    f32 m_decayRateDelta;         ///< Amount subtracted from the decay rate every frame
    f32 m_targetAngle;            ///< Target total rotation for the trick (in degrees)
    s16 m_trickDelay;             ///< Frames until a boost is allowed to start after a trick
    EGG::Quatf m_rot;             ///< The current rotation of the kart due to the trick
    KartMove *const m_move;       ///< Pointer to the @ref KartMove subsystem

private:
    [[nodiscard]] bool shouldIgnoreTrick();

    s16 m_leniencyTimer;   ///< Frames remaining before the buffered trick input will be dropped
    bool m_boostRampTrick; ///< True if in the middle of a trick from a boost ramp
};

/// @brief Specialization of @ref KartJump that calculates rotation differently for
/// bike-specific trick behavior
/// @copydetails KartJump
class KartJumpBike final : public KartJump {
public:
    KartJumpBike(KartMove *move);
    ~KartJumpBike() override;

private:
    void calcRot() override;
    void start(const EGG::Vector3f &left) override;
    void init() override;
};

} // namespace Kart

} // namespace Kinoko
