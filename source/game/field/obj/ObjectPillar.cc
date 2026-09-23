
#include "ObjectPillar.hh"

namespace Kinoko::Field {

/// @addr{0x807FEC30}
/// @copybrief ObjectBase::calcCollisionTransform()
/// @details Extends the falling pillar's collision transform upwards and in the direction of its
/// fall.
void ObjectPillarC::calcCollisionTransform() {
    constexpr f32 HEIGHT = 1900.0f;

    if (!m_collision) {
        return;
    }

    EGG::Matrix34f mat = EGG::Matrix34f::zero;
    mat.makeT(EGG::Vector3f(0.0f, 2.0f * (HEIGHT * scale().y) / 3.0f, 0.0f));

    calcTransform();

    EGG::Vector3f speed = transform().ps_multVector(EGG::Vector3f::ez * 1000.0f);
    m_collision->transform(transform().multiplyTo(mat), scale(), speed);
}

/// @addr{0x807FF17C}
/// @copybrief ObjectBase::calc()
/// @details Checks the current race duration to update the pillar's fall state and rotation. If the
/// pillar is upright, then the @ref ObjectPillarC collision is enabled. If the pillar is falling,
/// then the rotation is updated and the collision transform is updated. If the pillar has finished
/// falling, then the @ref ObjectPillarC collision is disabled and the @ref ObjectPillar collision
/// is enabled.
void ObjectPillar::calc() {
    u32 time = System::RaceManager::Instance()->timer();

    if (m_state == State::Upright && time < m_fallStart) {
        m_collidable->enableCollision();
    } else if (m_state == State::Upright && time == m_fallStart) {
        // The pillar has now started to fall.
        m_state = State::Break;
    } else if (m_state == State::Break) {
        f32 rot = calcRot(static_cast<s32>(time));
        if (rot < m_targetRot) {
            setTransform(getUpdatedMatrix(0));
            m_collidable->setTransform(transform());

            setRot(EGG::Vector3f(rot, m_currRot.y, m_currRot.z));
        } else {
            // The pillar has finished falling.
            // We can now drive on top of the base and on the pillar itself.
            m_state = State::Ground;

            m_collidable->disableCollision();
            enableCollision();
            m_groundFrame = time;
        }

    } else {
        m_state = State::Ground;
    }
}

} // namespace Kinoko::Field
