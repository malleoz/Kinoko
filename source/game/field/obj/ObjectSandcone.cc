#include "ObjectSandcone.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x80686F84}
ObjectSandcone::ObjectSandcone(const System::MapdataGeoObj &params) : ObjectKCL(params) {
    m_flowRate = params.setting(0) / 100.0f;
    m_finalHeightDelta = static_cast<f32>(params.setting(1));
    m_startFrame = params.setting(2);
    m_rtMtx.makeRT(m_rot, m_pos);
}

/// @addr{0x806871E0}
ObjectSandcone::~ObjectSandcone() = default;

/// @addr{0x806872A0}
void ObjectSandcone::init() {
    m_duration = m_finalHeightDelta / m_flowRate;
    m_currentMtx = m_rtMtx;
}

/// @addr{0x806873Bc}
void ObjectSandcone::calc() {
    m_flags |= 0x4;
    m_transform = getUpdatedMatrix(0);
    m_pos = m_transform.base(3);
}

/// @addr{0x80687E14}
u32 ObjectSandcone::loadFlags() const {
    return 1;
}

/// @addr{0x80687800}
const EGG::Matrix34f &ObjectSandcone::getUpdatedMatrix(u32 timeOffset) {
    m_currentMtx = m_rtMtx;

    u32 currentTime = System::RaceManager::Instance()->timer() - timeOffset;

    // The sandcone has finished "flowing", so just return the final position.
    // For Kinoko, we can introduce a slight performance improvement by caching the final position.
    if (currentTime > m_startFrame + m_duration) {
        if (!m_cachedFinalPos) {
            m_cachedFinalPos = m_rtMtx.base(3) + EGG::Vector3f::ey * (m_duration * m_flowRate);
        }

        m_currentMtx.setBase(3, *m_cachedFinalPos);
    } else if (currentTime > m_startFrame) {
        EGG::Vector3f deltaPos = EGG::Vector3f::ey * ((currentTime - m_startFrame) * m_flowRate);
        m_currentMtx.setBase(3, m_rtMtx.base(3) + deltaPos);
    }

    return m_currentMtx;
}

/// @addr{0x80687A2C}
bool ObjectSandcone::checkCollision(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x80687CC0}
bool ObjectSandcone::checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);

    return m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut);
}

} // namespace Field
