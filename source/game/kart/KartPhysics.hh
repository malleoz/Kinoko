#pragma once

#include "game/kart/CollisionGroup.hh"
#include "game/kart/KartDynamics.hh"

namespace Kinoko::Kart {

/// @brief The physics "controller" layer that sits between higher-level kart logic and the
/// lower-level @ref KartDynamics subsystem.
/// @details Owns the @ref KartDynamics subsystem and the kart's @ref CollisionGroup. Builds and
/// exposes the kart's pose matrix components. Handles rotation resulting from tricks and landings.
/// Also tracks moving object and moving road velocity.
class KartPhysics {
public:
    /// @addr{0x8059F5BC}
    /// @brief Constructor which initializes the @ref KartDynamics or @ref KartDynamicsBike
    /// subsystem depending on `isBike` and constructs the kart's @ref CollisionGroup
    /// @param isBike Whether the kart is a bike
    /// @details Constructs the @ref KartDynamics or @ref KartDynamicsBike subsystem based on
    /// whether the kart is a bike and also constructs the kart's @ref CollisionGroup subsystem.
    /// Initializes @ref m_pose to the identity matrix and sets @ref m_halfLength to `50.0f`.
    KartPhysics(bool isBike)
        : m_dynamics(isBike ? EGG::egg_new<KartDynamicsBike>() : EGG::egg_new<KartDynamics>()),
          m_hitboxGroup(EGG::egg_new<CollisionGroup>()) {
        m_pose = EGG::Matrix34f::ident;
        m_halfLength = 50.0f; // set immediately after in KartPhysics::Create()
    }

    /// @addr{0x8059F6F8}
    /// @brief Destructor that destroys the @ref KartDynamics and @ref CollisionGroup subsystem
    ~KartPhysics() {
        EGG::egg_delete(m_dynamics);
        EGG::egg_delete(m_hitboxGroup);
    }

    void reset();

    /// @addr{0x805A0340}
    /// @brief Constructs a transformation matrix from rotation and position
    /// @details Recomputes @ref m_pose based on the kart's @ref KartDynamics::m_fullRot and @ref
    /// KartDynamics::m_pos. Updates @ref m_xAxis, @ref m_yAxis, and @ref m_zAxis based on the new
    /// pose.
    void updatePose() {
        m_pose.makeQT(m_dynamics->fullRot(), m_dynamics->pos());
        m_xAxis = m_pose.base(0);
        m_yAxis = m_pose.base(1);
        m_zAxis = m_pose.base(2);
    }

    void calc(f32 dt, f32 maxSpeed, const EGG::Vector3f &scale, bool air);

    /// @beginSetters

    /// @brief Sets the position of the kart on the last frame
    /// @param pos The position to set for the kart on the last frame
    void setPos(const EGG::Vector3f &pos) {
        m_pos = pos;
    }

    /// @brief Sets the velocity of the kart on the last frame
    /// @param vel The velocity to set for the kart on the last frame
    void setVelocity(const EGG::Vector3f &vel) {
        m_velocity = vel;
    }

    /// @brief Sets the half-length of the kart's collision box
    /// @param val The half-length to set for the kart's collision box
    void setHalfLength(f32 val) {
        m_halfLength = val;
    }

    /// @endSetters

    /// @addr{0x8059FC48}
    /// @brief Sets a rotation resulting from a trick that is applied for only the current frame
    /// @param rot The rotation to apply
    /// @details The provided `rot` is composed into @ref m_instantaneousStuntRot.
    void composeStuntRot(const EGG::Quatf &rot) {
        m_instantaneousStuntRot *= rot;
    }

    /// @addr{0x8059FD0C}
    /// @brief Sets a corrective rotation that is applied for only the current frame, such as when
    /// in a spinout action, in a launch action, or learning in an automatic drift
    /// @param rot The rotation to apply
    /// @details The provided `rot` is composed into @ref m_instantaneousExtraRot.
    void composeExtraRot(const EGG::Quatf &rot) {
        m_instantaneousExtraRot *= rot;
    }

    /// @addr{0x8059FDD0}
    /// @brief Sets a rotation resulting from a trick that decays 10% per frame, such as when
    /// landing from a zipper trick
    /// @param rot The rotation to apply
    /// @details The provided `rot` is composed into @ref m_decayingStuntRot.
    void composeDecayingStuntRot(const EGG::Quatf &rot) {
        m_decayingStuntRot *= rot;
    }

    /// @addr{0x8059FE94}
    /// @brief Sets a corrective rotation that decays 10% per frame, such as when landing from a
    /// launch action
    /// @param rot The rotation to apply
    /// @details The provided `rot` is composed into @ref m_decayingExtraRot.
    void composeDecayingExtraRot(const EGG::Quatf &rot) {
        m_decayingExtraRot *= rot;
    }

    /// @addr{0x805A0050}
    /// @brief Linearly interpolates the moving object velocity towards the provided velocity
    /// @param vel The target velocity
    /// @param t The interpolation rate
    /// @details Interpolates @ref m_movingObjVel towards the provided `vel` using the interpolation
    /// rate `t`. Saves the velocity to @ref KartDynamics::m_movingObjVel.
    void composeMovingObjVel(const EGG::Vector3f &vel, f32 t) {
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
    void composeDecayingMovingObjVel(f32 floorScalar, f32 airScalar, bool floor) {
        m_movingObjVel *= floor ? floorScalar : airScalar;
        m_dynamics->setMovingObjVel(m_movingObjVel);
    }

    /// @addr{0x805A014C}
    /// @brief Linearly interpolates the moving road velocity towards the provided velocity
    /// @param vel The target velocity
    /// @param t The interpolation factor
    /// @details Interpolates @ref m_movingRoadVel towards the provided `vel` using the
    /// interpolation factor `t`. Saves the velocity to @ref KartDynamics::m_movingRoadVel.
    void composeMovingRoadVel(const EGG::Vector3f &vel, f32 t) {
        m_movingRoadVel += (vel - m_movingRoadVel) * t;
        m_dynamics->setMovingRoadVel(m_movingRoadVel);
    }

    void shiftDecayMovingRoadVel(const EGG::Vector3f &v, f32 maxPullSpeed);

    /// @addr{0x805A02B8}
    /// @brief Decays the moving road velocity by the provided scalars
    /// @param floorScalar The decay factor to apply when on the ground
    /// @param airScalar The decay factor to apply when in the air
    /// @param floor Whether the kart is on the ground
    /// @details Decays @ref m_movingRoadVel by the appropriate scalar depending on whether the kart
    /// is on the ground or in the air. Sets the y-component of @ref m_movingRoadVel to 0 and saves
    /// the velocity to @ref KartDynamics::m_movingRoadVel.
    void decayMovingRoadVel(f32 floorScalar, f32 airScalar, bool floor) {
        m_movingRoadVel *= floor ? floorScalar : airScalar;
        m_movingRoadVel.y = 0.0f;
        m_dynamics->setMovingRoadVel(m_movingRoadVel);
    }

    /// @addr{0x805A0410}
    /// @brief Clears the decaying stunt and extra rotations
    /// @details Sets @ref m_decayingStuntRot and @ref m_decayingExtraRot to the identity
    /// quaternion.
    void clearDecayingRot() {
        m_decayingStuntRot = EGG::Quatf::ident;
        m_decayingExtraRot = EGG::Quatf::ident;
    }

    /// @beginGetters

    /// @brief Gets the underlying @ref KartDynamics subsystem
    /// @return A pointer to the underlying @ref KartDynamics subsystem
    [[nodiscard]] KartDynamics *dynamics() {
        return m_dynamics;
    }

    /// @brief Gets the underlying @ref KartDynamics subsystem
    /// @return A const pointer to the underlying @ref KartDynamics subsystem
    [[nodiscard]] const KartDynamics *dynamics() const {
        return m_dynamics;
    }

    /// @brief Gets the pose matrix of the kart
    /// @return A const reference to the pose matrix of the kart
    [[nodiscard]] const EGG::Matrix34f &pose() const {
        return m_pose;
    }

    /// @brief Gets the collision group of the kart
    /// @return A pointer to the collision group of the kart
    [[nodiscard]] CollisionGroup *hitboxGroup() {
        return m_hitboxGroup;
    }

    /// @brief Gets the x-axis pose of the kart
    /// @return A const reference to the x-axis of the kart
    [[nodiscard]] const EGG::Vector3f &xAxis() const {
        return m_xAxis;
    }

    /// @brief Gets the y-axis pose of the kart
    /// @return A const reference to the y-axis of the kart
    [[nodiscard]] const EGG::Vector3f &yAxis() const {
        return m_yAxis;
    }

    /// @brief Gets the z-axis pose of the kart
    /// @return A const reference to the z-axis of the kart
    [[nodiscard]] const EGG::Vector3f &zAxis() const {
        return m_zAxis;
    }

    /// @brief Gets the last frame's finalized position of the kart
    /// @return A const reference to the kart finalized position on the last frame
    [[nodiscard]] const EGG::Vector3f &pos() const {
        return m_pos;
    }

    /// @brief Gets half of the kart's largest Z-axis hitbox extent
    /// @return The half length of the kart's largest Z-axis hitbox extent
    [[nodiscard]] f32 halfLength() const {
        return m_halfLength;
    }

    /// @endGetters

    /// @addr{0x805A04A0}
    /// @brief Factory function that constructs a @ref KartPhysics object
    /// @param param The kart parameters used to initialize the physics object
    /// @return A pointer to the newly created @ref KartPhysics object
    /// @details Initializes the hitboxes of the kart's @ref CollisionGroup based on the hitboxes
    /// defined in the @ref BSP from the provided `param`. Sets @ref KartPhysics::m_halfLength based
    /// on the initialized hitboxes, and saves some @ref BSP params via @ref
    /// KartDynamics::setBspParams().
    [[nodiscard]] static KartPhysics *Create(const KartParam &param) {
        KartPhysics *physics = EGG::egg_new<KartPhysics>(param.isBike());
        const BSP &bsp = param.bsp();
        physics->setHalfLength(physics->hitboxGroup()->initHitboxes(bsp.hitboxes));
        physics->dynamics()->setBspParams(bsp.angVel0Factor, bsp.cuboids[0], bsp.cuboids[1], false);

        return physics;
    }

private:
    KartDynamics *const m_dynamics;      ///< Pointer to the @ref KartDynamics subsystem
    CollisionGroup *const m_hitboxGroup; ///< Pointer to the @ref CollisionGroup subsystem
    EGG::Vector3f m_pos;                 ///< The last frame's finalized position of the kart
    EGG::Quatf m_decayingStuntRot;       ///< Trick rotation that decays 10% per frame
    EGG::Quatf m_instantaneousStuntRot;  ///< Trick rotation applied only for the current frame
    EGG::Quatf m_stuntRot;               ///< Combined stunt rotations
    EGG::Quatf m_decayingExtraRot;       ///< Corrective rotation that decays 10% per frame
    EGG::Quatf m_instantaneousExtraRot;  ///< Corrective rotation applied only for the current frame
    EGG::Quatf m_extraRot;               ///< Combined corrective rotations
    EGG::Vector3f m_movingObjVel;        ///< Velocity of moving objects that the kart is driving on
    EGG::Vector3f m_movingRoadVel;       ///< Velocity of moving roads that the kart is driving on
    EGG::Matrix34f m_pose;               ///< The kart's current rotation and position
    EGG::Vector3f m_xAxis;               ///< The first column of the pose
    EGG::Vector3f m_yAxis;               ///< The second column of the pose
    EGG::Vector3f m_zAxis;               ///< The third column of the pose
    EGG::Vector3f m_velocity;            ///< The last frame's finalized velocity of the kart
    f32 m_halfLength;                    ///< Half of the kart's largest Z-axis hitbox extent
};

} // namespace Kinoko::Kart
