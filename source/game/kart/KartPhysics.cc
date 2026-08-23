#include "KartPhysics.hh"

namespace Kinoko::Kart {

/// @addr{0x8059F5BC}
/// @brief Initializes the @ref KartDynamics or @ref KartDynamicsBike subsystem depending on
/// `isBike` and constructs the kart's @ref CollisionGroup
/// @param isBike Whether the kart is a bike
KartPhysics::KartPhysics(bool isBike) {
    m_pose = EGG::Matrix34f::ident;
    m_dynamics = isBike ? static_cast<KartDynamics *>(EGG::egg_new<KartDynamicsBike>()) :
                          EGG::egg_new<KartDynamics>();
    m_hitboxGroup = EGG::egg_new<CollisionGroup>();
    m_halfLength = 50.0f; // set immediately after in KartPhysics::Create()
}

/// @addr{0x8059F6F8}
/// @brief Destroys the @ref KartDynamics or @ref KartDynamicsBike subsystem and the kart's @ref
/// CollisionGroup subsystem
KartPhysics::~KartPhysics() {
    EGG::egg_delete(m_dynamics);
    EGG::egg_delete(m_hitboxGroup);
}

/// @addr{0x8059F7C8}
/// @brief Resets the @ref KartDynamics and @ref CollisionGroup subsystems to their initial state
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
    m_xAxis = EGG::Vector3f(m_pose[0, 0], m_pose[1, 0], m_pose[2, 0]);
    m_yAxis = EGG::Vector3f(m_pose[0, 1], m_pose[1, 1], m_pose[2, 1]);
    m_zAxis = EGG::Vector3f(m_pose[0, 2], m_pose[1, 2], m_pose[2, 2]);
    m_pos = m_dynamics->pos();
    m_velocity = m_dynamics->velocity();
}

/// @addr{0x805A0340}
/// @brief Constructs a transformation matrix from rotation and position
void KartPhysics::updatePose() {
    m_pose.makeQT(m_dynamics->fullRot(), m_dynamics->pos());
    m_xAxis = EGG::Vector3f(m_pose[0, 0], m_pose[1, 0], m_pose[2, 0]);
    m_yAxis = EGG::Vector3f(m_pose[0, 1], m_pose[1, 1], m_pose[2, 1]);
    m_zAxis = EGG::Vector3f(m_pose[0, 2], m_pose[1, 2], m_pose[2, 2]);
}

/// @addr{0x8059F968}
/// @brief Computes trick and correction rotation and calls to KartDynamics::calc().
/// @param dt delta time step (always 1.0f)
/// @param maxSpeed 120.0f, unless we're in a bullet (145.0f)
/// @param air Whether we're touching ground
void KartPhysics::calc(f32 dt, f32 maxSpeed, const EGG::Vector3f &scale, bool air) {
    m_stuntRot = m_instantaneousStuntRot * m_decayingStuntRot;
    m_extraRot = m_instantaneousExtraRot * m_decayingExtraRot;

    m_dynamics->setSpecialRot(m_stuntRot);
    m_dynamics->setExtraRot(m_extraRot);
    m_dynamics->setScale(scale);

    m_dynamics->calc(dt, maxSpeed, air);

    m_decayingStuntRot = m_decayingStuntRot.slerpTo(EGG::Quatf::ident, 0.1f);
    m_decayingExtraRot = m_decayingExtraRot.slerpTo(EGG::Quatf::ident, 0.1f);

    m_instantaneousStuntRot = EGG::Quatf::ident;
    m_instantaneousExtraRot = EGG::Quatf::ident;
}

/// @addr{0x805A01CC}
/// @brief Adds velocity to the moving road velocity and clamps it to the provided maximum speed
/// @param v The velocity to add
/// @param maxPullSpeed The maximum speed to clamp the moving road velocity to
void KartPhysics::shiftDecayMovingRoadVel(const EGG::Vector3f &v, f32 maxPullSpeed) {
    m_movingRoadVel += v;

    if (m_movingRoadVel.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        f32 speed = std::min(maxPullSpeed, m_movingRoadVel.normalise());
        m_movingRoadVel *= speed;
        m_dynamics->setMovingRoadVel(m_movingRoadVel);
    }
}

/// @addr{0x805A04A0}
/// @brief Factory function that constructs a @ref KartPhysics object based on the provided @ref
/// KartParam and does preliminary initialization for its @ref KartDynamics and @ref CollisionGroup
/// subsystems
KartPhysics *KartPhysics::Create(const KartParam &param) {
    KartPhysics *physics = EGG::egg_new<KartPhysics>(param.isBike());

    const BSP &bsp = param.bsp();

    physics->setHalfLength(physics->hitboxGroup()->initHitboxes(bsp.hitboxes));

    physics->dynamics()->setBspParams(bsp.angVel0Factor, bsp.cuboids[0], bsp.cuboids[1], false);

    return physics;
}

} // namespace Kinoko::Kart
