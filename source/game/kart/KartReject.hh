#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko {

namespace Field {

struct CollisionInfo;

} // namespace Field

namespace Kart {

/// @brief Pertains to handling kart collisions against reject road
/// @details Reject road refers to drivable KCL surfaces that have the @ref
/// Field::CollisionDirector::eCollisionAttribute::RejectRoad bit set. When driving on reject road,
/// found on the edge of a non-trickable halfpipe, it will try to push you away back towards the
/// surface you came from. Because it performs a direct position update rather than via external
/// velocity, this can result in the kart moving faster than the normal 120 units per frame speed
/// limit.
class KartReject : private KartObjectProxy {
public:
    KartReject();
    ~KartReject() override;

    void reset();
    void calc();
    bool calcRejection();

private:
    bool calcCollision(Field::CollisionInfo &colInfo, Field::KCLTypeMask mask,
            EGG::Vector3f &tangentOff);

    f32 m_rejectSign; ///< Direction of the current rejection
};

} // namespace Kart

} // namespace Kinoko
