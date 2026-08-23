#pragma once

#include "game/kart/KartSuspensionPhysics.hh"

namespace Kinoko::Kart {

/// @brief A holder for a wheel's physics data.
class KartTire {
public:
    /// @addr{0x8059AA44}
    KartTire(KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx)
        : m_tireType(tireType), m_bspWheelIdx(bspWheelIdx) {}

    /// @addr{0x8058EC08}
    virtual ~KartTire() {
        EGG::egg_delete(m_wheelPhysics);
    }

    /// @addr{0x8059AB14}
    virtual void createPhysics(u16 tireIdx) {
        m_wheelPhysics = EGG::egg_new<WheelPhysics>(tireIdx, 1);
    }

    /// @addr{0x8059AAB0}
    void init(u16 tireIdx) {
        createPhysics(tireIdx);
        m_wheelPhysics->init();
    }

    /// @addr{0x8059AB68}
    void initBsp() {
        m_wheelPhysics->initBsp();
    }

    /// @beginGetters
    [[nodiscard]] WheelPhysics *wheelPhysics() {
        return m_wheelPhysics;
    }
    /// @endGetters

protected:
    KartSuspensionPhysics::TireType m_tireType;
    u16 m_bspWheelIdx;
    WheelPhysics *m_wheelPhysics;
};

/// @brief A holder for a kart's front tire's physics data.
class KartTireFront : public KartTire {
public:
    /// @addr{Inlined in 0x8058EA0C}
    KartTireFront(KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx)
        : KartTire(tireType, bspWheelIdx) {}

    /// @addr{0x8058F4AC}
    ~KartTireFront() override = default;

    /// @addr{0x8059AC1C}
    void createPhysics(u16 tireIdx) override {
        m_wheelPhysics = EGG::egg_new<WheelPhysics>(tireIdx, 0);
    }
};

/// @brief A holder for a bike's front tire's physics data.
class KartTireFrontBike : public KartTire {
public:
    KartTireFrontBike(KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx)
        : KartTire(tireType, bspWheelIdx) {}

    /// @addr{0x8058F4EC}
    ~KartTireFrontBike() override = default;

    /// @addr{0x8059B038}
    void createPhysics(u16 tireIdx) override {
        m_wheelPhysics = EGG::egg_new<WheelPhysics>(tireIdx, 0);
    }
};

/// @brief A holder for a bike's rear tire's physics data.
class KartTireRearBike : public KartTire {
public:
    KartTireRearBike(KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx)
        : KartTire(tireType, bspWheelIdx) {}

    /// @addr{0x8059B564}
    ~KartTireRearBike() override = default;

    /// @addr{0x8059B1FC}
    void createPhysics(u16 tireIdx) override {
        m_wheelPhysics = EGG::egg_new<WheelPhysics>(tireIdx, 1);
    }
};

} // namespace Kinoko::Kart
