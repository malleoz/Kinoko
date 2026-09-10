#include "ObjectCrane.hh"

namespace Kinoko::Field {

/// @addr{0x807FE658}
/// @brief Constructor
/// @param params The parameters used to initialize the object
/// @details Sets the initial x-axis phase based off param setting 4 and sets the y-axis phase to
/// zero.
ObjectCrane::ObjectCrane(const System::MapdataGeoObj &params)
    : ObjectKCL(params),
      m_startPos(pos()),
      m_xPeriod(std::max<u16>(2, params.setting(1))),
      m_yPeriod(std::max<u16>(2, params.setting(4))),
      m_xAmplitude(params.setting(2)),
      m_yAmplitude(params.setting(5)),
      m_xFreq(2.0f * F_PI / static_cast<f32>(m_xPeriod)),
      m_yFreq(2.0f * F_PI / static_cast<f32>(m_yPeriod)) {
    m_xt = params.setting(3);
    m_yt = 0;
}

/// @addr{0x807FE7EC}
/// @copybrief ObjectBase::calc()
/// @details Updates the crane's position based on its x and y oscillation periods and amplitudes.
/// Increments the x and y phase counters accordingly. Finally, computes the moving object velocity
/// based on the change in position this frame.
void ObjectCrane::calc() {
    const EGG::Vector3f prevPos = pos();

    f32 xDelta = EGG::Mathf::cos(m_xFreq * static_cast<f32>(m_xt));
    EGG::Vector3f scaledX = EGG::Vector3f::ex * xDelta * static_cast<f32>(m_xAmplitude);

    f32 yDelta = EGG::Mathf::cos(m_yFreq * static_cast<f32>(m_yt));
    EGG::Vector3f scaledY = EGG::Vector3f::ey * yDelta * static_cast<f32>(m_yAmplitude);

    calcTransform();
    setPos(m_startPos + transform().multVector33(scaledX + scaledY));

    if (m_yt++ > m_yPeriod) {
        m_yt = 0;
    }

    if (m_xt++ > m_xPeriod) {
        m_xt = 0;
    }

    setMovingObjVel(pos() - prevPos);
}

} // namespace Kinoko::Field
