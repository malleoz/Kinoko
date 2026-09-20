#include "ObjectKCL.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x8081AB4C}
/// @copybrief ObjectDrivable::initCollision()
/// @details Saves the initial transformation matrix to the @ref ObjColMgr and computes the KCL's
/// bounding box midpoint and half-width.
void ObjectKCL::initCollision() {
    const EGG::Matrix34f &mat = getUpdatedMatrix(0);
    EGG::Matrix34f matInv;
    bool inverted = mat.ps_inverse(matInv);
    ASSERT(inverted);

    m_objColMgr->setMtx(mat);
    m_objColMgr->setInvMtx(matInv);
    m_objColMgr->setScale(getScaleY(0));

    EGG::Vector3f high = m_objColMgr->kclHighWorld();
    EGG::Vector3f low = m_objColMgr->kclLowWorld();

    m_kclMidpoint = (high + low).multInv(2.0f);

    EGG::Vector3f highLowDiffAbs = (high - low).abs();
    f32 maxDiff = std::max(highLowDiffAbs.x, highLowDiffAbs.z);
    m_bboxHalfSideLength = maxDiff * 0.5f;
}

/// @addr{0x8081AD6C}
/// @brief Advances the collision manager's transform to reflect the current frame
/// @param timeOffset The time offset used to calculate the current frame's transformation
/// @details If the collision manager's transform has already been updated for the current frame,
/// this function does nothing. Otherwise, it computes the updated transformation matrix and sets
/// it in the collision manager. If `timeOffset` is 0, then computes the transformation via @ref
/// calcTransform(), otherwise calls @ref getUpdatedMatrix(). Finally, updates @ref
/// m_lastMtxUpdateFrame.
void ObjectKCL::update(u32 timeOffset) {
    u32 time = System::RaceManager::Instance()->timer() - timeOffset;
    if (m_lastMtxUpdateFrame == static_cast<s32>(time)) {
        return;
    }

    EGG::Matrix34f mat;

    if (timeOffset == 0) {
        calcTransform();
        mat = transform();
    } else {
        mat = getUpdatedMatrix(timeOffset);
    }

    EGG::Matrix34f matInv;
    mat.ps_inverse(matInv);
    m_objColMgr->setMtx(mat);
    m_objColMgr->setInvMtx(matInv);

    m_lastMtxUpdateFrame = time;
}

/// @addr{0x8081AF28}
/// @brief Updates the collision manager's scale to reflect the current frame
/// @param timeOffset The time offset used to calculate the current frame's transformation
/// @details If the collision manager's scale has already been updated for the current frame,
/// this function does nothing. Otherwise, it computes the updated scale and sets it in the
/// collision manager. If `timeOffset` is 0, then uses the default scale via @ref scale().y,
/// otherwise calls @ref getScaleY(timeOffset). Finally, updates @ref m_lastScaleUpdateFrame.
void ObjectKCL::calcScale(u32 timeOffset) {
    u32 time = System::RaceManager::Instance()->timer() - timeOffset;
    if (m_lastScaleUpdateFrame == static_cast<s32>(time)) {
        return;
    }

    m_objColMgr->setScale(time == 0 ? scale().y : getScaleY(timeOffset));
    m_lastScaleUpdateFrame = time;
}

} // namespace Kinoko::Field
