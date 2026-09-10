#include "game/field/obj/ObjectBulldozer.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x807FD938}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectBulldozer::ObjectBulldozer(const System::MapdataGeoObj &params)
    : ObjectKCL(params),
      m_initialPos(pos()),
      m_initialRot(rot()),
      m_timeOffset(params.setting(3) * 2),
      m_periodDenom(std::max<u16>(2, params.setting(2))),
      m_restFrames(params.setting(4)),
      m_fullPeriod(m_periodDenom + m_restFrames * 2),
      m_amplitude(params.setting(1)),
      m_left(strcmp(getName(), "bulldozer_left") == 0),
      m_period(F_TAU / static_cast<f32>(m_periodDenom)),
      m_halfPeriod(m_fullPeriod / 2) {}

/// @addr{0x807FDC50}
/// @copybrief ObjectBase::calc()
/// @details Updates the position of the bulldozer based on its oscillation on the current frame.
void ObjectBulldozer::calc() {
    u32 timer = System::RaceManager::Instance()->timer();
    f32 posOffset = calcPosOffset(m_timeOffset + timer);
    EGG::Vector3f prevPos = pos();
    f32 xPos = m_left ? m_initialPos.x + posOffset : m_initialPos.x - posOffset;
    setPos(EGG::Vector3f(xPos, prevPos.y, prevPos.z));
    setMovingObjVel(pos() - prevPos);
}

/// @addr{0x807FE364}
/// @copybrief ObjectKCL::initCollision()
/// @details Initializes the collision manager and calculates the midpoint of the bulldozer's
/// collision bounding box.
void ObjectBulldozer::initCollision() {
    calcTransform();

    EGG::Matrix34f matInv;
    transform().ps_inverse(matInv);

    calcTransform();

    m_objColMgr->setMtx(transform());
    m_objColMgr->setInvMtx(matInv);
    m_objColMgr->setScale(getScaleY(0));

    ObjectKCL::initCollision();

    m_kclMidpoint = (m_objColMgr->kclHighWorld() + m_objColMgr->kclLowWorld()).multInv(2.0f);
}

/// @addr{0x807FE534}
/// @copybrief ObjectKCL::getUpdatedMatrix()
/// @param timeOffset The time offset used to calculate the current frame's transformation
/// @return Const ref to the updated transformation matrix for the current frame, taking into
/// account the time offset.
/// @details Rotation stays constant. The position offset is based off the bulldozer's facing
/// direction and its oscillation.
const EGG::Matrix34f &ObjectBulldozer::getUpdatedMatrix(u32 timeOffset) {
    EGG::Vector3f pos = m_initialPos;
    u32 timer = System::RaceManager::Instance()->timer();
    f32 posOffset = calcPosOffset(m_timeOffset + timer - timeOffset);

    pos.x = m_left ? pos.x + posOffset : pos.x - posOffset;
    m_rtMat.makeRT(m_initialRot, pos);

    return m_rtMat;
}

/// @addr{0x807FDE5C}
/// @brief Based off `t`, determine the position offset from the bulldozer's initial position
/// @param t The frame on which to calculate the position offset
/// @return The position offset from the bulldozer's initial position on frame `t`.
/// @details Calculates the position offset of the bulldozer based on its sinusoidal oscillation at
/// frame `t`.
f32 ObjectBulldozer::calcPosOffset(u32 t) const {
    u16 phase = t % m_fullPeriod;

    if (phase >= m_halfPeriod - m_restFrames) {
        if (phase < m_halfPeriod) {
            phase = m_periodDenom / 2;
        } else if (phase < m_fullPeriod - m_restFrames) {
            phase -= m_restFrames;
        } else {
            phase = m_periodDenom;
        }
    }

    return static_cast<f32>(m_amplitude) *
            (1.0f + EGG::Mathf::cos(m_period * static_cast<f32>(phase))) * 0.5f;
}

} // namespace Kinoko::Field
