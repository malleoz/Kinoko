#pragma once

#include <egg/math/Matrix.hh>

namespace Kinoko::Kart {

class CollisionGroup;
class KartDynamics;
class KartParam;

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

    void setPos(const EGG::Vector3f &pos);
    void setVelocity(const EGG::Vector3f &vel);
    void setHalfLength(f32 val);

    /// @endSetters

    void composeStuntRot(const EGG::Quatf &rot);
    void composeExtraRot(const EGG::Quatf &rot);
    void composeDecayingStuntRot(const EGG::Quatf &rot);
    void composeDecayingExtraRot(const EGG::Quatf &rot);
    void composeMovingObjVel(const EGG::Vector3f &vel, f32 t);
    void composeDecayingMovingObjVel(f32 floorScalar, f32 airScalar, bool floor);
    void composeMovingRoadVel(const EGG::Vector3f &vel, f32 t);
    void shiftDecayMovingRoadVel(const EGG::Vector3f &v, f32 maxPullSpeed);
    void decayMovingRoadVel(f32 floorScalar, f32 airScalar, bool floor);
    void clearDecayingRot();

    /// @beginGetters

    [[nodiscard]] KartDynamics *dynamics();
    [[nodiscard]] const KartDynamics *dynamics() const;
    [[nodiscard]] const EGG::Matrix34f &pose() const;
    [[nodiscard]] CollisionGroup *hitboxGroup();
    [[nodiscard]] const EGG::Vector3f &xAxis() const;
    [[nodiscard]] const EGG::Vector3f &yAxis() const;
    [[nodiscard]] const EGG::Vector3f &zAxis() const;
    [[nodiscard]] const EGG::Vector3f &pos() const;
    [[nodiscard]] f32 halfLength() const;

    /// @endGetters

    [[nodiscard]] static KartPhysics *Create(const KartParam &param);

private:
    KartDynamics *const m_dynamics;      ///< Pointer to the @ref KartDynamics subsystem
    CollisionGroup *const m_hitboxGroup; ///< Pointer to the @ref CollisionGroup subsystem
    EGG::Vector3f m_pos;                 ///< The last frame's finalized position of the kart
    EGG::Quatf m_decayingStuntRot;       ///< Trick rotation that decays 10% per frame
    EGG::Quatf m_instantaneousStuntRot;  ///< Trick rotation applied only for the current frame
    EGG::Quatf m_stuntRot;               ///< Combined stunt rotations
    EGG::Quatf m_decayingExtraRot;       ///< Corrective rotation that decays 10% per frame
    EGG::Quatf m_instantaneousExtraRot;  ///< Corrective rotation applied only for the
                                         ///< current frame
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
