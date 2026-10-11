#pragma once

#include "game/kart/KartObjectProxy.hh"
#include "game/kart/KartPhysics.hh"

namespace Kinoko::Kart {

/// @brief Represents the body of a general vehicle
/// @details This class owns the @ref KartPhysics object for this kart. It also tracks the current
/// and target sink depth of the kart, which represents how far the kart can sink into the ground in
/// response to the colliding @ref Field::CollisionInfo::intensity.
class KartBody : protected KartObjectProxy {
public:
    /// @addr{0x8056C394}
    /// @brief Constructor
    /// @param physics Pointer to the kart's physics state manager
    /// @details Saves a pointer to the kart's physics state manager to @ref m_physics. Initializes
    /// @ref m_leanAngle, @ref m_sinkDepth, and @ref m_targetSinkDepth to zero.
    KartBody(KartPhysics *physics) : m_physics(physics) {
        m_leanAngle = 0.0f;
        m_sinkDepth = 0.0f;
        m_targetSinkDepth = 0.0f;
    }

    /// @addr{0x8056CD84}
    /// @brief Default virtual destructor
    /// @note The derived classes are responsible for properly deleting the @ref KartPhysics object
    virtual ~KartBody() = default;

    /// @addr{0x8056C604}
    /// @brief Computes a matrix to represent wheel rotation. For karts, this is wheel-agnostic.
    /// @return The transformation matrix representing the wheel's rotation
    /// @details For karts, this is simply the kart's full rotation and position.
    [[nodiscard]] virtual EGG::Matrix34f wheelMatrix(u16) {
        EGG::Matrix34f mat;
        mat.makeQT(fullRot(), pos());
        return mat;
    }

    /// @addr{0x8056C4B4}
    /// @brief Resets the kart's body to default values
    /// @details Calls @ref KartPhysics::reset() on @ref m_physics. Resets @ref m_leanAngle, @ref
    /// m_sinkDepth, and @ref m_targetSinkDepth to zero.
    void reset() {
        m_physics->reset();
        m_leanAngle = 0.0f;
        m_sinkDepth = 0.0f;
        m_targetSinkDepth = 0.0f;
    }

    /// @addr{0x8056C9C4}
    /// @brief Interpolates the current sink depth towards the target sink depth
    /// @details Smoothly updates @ref m_sinkDepth to approach @ref m_targetSinkDepth using a fixed
    /// `0.1f` interpolation rate.
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
    /// @brief Updates the target sink depth based off of the current collision's intensity
    /// @details Sets @ref m_targetSinkDepth to three times the intensity of the current collision.
    void calcTargetSinkDepth() {
        m_targetSinkDepth = 3.0f * static_cast<f32>(collisionData().intensity);
    }

    /// @beginSetters

    /// @addr{0x8056E424}
    /// @brief Sets the lean angle of the kart
    /// @param val The new lean angle to set
    void setLeanAngle(f32 val) {
        m_leanAngle = val;
    }

    /// @endSetters

    /// @beginGetters

    /// @brief Gets a pointer to the underlying @ref KartPhysics instance
    /// @return A pointer to the underlying @ref KartPhysics instance, @ref m_physics
    [[nodiscard]] KartPhysics *physics() const {
        return m_physics;
    }

    /// @brief Gets the current sink depth of the kart
    /// @return The current sink depth, @ref m_sinkDepth
    [[nodiscard]] f32 sinkDepth() const {
        return m_sinkDepth;
    }

    /// @endGetters

protected:
    KartPhysics *const m_physics; ///< Pointer to the kart's physics state manager
    f32 m_leanAngle;       ///< Lean angle of the kart incurred from drifting, SSMT, or burnout
    f32 m_sinkDepth;       ///< Current smoothed vehicle offset applied downward into collision
    f32 m_targetSinkDepth; ///< Maximum vehicle offset applied downward into collision
};

/// @brief Represents the body of a kart
/// @details In the base game, this is a separate class from @ref KartBody, even though we don't
/// implement any overridden functions in this subclass. To maintain clarity, we define this as a
/// separate class so the @p addr{} Doxygen annotations are accurate.
class KartBodyKart final : public KartBody {
public:
    /// @addr{0x8056CCC0}
    /// @brief Constructor
    KartBodyKart(KartPhysics *physics) : KartBody(physics) {}

    /// @addr{0x8056E494}
    /// @brief Virtual destructor that destroys the underlying @ref KartPhysics subsystem
    ~KartBodyKart() override {
        EGG::egg_delete(m_physics);
    }
};

/// @brief Represents the body of a bike
/// @details Overrides the wheelMatrix() function to factor in handlebar rotation for the front
/// wheel.
class KartBodyBike : public KartBody {
public:
    /// @addr{0x8056D858}
    /// @brief Constructor
    /// @param physics Pointer to the kart's physics state manager
    KartBodyBike(KartPhysics *physics) : KartBody(physics) {}

    /// @addr{0x8056E2BC}
    /// @brief Virtual destructor that destroys the underlying @ref KartPhysics subsystem
    ~KartBodyBike() override {
        EGG::egg_delete(m_physics);
    }

    /// @addr{0x8056DD54}
    /// @brief Computes a matrix to represent the rotation of a wheel.
    /// @param wheelIdx 0 for front wheel, 1 for rear wheel
    /// @return The transformation matrix representing the wheel's rotation
    /// @details For the front wheel, we factor in the handlebar rotation.
    /// For the rear wheel, we only factor in the kart's rotation.
    [[nodiscard]] EGG::Matrix34f wheelMatrix(u16 wheelIdx) override {
        EGG::Matrix34f mat;

        mat.makeQT(fullRot(), pos());
        if (wheelIdx != 0) {
            return mat;
        }

        EGG::Vector3f position = param()->bikeDisp().m_handlePos * scale();
        EGG::Vector3f rotation = DEG2RAD * param()->bikeDisp().m_handleRot;

        EGG::Matrix34f handleMatrix;
        handleMatrix.makeRT(rotation, position);
        EGG::Matrix34f tmp = mat.multiplyTo(handleMatrix);

        EGG::Vector3f yRotation = EGG::Vector3f(0.0f, DEG2RAD * m_leanAngle, 0.0f);
        EGG::Matrix34f yRotMatrix;
        yRotMatrix.makeR(yRotation);
        mat = tmp.multiplyTo(yRotMatrix);

        return mat;
    }
};

/// @brief Represents the body of @ref Vehicle::Quacker
/// @details For the purposes of Kinoko, this effectively behaves the same as @ref KartBody in
/// terms of @ref wheelMatrix(). We keep it separate to maintain clarity and accuracy of the @p
/// addr{} Doxygen annotations.
class KartBodyQuacker : public KartBodyBike {
public:
    /// @addr{0x8056E218}
    /// @brief Constructor
    /// @param physics Pointer to the kart's physics state manager
    KartBodyQuacker(KartPhysics *physics) : KartBodyBike(physics) {}

    /// @addr{0x8056E42C}
    /// @brief Default virtual destructor
    ~KartBodyQuacker() override = default;

    /// @addr{0x8056E2FC}
    /// @brief Computes a matrix to represent wheel rotation. For Quacker, this is wheel-agnostic.
    /// @return The transformation matrix representing the wheel's rotation
    [[nodiscard]] EGG::Matrix34f wheelMatrix(u16 /* wheelIdx */) override {
        EGG::Matrix34f mat;
        mat.makeQT(fullRot(), pos());
        return mat;
    }
};

} // namespace Kinoko::Kart
