#include "ObjectHwanwan.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806E9724}
/// @copybrief ObjectBase::init()
/// @details Initializes the Chain Chomp's position such that the bottom of its spherical body is at
/// the initial position. Resets velocity and orientation vectors. Updates the Chain Chomp's
/// transform and initializes it to the bouncing/walking state.
void ObjectHwanwan::init() {
    m_workPos = m_initPos + EGG::Vector3f::ey * DIAMETER;
    m_extVel.setZero();
    m_bounceVel.setZero();
    m_tangent = EGG::Vector3f::ez;
    m_up = EGG::Vector3f::ey;
    m_targetUp = EGG::Vector3f::ey;
    m_touchingGround = false;
    m_targetY = m_workPos.y;

    calcTransform();

    m_nextStateId = 0;
}

/// @addr{0x806E9A78}
/// @copybrief ObjectBase::calc()
/// @details Evaluates the Chain Chomp's state machine. Applies a downward gravitational force of
/// `2.5f` every frame, updating its velocity and position accordingly. Calls @ref
/// checkFloorCollision() to see if the Chain Chomp should bounce. Finally, it updates the Chain
/// Chomp's transform to reflect its updated position and orientation.
void ObjectHwanwan::calc() {
    constexpr EGG::Vector3f GRAVITY = EGG::Vector3f(0.0f, 2.5f, 0.0f);

    StateManager::calc();

    m_extVel += m_bounceVel - GRAVITY;
    m_workPos += m_extVel;
    m_bounceVel.setZero();

    checkFloorCollision();

    EGG::Matrix34f mat;
    SetRotTangentHorizontal(mat, m_up, m_tangent);
    mat.setBase(3, m_workPos);
    setTransform(mat);
}

/// @addr{0x806EA784}
/// @brief Checks for a collision between the Chain Chomp and the floor
/// @details If a collision is detected and the Chain Chomp's Y-position is within `300.0f` frames
/// of the rail height, sets @ref m_touchingGround to `true`, offsets the Chain Chomp's position to
/// be on the floor, sets @ref m_targetUp to the floor's normal, and resets the external velocity to
/// zero.
void ObjectHwanwan::checkFloorCollision() {
    constexpr f32 RADIUS = DIAMETER * 0.5f;

    m_touchingGround = false;

    CollisionInfo colInfo;
    KCLTypeMask mask;
    EGG::Vector3f pos = m_workPos + EGG::Vector3f(0.0f, -DIAMETER, 0.0f);

    bool hasCol = CollisionDirector::Instance()->checkSphereFullPush(RADIUS, pos,
            EGG::Vector3f::inf, KCL_TYPE_FLOOR, &colInfo, &mask, 0);

    if (!hasCol || pos.y - m_targetY >= 300.0f) {
        return;
    }

    m_touchingGround = true;

    f32 len = colInfo.tangentOff.length();
    m_workPos += EGG::Vector3f::ey * len;

    if (colInfo.floorDist > -std::numeric_limits<f32>::min()) {
        m_targetUp = colInfo.floorNrm;
    }
    m_extVel.y = 0.0f;
}

/// @addr{0x806C571C}
/// @copybrief ObjectBase::init()
/// @details Initializes the rail interpolator to the beginning of the rail. Sets the Chain Chomp's
/// position and tangent to match the rail's initial position and orientation, and calls @ref
/// ObjectHwanwan::calc() twice to update its state accordingly. Finally, sets the rail
/// interpolator's speed based off param setting 1.
/// @note It is not clear why @ref ObjectHwanwan::calc() is called twice, but this must be done in
/// Kinoko to match the base game's behavior.
void ObjectHwanwanManager::init() {
    m_railInterpolator->init(0.0f, 0);
    m_hwanwan->m_tangent = m_railInterpolator->curTangentDir();
    const auto &curPos = m_railInterpolator->curPos();
    m_hwanwan->m_workPos.x = curPos.x;
    m_hwanwan->m_workPos.z = curPos.z;
    m_hwanwan->m_targetY = curPos.y;

    m_hwanwan->calc();
    m_hwanwan->calc();

    ASSERT(m_mapObj);
    m_railInterpolator->setSpeed(static_cast<f32>(m_mapObj->setting(0)));
}

} // namespace Kinoko::Field
