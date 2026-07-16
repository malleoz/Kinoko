#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Base class for the cataquacks on GCN Peach Beach
class ObjectPoihanaBase : public ObjectCollidable, virtual public StateManager {
public:
    ObjectPoihanaBase(const System::MapdataGeoObj &params);
    ~ObjectPoihanaBase() override;

    void init() override;

    /// @addr{0x807472F4}
    void calc() override {
        StateManager::calc();
    }

    /// @addr{0x8074815C}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

protected:
    /// @brief Whether the cataquack has a target position to walk towards or needs one
    enum class WalkState {
        NeedTarget = 0, ///< The cataquack needs to select a new target position to walk towards
        HasTarget = 1,  ///< The cataquack has a target position to walk towards
    };

    WalkState m_walkState;     ///< Whether the cataquack has a target to walk towards or needs one
    EGG::Vector3f m_up;        ///< Smoothed up vector for the cataquack's orientation
    EGG::Vector3f m_forward;   ///< Smoothed forward vector for the cataquack's orientation
    EGG::Matrix34f m_workMat;  ///< The current frame's transformation matrix
    EGG::Vector3f m_accel;     ///< Acceleration vector
    EGG::Vector3f m_extVel;    ///< Velocity incurred due to acceleration
    EGG::Vector3f m_vel;       ///< Smoothed velocity vector based on the target velocity
    EGG::Vector3f m_floorNrm;  ///< The normal of the floor surface the cataquack is currently on
    f32 m_heightOffset;        ///< Accounts for cataquack leg height
    f32 m_radius;              ///< Radius of the cataquack's collision sphere
    EGG::Vector3f m_targetPos; ///< The position the cataquack is currently walking towards
    EGG::Vector3f m_dir;       ///< Direction between the current position and the target position
    EGG::Vector3f m_initPos;   ///< Position of the cataquack during initialization
    EGG::Vector3f m_targetVel; ///< Derived from m_targetPos and current position
};

/// @brief Represents the cataquacks on GCN Peach Beach
/// @details Cataquacks walk towards a pseudo-random target position within a radius of their
/// initial position. Once within a threshold of 200 units from the target position, the cataquack
/// will compute a new target position.
class ObjectPoihana final : public ObjectPoihanaBase {
public:
    ObjectPoihana(const System::MapdataGeoObj &params);
    ~ObjectPoihana() override;

    void init() override;

    /// @addr{0x80748B70}
    void calc() override {
        ObjectPoihanaBase::calc();
    }

private:
    /// @addr{0x807498AC}
    /// @brief Runs when the cataquack enters its walking state (on race state for Time Trials)
    void enterWalk() {
        m_walkState = WalkState::NeedTarget;
    }

    /// @addr{0x80749924}
    /// @brief Runs every frame the cataquack is in its walking state (every frame in Time Trials)
    void calcWalk() {
        calcStep();
        calcPosAndCollision();
        calcOrthonormalBasis();
        setTransform(m_workMat);
    }

    /// @addr{0x80747308}
    /// @brief Fetches the cataquack's position this frame based on the current matrix
    [[nodiscard]] EGG::Vector3f curPos() const {
        return m_workMat.base(3);
    }

    void calcUp(f32 t);
    void calcOrthonormalBasis();
    void calcCollision();
    void calcStep();
    void calcDir();
    void calcForward();

    /// @addr{0x80747324}
    /// @brief Computes the position to be used for floor collision checks
    [[nodiscard]] EGG::Vector3f collisionPos() const {
        return curPos() + m_floorNrm * m_heightOffset;
    }

    /// @addr{0x807476D8}
    /// @brief Updates velocity and position, and checks for floor collision
    void calcPosAndCollision() {
        m_extVel += m_accel;
        m_workMat.setBase(3, m_workMat.base(3) + (m_extVel + m_vel));
        calcCollision();
    }

    /// @addr{0x8074B0AC}
    /// @brief Computes the vector projection of v0 onto v1
    [[nodiscard]] static EGG::Vector3f Project(const EGG::Vector3f &v0, const EGG::Vector3f &v1) {
        f32 scale = (v0.z * v1.z + (v0.x * v1.x + v0.y * v1.y)) /
                EGG::Mathf::fma(v1.z, v1.z, v1.x * v1.x + v1.y * v1.y);

        return v1 * scale;
    }

    static constexpr std::array<StateManagerEntry, 1> STATE_ENTRIES = {{
            {StateEntry<ObjectPoihana, &ObjectPoihana::enterWalk, &ObjectPoihana::calcWalk>(0)},
    }};
};

} // namespace Kinoko::Field
