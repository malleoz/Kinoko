#include "KartSuspension.hh"

namespace Kart {

/// @addr{0x80598B08}
KartSuspension::KartSuspension() = default;

KartSuspension::~KartSuspension() {
    delete m_physics;
}

/// @addr{0x80598B60}
void KartSuspension::init(u16 wheelIdx, u16 bspWheelIdx) {
    m_physics = new KartSuspensionPhysics(wheelIdx, bspWheelIdx);
}

/// @addr{0x80598BD4}
void KartSuspension::initPhysics() {
    m_physics->init();
}

/// @addr{0x80598BE4}
void KartSuspension::setInitialState() {
    m_physics->setInitialState();
}

KartSuspensionPhysics *KartSuspension::suspPhysics() {
    return m_physics;
}

KartSuspensionFrontBike::KartSuspensionFrontBike() = default;

KartSuspensionFrontBike::~KartSuspensionFrontBike() = default;

KartSuspensionRearBike::KartSuspensionRearBike() = default;

KartSuspensionRearBike::~KartSuspensionRearBike() = default;

} // namespace Kart
