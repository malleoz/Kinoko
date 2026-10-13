#include "KartPhysics.hh"

namespace Kinoko::Kart {

/// @addr{0x8059F7C8}
/// @brief Resets internal variables to their initial state
/// @details Dispatches to @ref KartDynamics::init() and @ref CollisionGroup::reset() to initialize
/// the @ref m_dynamics and @ref m_hitboxGroup subsystems. Initializes all rotation-related members
/// to the identity quaternion. Initializes @ref m_movingObjVel and @ref m_movingRoadVel to zero.
/// Initializes the pose matrix to the identity matrix and updates the axis vectors accordingly.
/// Sets the position and velocity from the @ref m_dynamics subsystem.
void KartPhysics::reset() {
    m_dynamics->init();
    m_hitboxGroup->reset();
    m_decayingStuntRot = EGG::Quatf::ident;
    m_instantaneousStuntRot = EGG::Quatf::ident;
    m_stuntRot = EGG::Quatf::ident;
    m_decayingExtraRot = EGG::Quatf::ident;
    m_instantaneousExtraRot = EGG::Quatf::ident;
    m_extraRot = EGG::Quatf::ident;
    m_movingObjVel.setZero();
    m_movingRoadVel.setZero();
    m_pose = EGG::Matrix34f::ident;
    m_xAxis = m_pose.base(0);
    m_yAxis = m_pose.base(1);
    m_zAxis = m_pose.base(2);
    m_pos = m_dynamics->pos();
    m_velocity = m_dynamics->velocity();
}

/// @addr{0x8059F968}
/// @brief Computes trick and correction rotation and calls to @ref KartDynamics::calc().
/// @param dt delta time step (always `1.0f`)
/// @param maxSpeed `120.0f`, unless we're in a bullet (`145.0f`)
/// @param air `true` if the kart is mid-air, `false` if it is touching the ground
/// @details Composes @ref m_instantaneousStuntRot and @ref m_decayingStuntRot, saving the result to
/// @ref m_stuntRot. Composes @ref m_instantaneousExtraRot and @ref m_decayingExtraRot, saving the
/// result to @ref m_extraRot. Sets these rotations in the @ref m_dynamics subsystem. Calls @ref
/// KartDynamics::setScale() with the provided `scale`. Uses spherical linear interpolation to decay
/// @ref m_decayingStuntRot and @ref m_decayingExtraRot towards the identity quaternion with a rate
/// of `0.1f`. Resets @ref m_instantaneousStuntRot and @ref m_instantaneousExtraRot to the identity
/// quaternion since these only apply for the current frame.
void KartPhysics::calc(f32 dt, f32 maxSpeed, const EGG::Vector3f &scale, bool air) {
    constexpr f32 DECAY_INTERP_RATE = 0.1f;

    m_stuntRot = m_instantaneousStuntRot * m_decayingStuntRot;
    m_extraRot = m_instantaneousExtraRot * m_decayingExtraRot;

    m_dynamics->setStuntRot(m_stuntRot);
    m_dynamics->setExtraRot(m_extraRot);
    m_dynamics->setScale(scale);

    m_dynamics->calc(dt, maxSpeed, air);

    m_decayingStuntRot = m_decayingStuntRot.slerpTo(EGG::Quatf::ident, DECAY_INTERP_RATE);
    m_decayingExtraRot = m_decayingExtraRot.slerpTo(EGG::Quatf::ident, DECAY_INTERP_RATE);

    m_instantaneousStuntRot = EGG::Quatf::ident;
    m_instantaneousExtraRot = EGG::Quatf::ident;
}

/// @addr{0x805A01CC}
/// @brief Adds velocity to the moving road velocity and clamps it to the provided maximum speed
/// @param v The velocity to add
/// @param maxPullSpeed The maximum speed to clamp the moving road velocity to
/// @details Adds the provided velocity `v` to the moving road velocity @ref m_movingRoadVel and
/// clamps its magnitude to the specified `maxPullSpeed`. Applies the result to @ref m_dynamics via
/// @ref KartDynamics::setMovingRoadVel().
void KartPhysics::shiftDecayMovingRoadVel(const EGG::Vector3f &v, f32 maxPullSpeed) {
    m_movingRoadVel += v;

    if (m_movingRoadVel.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        f32 speed = std::min(maxPullSpeed, m_movingRoadVel.normalise());
        m_movingRoadVel *= speed;
        m_dynamics->setMovingRoadVel(m_movingRoadVel);
    }
}

} // namespace Kinoko::Kart
