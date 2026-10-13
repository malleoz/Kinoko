#pragma once

#include "game/kart/KartSuspensionPhysics.hh"

namespace Kinoko::Kart {

/// @brief For Kinoko, this is just a wrapper around @ref WheelPhysics
class KartTire {
public:
    /// @addr{0x8059AA44}
    /// @brief Constructor
    /// @param tireType The type of tire
    /// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
    /// @details Sets @ref m_tireType to the provided `tireType` and @ref m_bspWheelIdx to the
    /// provided `bspWheelIdx`.
    KartTire(KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx)
        : m_tireType(tireType),
          m_bspWheelIdx(bspWheelIdx) {}

    /// @addr{0x8058EC08}
    /// @brief Virtual destructor which destroys the underlying @ref WheelPhysics subsystem
    virtual ~KartTire() {
        EGG::egg_delete(m_wheelPhysics);
    }

    /// @addr{0x8059AB14}
    /// @brief Creates the underlying @ref WheelPhysics subsystem
    /// @param tireIdx The index of the tire to associate with the @ref WheelPhysics subsystem
    virtual void createPhysics(u16 tireIdx) {
        m_wheelPhysics = EGG::egg_new<WheelPhysics>(tireIdx, 1);
    }

    /// @addr{0x8059AAB0}
    /// @brief Creates and initializes the underlying @ref WheelPhysics subsystem
    /// @param tireIdx The index of the tire to associate with the @ref WheelPhysics subsystem
    /// @details Calls @ref createPhysics() to create the subsystem and then initializes it via @ref
    /// WheelPhysics::init().
    void init(u16 tireIdx) {
        createPhysics(tireIdx);
        m_wheelPhysics->init();
    }

    /// @addr{0x8059AB68}
    /// @brief Initializes the @ref BSP::Wheel pointer for the @ref WheelPhysics subsystem
    /// @details Simply calls @ref WheelPhysics::initBsp().
    void initBsp() {
        m_wheelPhysics->initBsp();
    }

    /// @beginGetters

    /// @brief Gets the underlying @ref WheelPhysics subsystem
    /// @return Pointer to the underlying @ref WheelPhysics subsystem, @ref m_wheelPhysics
    [[nodiscard]] WheelPhysics *wheelPhysics() {
        return m_wheelPhysics;
    }

    /// @endGetters

protected:
    const KartSuspensionPhysics::TireType m_tireType; ///< The type of tire
    const u16 m_bspWheelIdx;      ///< The index of the wheel in the @ref BSP::wheels array
    WheelPhysics *m_wheelPhysics; ///< Pointer to the underlying @ref WheelPhysics subsystem
};

/// @brief A holder for a kart's front tire's physics data
class KartTireFront final : public KartTire {
public:
    /// @addr{Inlined in 0x8058EA0C}
    /// @brief Constructor
    /// @param tireType The type of tire
    /// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
    KartTireFront(KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx)
        : KartTire(tireType, bspWheelIdx) {}

    /// @addr{0x8058F4AC}
    /// @brief Default virtual destructor
    ~KartTireFront() override = default;

    /// @addr{0x8059AC1C}
    /// @copydetails @ref KartTire::createPhysics()
    void createPhysics(u16 tireIdx) override {
        m_wheelPhysics = EGG::egg_new<WheelPhysics>(tireIdx, 0);
    }
};

/// @brief A holder for a bike's front tire's physics data
class KartTireFrontBike final : public KartTire {
public:
    /// @brief Constructor
    /// @param tireType The type of tire
    /// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
    KartTireFrontBike(KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx)
        : KartTire(tireType, bspWheelIdx) {}

    /// @addr{0x8058F4EC}
    /// @brief Default virtual destructor
    ~KartTireFrontBike() override = default;

    /// @addr{0x8059B038}
    /// @copydetails @ref KartTire::createPhysics()
    void createPhysics(u16 tireIdx) override {
        m_wheelPhysics = EGG::egg_new<WheelPhysics>(tireIdx, 0);
    }
};

/// @brief A holder for a bike's rear tire's physics data
class KartTireRearBike final : public KartTire {
public:
    /// @brief Constructor
    /// @param tireType The type of tire
    /// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
    KartTireRearBike(KartSuspensionPhysics::TireType tireType, u16 bspWheelIdx)
        : KartTire(tireType, bspWheelIdx) {}

    /// @addr{0x8059B564}
    /// @brief Default virtual destructor
    ~KartTireRearBike() override = default;

    /// @addr{0x8059B1FC}
    /// @copydetails @ref KartTire::createPhysics()
    void createPhysics(u16 tireIdx) override {
        m_wheelPhysics = EGG::egg_new<WheelPhysics>(tireIdx, 1);
    }
};

} // namespace Kinoko::Kart
