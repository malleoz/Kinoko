#include "ObjectSandcone.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x80686F84}
ObjectSandcone::ObjectSandcone(const System::MapdataGeoObj &params)
    : ObjectKCL(params), m_flowRate(static_cast<f32>(params.setting(0)) / 100.0f),
      m_finalHeightDelta(static_cast<f32>(params.setting(1))), m_startFrame(params.setting(2)) {
    m_rtMat.makeRT(rot(), pos());
}

/// @addr{0x806871E0}
ObjectSandcone::~ObjectSandcone() = default;

/// @addr{0x80687800}
/// @details Based off the current race timer, raises the sandcone's height gradually until it
/// reaches the final height.
const EGG::Matrix34f &ObjectSandcone::getUpdatedMatrix(u32 timeOffset) {
    m_currentMtx = m_rtMat;

    u32 t = System::RaceManager::Instance()->timer() - timeOffset;

    if (t > m_startFrame + m_duration) {
        // The sandcone has finished "flowing", so just return the final position.
        // For Kinoko, we introduce a slight performance improvement by caching the m_finalPos.
        m_currentMtx.setBase(3, m_finalPos);
    } else if (t > m_startFrame) {
        EGG::Vector3f deltaPos = EGG::Vector3f::ey * ((t - m_startFrame) * m_flowRate);
        m_currentMtx.setBase(3, m_rtMat.base(3) + deltaPos);
    }

    return m_currentMtx;
}

} // namespace Kinoko::Field
