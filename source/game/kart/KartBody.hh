#pragma once

#include "game/kart/KartObjectProxy.hh"
#include "game/kart/KartPhysics.hh"

namespace Kinoko::Kart {

/// @brief Represents the body of a general vehicle
/// @details This class owns the @ref KartPhysics object for this kart. It also tracks the current
/// and target sink depth of the kart, which represents how far the kart can sink into the ground in
/// response to the colliding @ref CollisionInfo::intensity.
class KartBody : protected KartObjectProxy {
public:
    KartBody(KartPhysics *physics);
    virtual ~KartBody() = default;

    /// @addr{0x8056C604}
    /// @brief Computes a matrix to represent wheel rotation. For karts, this is wheel-agnostic.
    [[nodiscard]] virtual EGG::Matrix34f wheelMatrix(u16) {
        EGG::Matrix34f mat;
        mat.makeQT(fullRot(), pos());
        return mat;
    }

    /// @addr{0x8056C4B4}
    /// @brief Resets the kart's body to default values
    void reset() {
        m_physics->reset();
        m_anAngle = 0.0f;
        m_sinkDepth = 0.0f;
        m_targetSinkDepth = 0.0f;
    }

    /// @addr{0x8056C9C4}
    /// @brief Interpolates the current sink depth towards the target sink depth
    void calcSinkDepth() {
        constexpr f32 SINK_INTERP_RATE = 0.1f;

        m_sinkDepth += (m_targetSinkDepth - m_sinkDepth) * SINK_INTERP_RATE;
    }

    /// @addr{0x8056C950}
    /// @brief Sets the target sink depth to the provided value if it's greater than the current
    /// target sink depth
    /// @param val The new target sink depth to try to set
    void trySetTargetSinkDepth(f32 val) {
        m_targetSinkDepth = std::max(val, m_targetSinkDepth);
    }

    /// @addr{0x8056C964}
    /// @brief Sets the target sink depth based off of the current collision's intensity
    void calcTargetSinkDepth() {
        m_targetSinkDepth = 3.0f * static_cast<f32>(collisionData().intensity);
    }

    /// @beginSetters
    /// @addr{0x8056E424}
    void setAngle(f32 val) {
        m_anAngle = val;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] KartPhysics *physics() const {
        return m_physics;
    }

    [[nodiscard]] f32 sinkDepth() const {
        return m_sinkDepth;
    }
    /// @endGetters

protected:
    KartPhysics *m_physics; ///< Pointer to the kart's physics state manager
    f32 m_anAngle;          ///< @rename Possible pertains to handlebar/front wheel rotation
    f32 m_sinkDepth;        ///< Current smoothed vehicle offset applied downward into collision
    f32 m_targetSinkDepth;  ///< Maximum vehicle offset applied downward into collision
};

/// @brief Represents the body of a kart
/// @details In the base game, this is a separate class from @ref KartBody, even though we don't
/// implement any overridden functions in this subclass. To maintain clarity, we define this as a
/// separate class so the @p addr{} Doxygen annotations are accurate.
class KartBodyKart : public KartBody {
public:
    KartBodyKart(KartPhysics *physics);
    ~KartBodyKart() override;
};

/// @brief Represents the body of a bike
/// @details Overrides the wheelMatrix() function to factor in handlebar rotation for the front
/// wheel.
class KartBodyBike : public KartBody {
public:
    KartBodyBike(KartPhysics *physics);
    ~KartBodyBike() override;

    [[nodiscard]] EGG::Matrix34f wheelMatrix(u16 wheelIdx) override;
};

/// @brief Represents the body of @enum Vehicle::Quacker
/// @details For the purposes of Kinoko, this effectively behaves the same as @ref KartObjectKart in
/// terms of @ref wheelMatrix(). We keep it separate to maintain clarity and accuracy of the @p
/// addr{} Doxygen annotations.
class KartBodyQuacker : public KartBodyBike {
public:
    KartBodyQuacker(KartPhysics *physics) : KartBodyBike(physics) {}

    ~KartBodyQuacker() override = default;

    [[nodiscard]] EGG::Matrix34f wheelMatrix(u16 /* wheelIdx */) override {
        EGG::Matrix34f mat;
        mat.makeQT(fullRot(), pos());
        return mat;
    }
};

} // namespace Kinoko::Kart
