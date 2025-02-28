#include "ObjectCrane.hh"

#include <egg/math/Math.hh>

namespace Field {

/// @addr{0x807FE658}
ObjectCrane::ObjectCrane(const System::MapdataGeoObj &params) : ObjectKCL(params) {
    m_startPos = m_pos;
    m_startRot = m_rot;
    m_startScale = m_scale;

    m_t = params.setting(3);
    _12a = 0;
    m_period1Denom = std::max(static_cast<u16>(2), params.setting(1));
    m_period2Denom = std::max(static_cast<u16>(2), params.setting(4));
    _130 = params.setting(2);
    _132 = params.setting(5);

    m_period1 = 2 * F_PI / static_cast<f32>(m_period1Denom);
    m_period2 = 2 * F_PI / static_cast<f32>(m_period2Denom);
}

/// @addr{0x807FEB28}
ObjectCrane::~ObjectCrane() = default;

/// @addr{0x807FE7EC}
void ObjectCrane::calc() {
    EGG::Vector3f prevPos = m_pos;

    f32 fVar5 = EGG::Mathf::cos(m_period1 * static_cast<f32>(m_t));
    EGG::Vector3f scaledX = EGG::Vector3f::ex * fVar5 * static_cast<f32>(_130);

    fVar5 = EGG::Mathf::cos(m_period2 * static_cast<f32>(_12a));
    EGG::Vector3f scaledY = EGG::Vector3f::ey * fVar5 * static_cast<f32>(_132);

    calcTransform();

    m_pos = m_startPos + m_transform.multVector33(scaledX + scaledY);
    m_flags |= 1;

    if (_12a++ > m_period2Denom) {
        _12a = 0;
    }

    if (m_t++ > m_period1Denom) {
        m_t = 0;
    }

    setSomePos(m_pos - prevPos);
}

/// @addr{0x807FEB20}
u32 ObjectCrane::loadFlags() const {
    return 1;
}

/// @addr{0x807FEAF0}
f32 ObjectCrane::colRadiusAdditionalLength() const {
    return _130;
}

} // namespace Field
