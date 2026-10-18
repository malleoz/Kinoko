#include "KartPhysics.hh"

#include "game/kart/CollisionGroup.hh"
#include "game/kart/KartDynamics.hh"

namespace Kinoko::Kart {

/// @addr{0x8059F5BC}
/// @brief Constructor which initializes the @ref KartDynamics or @ref KartDynamicsBike
/// subsystem depending on `isBike` and constructs the kart's @ref CollisionGroup
/// @param isBike Whether the kart is a bike
/// @details Constructs the @ref KartDynamics or @ref KartDynamicsBike subsystem based on
/// whether the kart is a bike and also constructs the kart's @ref CollisionGroup subsystem.
/// Initializes @ref m_pose to the identity matrix and sets @ref m_halfLength to `50.0f`.
KartPhysics::KartPhysics(bool isBike)
    : m_dynamics(isBike ? EGG::egg_new<KartDynamicsBike>() : EGG::egg_new<KartDynamics>()),
      m_hitboxGroup(EGG::egg_new<CollisionGroup>()) {
    m_pose = EGG::Matrix34f::ident;
    m_halfLength = 50.0f; // set immediately after in KartPhysics::Create()
}

/// @addr{0x8059F6F8}
/// @brief Destructor that destroys the @ref KartDynamics and @ref CollisionGroup subsystem
KartPhysics::~KartPhysics() {
    EGG::egg_delete(m_dynamics);
    EGG::egg_delete(m_hitboxGroup);
}

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

/// @addr{0x805A0340}
/// @brief Constructs a transformation matrix from rotation and position
/// @details Recomputes @ref m_pose based on the kart's @ref KartDynamics::m_fullRot and @ref
/// KartDynamics::m_pos. Updates @ref m_xAxis, @ref m_yAxis, and @ref m_zAxis based on the new
/// pose.
void KartPhysics::updatePose() {
    m_pose.makeQT(m_dynamics->fullRot(), m_dynamics->pos());
    m_xAxis = m_pose.base(0);
    m_yAxis = m_pose.base(1);
    m_zAxis = m_pose.base(2);
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

/// @brief Sets the position of the kart on the last frame
/// @param pos The position to set for the kart on the last frame
void KartPhysics::setPos(const EGG::Vector3f &pos) {
    m_pos = pos;
}

/// @brief Sets the velocity of the kart on the last frame
/// @param vel The velocity to set for the kart on the last frame
void KartPhysics::setVelocity(const EGG::Vector3f &vel) {
    m_velocity = vel;
}

/// @brief Sets the half-length of the kart's collision box
/// @param val The half-length to set for the kart's collision box
void KartPhysics::setHalfLength(f32 val) {
    m_halfLength = val;
}

/// @addr{0x8059FC48}
/// @brief Sets a rotation resulting from a trick that is applied for only the current frame
/// @param rot The rotation to apply
/// @details The provided `rot` is composed into @ref m_instantaneousStuntRot.
void KartPhysics::composeStuntRot(const EGG::Quatf &rot) {
    m_instantaneousStuntRot *= rot;
}

/// @addr{0x8059FD0C}
/// @brief Sets a corrective rotation that is applied for only the current frame, such as when
/// in a spinout action, in a launch action, or learning in an automatic drift
/// @param rot The rotation to apply
/// @details The provided `rot` is composed into @ref m_instantaneousExtraRot.
void KartPhysics::composeExtraRot(const EGG::Quatf &rot) {
    m_instantaneousExtraRot *= rot;
}

/// @addr{0x8059FDD0}
/// @brief Sets a rotation resulting from a trick that decays 10% per frame, such as when
/// landing from a zipper trick
/// @param rot The rotation to apply
/// @details The provided `rot` is composed into @ref m_decayingStuntRot.
void KartPhysics::composeDecayingStuntRot(const EGG::Quatf &rot) {
    m_decayingStuntRot *= rot;
}

/// @addr{0x8059FE94}
/// @brief Sets a corrective rotation that decays 10% per frame, such as when landing from a
/// launch action
/// @param rot The rotation to apply
/// @details The provided `rot` is composed into @ref m_decayingExtraRot.
void KartPhysics::composeDecayingExtraRot(const EGG::Quatf &rot) {
    m_decayingExtraRot *= rot;
}

/// @addr{0x805A0050}
/// @brief Linearly interpolates the moving object velocity towards the provided velocity
/// @param vel The target velocity
/// @param t The interpolation rate
/// @details Interpolates @ref m_movingObjVel towards the provided `vel` using the interpolation
/// rate `t`. Saves the velocity to @ref KartDynamics::m_movingObjVel.
void KartPhysics::composeMovingObjVel(const EGG::Vector3f &vel, f32 t) {
    m_movingObjVel += (vel - m_movingObjVel) * t;
    m_dynamics->setMovingObjVel(m_movingObjVel);
}

/// @addr{0x805A00D0}
/// @brief Decays the moving object velocity by the provided scalars
/// @param floorScalar The decay factor to apply when on the ground
/// @param airScalar The decay factor to apply when in the air
/// @param floor Whether the kart is on the ground
/// @details Decays @ref m_movingObjVel by the appropriate scalar depending on whether the kart
/// is on the ground or in the air. Saves the velocity to @ref KartDynamics::m_movingObjVel.
void KartPhysics::composeDecayingMovingObjVel(f32 floorScalar, f32 airScalar, bool floor) {
    m_movingObjVel *= floor ? floorScalar : airScalar;
    m_dynamics->setMovingObjVel(m_movingObjVel);
}

/// @addr{0x805A014C}
/// @brief Linearly interpolates the moving road velocity towards the provided velocity
/// @param vel The target velocity
/// @param t The interpolation factor
/// @details Interpolates @ref m_movingRoadVel towards the provided `vel` using the
/// interpolation factor `t`. Saves the velocity to @ref KartDynamics::m_movingRoadVel.
void KartPhysics::composeMovingRoadVel(const EGG::Vector3f &vel, f32 t) {
    m_movingRoadVel += (vel - m_movingRoadVel) * t;
    m_dynamics->setMovingRoadVel(m_movingRoadVel);
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

/// @addr{0x805A02B8}
/// @brief Decays the moving road velocity by the provided scalars
/// @param floorScalar The decay factor to apply when on the ground
/// @param airScalar The decay factor to apply when in the air
/// @param floor Whether the kart is on the ground
/// @details Decays @ref m_movingRoadVel by the appropriate scalar depending on whether
/// the kart is on the ground or in the air. Sets the y-component of @ref
/// m_movingRoadVel to 0 and saves the velocity to @ref KartDynamics::m_movingRoadVel.
void KartPhysics::decayMovingRoadVel(f32 floorScalar, f32 airScalar, bool floor) {
    m_movingRoadVel *= floor ? floorScalar : airScalar;
    m_movingRoadVel.y = 0.0f;
    m_dynamics->setMovingRoadVel(m_movingRoadVel);
}

/// @addr{0x805A0410}
/// @brief Clears the decaying stunt and extra rotations
/// @details Sets @ref m_decayingStuntRot and @ref m_decayingExtraRot to the identity
/// quaternion.
void KartPhysics::clearDecayingRot() {
    m_decayingStuntRot = EGG::Quatf::ident;
    m_decayingExtraRot = EGG::Quatf::ident;
}

/// @brief Gets the underlying @ref KartDynamics subsystem
/// @return A pointer to the underlying @ref KartDynamics subsystem
KartDynamics *KartPhysics::dynamics() {
    return m_dynamics;
}

/// @brief Gets the underlying @ref KartDynamics subsystem
/// @return A const pointer to the underlying @ref KartDynamics subsystem
const KartDynamics *KartPhysics::dynamics() const {
    return m_dynamics;
}

/// @brief Gets the pose matrix of the kart
/// @return A const reference to the pose matrix of the kart
const EGG::Matrix34f &KartPhysics::pose() const {
    return m_pose;
}

/// @brief Gets the collision group of the kart
/// @return A pointer to the collision group of the kart
CollisionGroup *KartPhysics::hitboxGroup() {
    return m_hitboxGroup;
}

/// @brief Gets the x-axis pose of the kart
/// @return A const reference to the x-axis of the kart
const EGG::Vector3f &KartPhysics::xAxis() const {
    return m_xAxis;
}

/// @brief Gets the y-axis pose of the kart
/// @return A const reference to the y-axis of the kart
const EGG::Vector3f &KartPhysics::yAxis() const {
    return m_yAxis;
}

/// @brief Gets the z-axis pose of the kart
/// @return A const reference to the z-axis of the kart
const EGG::Vector3f &KartPhysics::zAxis() const {
    return m_zAxis;
}

/// @brief Gets the last frame's finalized position of the kart
/// @return A const reference to the kart finalized position on the last frame
const EGG::Vector3f &KartPhysics::pos() const {
    return m_pos;
}

/// @brief Gets half of the kart's largest Z-axis hitbox extent
/// @return The half length of the kart's largest Z-axis hitbox extent
f32 KartPhysics::halfLength() const {
    return m_halfLength;
}

/// @addr{0x805A04A0}
/// @brief Factory function that constructs a @ref KartPhysics object
/// @param param The kart parameters used to initialize the physics object
/// @return A pointer to the newly created @ref KartPhysics object
/// @details Initializes the hitboxes of the kart's @ref CollisionGroup based on the
/// hitboxes defined in the @ref BSP from the provided `param`. Sets @ref
/// KartPhysics::m_halfLength based on the initialized hitboxes, and saves some @ref BSP
/// params via @ref KartDynamics::setBspParams().
KartPhysics *KartPhysics::Create(const KartParam &param) {
    KartPhysics *physics = EGG::egg_new<KartPhysics>(param.isBike());
    const BSP &bsp = param.bsp();
    physics->setHalfLength(physics->hitboxGroup()->initHitboxes(bsp.hitboxes));
    physics->dynamics()->setBspParams(bsp.angVel0Factor, bsp.cuboids[0], bsp.cuboids[1], false);

    return physics;
}

} // namespace Kinoko::Kart
