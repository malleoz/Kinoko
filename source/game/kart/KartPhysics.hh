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
    KartPhysics(bool isBike);
    ~KartPhysics();

    void reset();
    void updatePose();

    void calc(f32 dt, f32 maxSpeed, const EGG::Vector3f &scale, bool air);

    /// @beginSetters
    void setPos(const EGG::Vector3f &pos) {
        m_pos = pos;
    }

    void setVelocity(const EGG::Vector3f &vel) {
        m_velocity = vel;
    }

    void setHalfLength(f32 val) {
        m_halfLength = val;
    }
    /// @endSetters

    /// @addr{0x8059FC48}
    /// @brief Sets a rotation resulting from a trick that is applied for only the current frame
    /// @param rot The rotation to apply
    void composeStuntRot(const EGG::Quatf &rot) {
        m_instantaneousStuntRot *= rot;
    }

    /// @addr{0x8059FD0C}
    /// @brief Sets a corrective rotation that is applied for only the current frame, such as when
    /// in a spinout action, in a launch action, or learning in an automatic drift
    /// @param rot The rotation to apply
    void composeExtraRot(const EGG::Quatf &rot) {
        m_instantaneousExtraRot *= rot;
    }

    /// @addr{0x8059FDD0}
    /// @brief Sets a rotation resulting from a trick that decays 10% per frame, such as when
    /// landing from a zipper trick
    /// @param rot The rotation to apply
    void composeDecayingStuntRot(const EGG::Quatf &rot) {
        m_decayingStuntRot *= rot;
    }

    /// @addr{0x8059FE94}
    /// @brief Sets a corrective rotation that decays 10% per frame, such as when landing from a
    /// launch action
    /// @param rot The rotation to apply
    void composeDecayingExtraRot(const EGG::Quatf &rot) {
        m_decayingExtraRot *= rot;
    }

    /// @addr{0x805A0050}
    /// @brief Linearly interpolates the moving object velocity towards the provided velocity
    /// @param vel The target velocity
    /// @param t The interpolation factor
    void composeMovingObjVel(const EGG::Vector3f &vel, f32 t) {
        m_movingObjVel += (vel - m_movingObjVel) * t;
        m_dynamics->setMovingObjVel(m_movingObjVel);
    }

    /// @addr{0x805A00D0}
    /// @brief Decays the moving object velocity by the provided scalars
    /// @param floorScalar The decay factor to apply when on the ground
    /// @param airScalar The decay factor to apply when in the air
    /// @param floor Whether the kart is on the ground
    void composeDecayingMovingObjVel(f32 floorScalar, f32 airScalar, bool floor) {
        m_movingObjVel *= floor ? floorScalar : airScalar;
        m_dynamics->setMovingObjVel(m_movingObjVel);
    }

    /// @addr{0x805A014C}
    /// @brief Linearly interpolates the moving road velocity towards the provided velocity
    /// @param vel The target velocity
    /// @param t The interpolation factor
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
    void decayMovingRoadVel(f32 floorScalar, f32 airScalar, bool floor) {
        m_movingRoadVel *= floor ? floorScalar : airScalar;
        m_movingRoadVel.y = 0.0f;
        m_dynamics->setMovingRoadVel(m_movingRoadVel);
    }

    /// @addr{0x805A0410}
    /// @brief Resets the decaying stunt and extra rotations to the identity quaternion
    void clearDecayingRot() {
        m_decayingStuntRot = EGG::Quatf::ident;
        m_decayingExtraRot = EGG::Quatf::ident;
    }

    /// @beginGetters
    [[nodiscard]] KartDynamics *dynamics() {
        return m_dynamics;
    }

    [[nodiscard]] const KartDynamics *dynamics() const {
        return m_dynamics;
    }

    [[nodiscard]] const EGG::Matrix34f &pose() const {
        return m_pose;
    }

    [[nodiscard]] CollisionGroup *hitboxGroup() {
        return m_hitboxGroup;
    }

    [[nodiscard]] const EGG::Vector3f &xAxis() const {
        return m_xAxis;
    }

    [[nodiscard]] const EGG::Vector3f &yAxis() const {
        return m_yAxis;
    }

    [[nodiscard]] const EGG::Vector3f &zAxis() const {
        return m_zAxis;
    }

    [[nodiscard]] const EGG::Vector3f &pos() const {
        return m_pos;
    }

    [[nodiscard]] f32 halfLength() const {
        return m_halfLength;
    }
    /// @endGetters

    [[nodiscard]] static KartPhysics *Create(const KartParam &param);

private:
    KartDynamics *m_dynamics;           ///< Pointer to the @ref KartDynamics subsystem
    CollisionGroup *m_hitboxGroup;      ///< Pointer to the @ref CollisionGroup subsystem
    EGG::Vector3f m_pos;                ///< The last frame's finalized position of the kart
    EGG::Quatf m_decayingStuntRot;      ///< Trick rotation that decays 10% per frame
    EGG::Quatf m_instantaneousStuntRot; ///< Trick rotation applied only for the current frame
    EGG::Quatf m_stuntRot;              ///< Combined decaying and instantaneous stunt rotations
    EGG::Quatf m_decayingExtraRot;      ///< Corrective rotation that decays 10% per frame
    EGG::Quatf m_instantaneousExtraRot; ///< Corrective rotation applied only for the current frame
    EGG::Quatf m_extraRot;         ///< Combined decaying and instantaneous corrective rotations
    EGG::Vector3f m_movingObjVel;  ///< The velocity of moving objects that the kart is driving on
    EGG::Vector3f m_movingRoadVel; ///< The velocity of moving roads that the kart is driving on
    EGG::Matrix34f m_pose;         ///< The kart's current rotation and position
    EGG::Vector3f m_xAxis;         ///< The first column of the pose
    EGG::Vector3f m_yAxis;         ///< The second column of the pose
    EGG::Vector3f m_zAxis;         ///< The third column of the pose
    EGG::Vector3f m_velocity;      ///< Copied from KartDynamics
    f32 m_halfLength;              ///< Half of the kart's largest Z-axis hitbox extent
};

} // namespace Kinoko::Kart
