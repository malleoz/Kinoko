#pragma once

#include "game/kart/KartSuspensionPhysics.hh"

namespace Kinoko::Kart {

/// @brief For Kinoko, this is just a wrapper around @ref KartSuspensionPhysics
class KartSuspension : private KartObjectProxy {
public:
    /// @addr{0x80598B08}
    /// @brief Default constructor
    KartSuspension() = default;

    /// @addr{0x8058F52C}
    /// @brief Virtual destructor that destroys the underlying @ref KartSuspensionPhysics subsystem
    virtual ~KartSuspension() {
        EGG::egg_delete(m_physics);
    }

    /// @addr{0x80598B60}
    /// @brief Creates the @ref KartSuspensionPhysics subsystem with the provided arguments
    /// @param wheelIdx The index of the wheel this suspension is associated with
    /// @param tireType The type of tire this suspension is associated with
    /// @param bspWheelIdx The BSP wheel index for this suspension
    void init(u16 wheelIdx, KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx) {
        m_physics = EGG::egg_new<KartSuspensionPhysics>(wheelIdx, tireType, bspWheelIdx);
    }

    /// @addr{0x80598BD4}
    /// @brief Initializes the suspension physics subsystem
    /// @details Simply calls @ref KartSuspensionPhysics::init().
    void initPhysics() {
        m_physics->init();
    }

    /// @addr{0x80598BE4}
    /// @brief Computes the initial state of the @ref m_physics subsystem
    /// @details Simply calls @ref KartSuspensionPhysics::setInitialState().
    void setInitialState() {
        m_physics->setInitialState();
    }

    /// @beginGetters

    /// @brief Gets the underlying @ref KartSuspensionPhysics subsystem
    /// @return Pointer to the underlying @ref KartSuspensionPhysics subsystem, @ref m_physics
    [[nodiscard]] KartSuspensionPhysics *suspPhysics() {
        return m_physics;
    }

    /// @endGetters

private:
    KartSuspensionPhysics *m_physics; ///< Pointer to the @ref KartSuspensionPhysics subsystem
};

/// @brief Front suspension for bikes, wrapping around @ref KartSuspensionPhysics
class KartSuspensionFrontBike final : public KartSuspension {
public:
    /// @brief Default constructor
    KartSuspensionFrontBike() = default;

    /// @addr{0x805993CC}
    /// @brief Default virtual destructor
    ~KartSuspensionFrontBike() override = default;
};

/// @brief Rear suspension for bikes, wrapping around @ref KartSuspensionPhysics
class KartSuspensionRearBike final : public KartSuspension {
public:
    /// @brief Default constructor
    KartSuspensionRearBike() = default;

    /// @addr{0x8059938C}
    /// @brief Default virtual destructor
    ~KartSuspensionRearBike() override = default;
};

} // namespace Kinoko::Kart
