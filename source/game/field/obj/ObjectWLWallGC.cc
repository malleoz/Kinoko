#include "ObjectWLWallGC.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x8086BC1C}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectWLWallGC::ObjectWLWallGC(const System::MapdataGeoObj &params)
    : ObjectKCL(params), m_extendedDuration(static_cast<s32>(params.setting(1))),
      m_startFrame(static_cast<s32>(params.setting(4))), m_initialPos(pos()) {
    u32 rate = params.setting(2);
    u16 distance = params.setting(3);

    if (rate == 0) {
        m_moveDuration = -1;
        m_hiddenDuration = -1;
        m_extendedFrame = -1;
        m_retractingFrame = -1;
        m_cycleDuration = -1;
    } else {
        m_moveDuration = distance / rate;
        m_hiddenDuration = params.setting(0);
        m_extendedFrame = m_hiddenDuration + m_moveDuration;
        m_retractingFrame = m_extendedFrame + m_extendedDuration;
        m_cycleDuration = m_retractingFrame + m_moveDuration;
    }

    calcTransform();

    m_extendedPos = m_initialPos - transform().base(2) * static_cast<f32>(distance);

    calcTransform();
    m_rtMat = transform();
}

/// @addr{0x8086BDE4}
/// @brief Default virtual destructor
ObjectWLWallGC::~ObjectWLWallGC() = default;

/// @addr{0x8086BF30}
/// @details Linearly interpolates between the piranha's initial position and its extended position
/// based on the current frame within the movement cycle.
const EGG::Matrix34f &ObjectWLWallGC::getUpdatedMatrix(u32 timeOffset) {
    s32 time = cycleFrame(System::RaceManager::Instance()->timer() - timeOffset);

    f32 t;
    if (time < m_hiddenDuration) {
        t = 0.0f;
    } else if (time < m_extendedFrame) {
        t = static_cast<f32>(time - m_hiddenDuration) / static_cast<f32>(m_moveDuration);
    } else if (time < m_retractingFrame) {
        t = 1.0f;
    } else {
        t = 1.0f - static_cast<f32>(time - m_retractingFrame) / static_cast<f32>(m_moveDuration);
    }

    m_rtMat.setBase(3, Interpolate(t, m_initialPos, m_extendedPos));

    return m_rtMat;
}

} // namespace Kinoko::Field
