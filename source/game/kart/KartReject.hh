#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko {

namespace Field {
struct CollisionInfo;
}

namespace Kart {

/// @brief Pertains to handling reject road.
class KartReject : public KartObjectProxy {
public:
    KartReject();
    ~KartReject();

    /// @addr{0x80585AE8}
    void reset() {
        m_rejectSign = 0.0f;
    }

    void calcRejectRoad();
    bool calcRejection();

private:
    bool calcCollision(Field::CollisionInfo &colInfo, Field::KCLTypeMask mask,
            EGG::Vector3f &tangentOff);

    f32 m_rejectSign;
};

} // namespace Kart

} // namespace Kinoko
