#include "KartTire.hh"

namespace Kart {

/// @addr{0x8059AA44}
KartTire::KartTire(MirroredTire mirroredTire, u16 bspWheelIdx)
    : m_mirroredTire(mirroredTire), m_bspWheelIdx(bspWheelIdx) {}

/// @addr{0x8058EC08}
KartTire::~KartTire() {
    delete m_wheelPhysics;
}

/// @addr{0x8059AB14}
void KartTire::createPhysics(u16 tireIdx) {
    m_wheelPhysics = new WheelPhysics(tireIdx, 1);
}

/// @addr{0x8059AAB0}
void KartTire::init(u16 tireIdx) {
    createPhysics(tireIdx);
    m_wheelPhysics->init();
}

/// @addr{0x8059AB68}
void KartTire::initBsp() {
    m_wheelPhysics->initBsp();
}

WheelPhysics *KartTire::wheelPhysics() {
    return m_wheelPhysics;
}

/// @addr{Inlined in 0x8058EA0C}
KartTireFront::KartTireFront(MirroredTire mirroredTire, u16 bspWheelIdx)
    : KartTire(mirroredTire, bspWheelIdx) {}

/// @addr{0x8058F4AC}
KartTireFront::~KartTireFront() = default;

/// @addr{0x8059AC1C}
void KartTireFront::createPhysics(u16 tireIdx) {
    m_wheelPhysics = new WheelPhysics(tireIdx, 0);
}

KartTireFrontBike::KartTireFrontBike(MirroredTire mirroredTire, u16 bspWheelIdx)
    : KartTire(mirroredTire, bspWheelIdx) {}

/// @addr{0x8058F4EC}
KartTireFrontBike::~KartTireFrontBike() = default;

/// @addr{0x8059B038}
void KartTireFrontBike::createPhysics(u16 tireIdx) {
    m_wheelPhysics = new WheelPhysics(tireIdx, 0);
}

KartTireRearBike::KartTireRearBike(MirroredTire mirroredTire, u16 bspWheelIdx)
    : KartTire(mirroredTire, bspWheelIdx) {}

/// @addr{0x8059B564}
KartTireRearBike::~KartTireRearBike() = default;

/// @addr{0x8059B1FC}
void KartTireRearBike::createPhysics(u16 tireIdx) {
    m_wheelPhysics = new WheelPhysics(tireIdx, 1);
}

} // namespace Kart
