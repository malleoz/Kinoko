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
    void init(u16 wheelIdx, KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx) {
        m_physics = EGG::egg_new<KartSuspensionPhysics>(wheelIdx, tireType, bspWheelIdx);
    }

    /// @addr{0x80598BD4}
    void initPhysics() {
        m_physics->init();
    }

    /// @addr{0x80598BE4}
    void setInitialState() {
        m_physics->setInitialState();
    }

    /// @beginGetters
    [[nodiscard]] KartSuspensionPhysics *suspPhysics() {
        return m_physics;
    }
    /// @endGetters

private:
    KartSuspensionPhysics *m_physics; ///< Pointer to the @ref KartSuspensionPhysics subsystem
};

/// @brief Front suspension for bikes, wrapping around @ref KartSuspensionPhysics
class KartSuspensionFrontBike : public KartSuspension {
public:
    /// @brief Default constructor
    KartSuspensionFrontBike() = default;

    /// @addr{0x805993CC}
    /// @brief Default virtual destructor
    ~KartSuspensionFrontBike() override = default;
};

/// @brief Rear suspension for bikes, wrapping around @ref KartSuspensionPhysics
class KartSuspensionRearBike : public KartSuspension {
public:
    /// @brief Default constructor
    KartSuspensionRearBike() = default;

    /// @addr{0x8059938C}
    /// @brief Default virtual destructor
    ~KartSuspensionRearBike() override = default;
};

} // namespace Kinoko::Kart
