#include "ObjectObakeBlock.hh"

namespace Field {

/// @addr{0x8080AD20}
ObjectObakeBlock::ObjectObakeBlock(const System::MapdataGeoObj &params) : ObjectBase(params) {
    m_framesFallen = 0.0f;
    m_fallVel.setZero();
    m_fallAngVel.setZero();
    m_fallFrame = static_cast<u32>(params.setting(2) + params.setting(1) * 60);
    m_currPos = params.pos();

    switch (params.rot().y) {
    case 0.0f:
        m_fallVel.z = -1.0f;
        m_fallAngVel.x = -0.02f;
        m_bbox.min = m_pos + EGG::Vector3f(162.5f, 162.5f, 162.5f);
        m_bbox.max = m_pos + EGG::Vector3f(-162.5f, 162.5f, 162.5f);
        break;
    case 90.0f:
        m_fallVel.x = -1.0f;
        m_fallAngVel.x = 0.02f;
        m_bbox.min = m_pos + EGG::Vector3f(162.5f, 162.5f, 162.5f);
        m_bbox.max = m_pos + EGG::Vector3f(162.5f, 162.5f, -162.5f);
        break;
    case 180.0f:
        m_fallVel.z = 1.0f;
        m_fallAngVel.x = 0.02f;
        m_bbox.min = m_pos + EGG::Vector3f(162.5f, 162.5f, -162.5f);
        m_bbox.max = m_pos + EGG::Vector3f(-162.5f, 162.5f, -162.5f);
        break;
    case -90.0f:
        m_fallVel.x = 1.0f;
        m_fallAngVel.z = -0.02f;
        m_bbox.min = m_pos + EGG::Vector3f(-162.5f, 162.5f, 162.5f);
        m_bbox.max = m_pos + EGG::Vector3f(-162.5f, 162.5f, -162.5f);
        break;
    default:
        break;
    }
}

/// @addr{0x8080D8FC}
ObjectObakeBlock::~ObjectObakeBlock() = default;

/// @addr{0x8080BC64}
void ObjectObakeBlock::calc() {
    if (m_fallState != FallState::Falling) {
        return;
    }

    m_pos += m_fallVel * (m_framesFallen * 2.0f);
    m_rot = m_fallAngVel * m_framesFallen;
    m_pos.y -= (0.5f * m_framesFallen) * (0.5f * m_framesFallen);
    m_flags |= 3;

    if (static_cast<u32>(++m_framesFallen) > 255) {
        m_fallState = FallState::Gone;
    }
}

} // namespace Field
