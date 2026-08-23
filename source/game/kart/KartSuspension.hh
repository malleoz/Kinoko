#pragma once

#include "game/kart/KartSuspensionPhysics.hh"

namespace Kinoko::Kart {

/// @brief Doesn't do much besides hold a pointer to KartSuspensionPhysics.
class KartSuspension : protected KartObjectProxy {
public:
    /// @addr{0x80598B08}
    KartSuspension() = default;

    /// @addr{0x8058F52C}
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
    KartSuspensionPhysics *m_physics;
};

class KartSuspensionFrontBike : public KartSuspension {
public:
    KartSuspensionFrontBike() = default;

    /// @addr{0x805993CC}
    ~KartSuspensionFrontBike() override = default;
};

class KartSuspensionRearBike : public KartSuspension {
public:
    KartSuspensionRearBike() = default;

    /// @addr{0x8059938C}
    ~KartSuspensionRearBike() override = default;
};

} // namespace Kinoko::Kart
