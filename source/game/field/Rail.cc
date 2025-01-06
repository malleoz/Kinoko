#include "Rail.hh"

#include "game/field/CollisionDirector.hh"

namespace Field {

/// @addr{0x806EC9A4}
Rail::Rail(u16 idx, const System::MapdataPointInfo &pointInfo)
    : m_idx(idx), m_pointCount1(pointInfo.pointCount()), m_pointCount2(pointInfo.pointCount()),
      m_points(pointInfo.points()) {}

/// @addr{0x806ECC40}
Rail::~Rail() = default;

/// @addr{0x806ECCC0}
void Rail::checkSphereFull() {
    m_floorNrms.reserve(m_pointCount2);

    for (size_t i = 0; i < m_pointCount2; ++i) {
        CourseColMgr::CollisionInfo colInfo;

        if (CollisionDirector::Instance()->checkSphereFull(100.0f, m_points[i].pos,
                    EGG::Vector3f::inf, KCL_TYPE_FLOOR, &colInfo, nullptr, 0)) {
            if (colInfo.floorDist > std::numeric_limits<f32>::min()) {
                m_floorNrms[i] = colInfo.floorNrm;
            }
        } else {
            m_floorNrms[i] = EGG::Vector3f::ey;
        }
    }
}

/// @addr{0x806ECE5C}
void Rail::checkSpherePartialPush() {
    m_floorAttrs.reserve(m_pointCount2);

    CollisionDirector *colDir = CollisionDirector::Instance();

    for (size_t i = 0; i < m_pointCount2; ++i) {
        KCLTypeMask mask = KCL_NONE;

        bool hasCourseCol = colDir->checkSpherePartialPush(100.0f, m_points[i].pos,
                EGG::Vector3f::inf, KCL_TYPE_FLOOR, nullptr, &mask, 0);

        if (hasCourseCol && (mask & KCL_TYPE_FLOOR) &&
                colDir->findClosestCollisionEntry(&mask, KCL_TYPE_FLOOR)) {
            m_floorAttrs[i] = colDir->closestCollisionEntry()->attribute >> 8 & 7;
        } else {
            m_floorAttrs[i] = -1;
        }
    }
}

} // namespace Field
