#include "game/field/obj/ObjectBulldozer.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x807FDC50}
/// @copybrief ObjectBase::calc()
/// @details Updates the position of the bulldozer based on its oscillation on the current frame.
/// The direction of oscillation is flipped depending on whether @ref m_left is `true`. Finally,
/// sets moving object velocity based on the change in position this frame.
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
/// @details Updates the bulldozer's transformation matrix. Sets the @ref ObjColMgr transformation
/// matrix based off the transform. Calls the base class @ref ObjectKCL::initCollision(). Finally,
/// computes @ref m_kclMidpoint based on the updated collision manager's KCL bounds.
/// @todo The @ref ObjColMgr matrices are immediately overwritten in the call to @ref
/// ObjectKCL::initCollision(). The @ref m_kclMidpoint assignment is also redundant, since the scale
/// is always based on `getScaleY(0)`. We can likely clean this function up for Kinoko to improve
/// performance slightly.
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
/// direction and its oscillation, computed via @ref calcPosOffset().
const EGG::Matrix34f &ObjectBulldozer::getUpdatedMatrix(u32 timeOffset) {
    EGG::Vector3f pos = m_initialPos;
    u32 timer = System::RaceManager::Instance()->timer();
    f32 posOffset = calcPosOffset(m_timeOffset + timer - timeOffset);

    pos.x = m_left ? pos.x + posOffset : pos.x - posOffset;
    m_rtMat.makeRT(m_initialRot, pos);

    return m_rtMat;
}

/// @addr{0x807FDE5C}
/// @brief Determine the position offset from the bulldozer's initial position on the provided frame
/// @param t The frame during which to calculate the position offset
/// @return The position offset from the bulldozer's initial position on frame `t`.
/// @details Maps `t` to the frame within the bulldozer's oscillation period. Adjusts the resulting
/// frame based on where it falls relative to the two resting periods. This is then passed into a
/// `cosine` function with a period of @ref m_period and an amplitude of @ref m_amplitude.
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
