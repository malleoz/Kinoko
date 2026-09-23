#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Base class for the Cataquacks on GCN Peach Beach
/// @details It is not clear why this is separate from @ref ObjectPoihana, as no other class is
/// derived from it. We separate them in Kinoko in order to more closely match the base game's code
/// structure.
class ObjectPoihanaBase : public ObjectCollidable, virtual protected StateManager {
public:
    /// @addr{0x80747198}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Initializes @ref m_walkState to @ref WalkState::NeedTarget, @ref m_heightOffset to
    /// 0, @ref m_radius to `150.0f`, and @ref m_targetPos to the zero vector.
    ObjectPoihanaBase(const System::MapdataGeoObj &params)
        : StateManager(this, {}),
          ObjectCollidable(params),
          m_walkState(WalkState::NeedTarget),
          m_heightOffset(0.0f),
          m_radius(150.0f),
          m_targetPos(EGG::Vector3f::zero) {}

    /// @addr{0x80747208}
    /// @brief Default virtual destructor
    ~ObjectPoihanaBase() override = default;

    void init() override;

    /// @addr{0x807472F4}
    /// @copybrief ObjectBase::calc()
    /// @details Simply evaluates the Cataquack's state machine.
    void calc() override {
        StateManager::calc();
    }

    /// @addr{0x8074815C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
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
/// @details Cataquacks walk towards a pseudo-random target position within 1000 to 2000 units of
/// their initial position. Once within a threshold of 200 units from the target position, the
/// cataquack will compute a new target position.
class ObjectPoihana final : public ObjectPoihanaBase {
public:
    /// @addr{0x8074816C}
    /// @copydoc ObjectPoihanaBase::ObjectPoihanaBase(const System::MapdataGeoObj &)
    ObjectPoihana(const System::MapdataGeoObj &params)
        : StateManager(this, STATE_ENTRIES),
          ObjectPoihanaBase(params) {}

    /// @addr{0x807488BC}
    /// @brief Default virtual destructor
    ~ObjectPoihana() override = default;

    void init() override;

private:
    /// @addr{0x807498AC}
    /// @brief Runs when the cataquack enters its walking state (on race state for Time Trials)
    /// @details Sets @ref m_walkState to @ref WalkState::NeedTarget.
    void enterWalk() {
        m_walkState = WalkState::NeedTarget;
    }

    /// @addr{0x80749924}
    /// @brief Runs every frame the cataquack is in its walking state (every frame in Time Trials)
    /// @details Calculates the Cataquack's direction and velocity, including setting a target
    /// position if needed. Applies acceleration and velocity to the Cataquack's @ref m_workMat.
    /// Orients the Cataquack based on its @ref m_up vector. Finally, applies the staged @ref
    /// m_workMat to the Cataquack's transform.
    void calcWalk() {
        calcStep();
        calcPosAndCollision();
        calcOrthonormalBasis();
        setTransform(m_workMat);
    }

    /// @addr{0x80747308}
    /// @brief Fetches the cataquack's position this frame based on @ref m_workMat
    /// @return The staged current position of the Cataquack.
    [[nodiscard]] EGG::Vector3f curPos() const {
        return m_workMat.base(3);
    }

    /// @addr{0x80747530}
    /// @brief Interpolates @ref m_up towards @ref m_floorNrm
    /// @param t The interpolation factor used to blend between the current up vector and the floor
    /// normal.
    void calcUp(f32 t) {
        m_up = Interpolate(t, m_up, m_floorNrm);
        if (m_up.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            m_up.normalise2();
        } else {
            m_up = EGG::Vector3f::ey;
        }
    }

    void calcOrthonormalBasis();
    void calcCollision();
    void calcStep();

    /// @addr{0x80749610}
    /// @brief Calculates the XZ direction vector between the current position and the target
    /// position.
    void calcDir() {
        EGG::Vector3f delta = m_targetPos - curPos();
        delta.y = 0.0f;
        if (delta.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            delta.normalise2();
            m_dir = delta;
        }
    }

    /// @addr{0x807496C4}
    /// @brief Calculates the smoothed forward vector based on @ref m_dir and the staged forward
    /// direction in @ref m_workMat
    void calcForward() {
        constexpr f32 INTERP_RATE = 0.1f;

        EGG::Vector3f forward = Interpolate(INTERP_RATE, m_workMat.base(2), m_dir);
        if (forward.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            forward.normalise2();
        } else {
            forward = EGG::Vector3f::ez;
        }

        m_forward = forward;
    }

    /// @addr{0x80747324}
    /// @brief Computes the position to be used for floor collision checks
    /// @return The position to be used for floor collision checks, offset by @ref m_floorNrm and
    /// @ref m_heightOffset.
    [[nodiscard]] EGG::Vector3f collisionPos() const {
        return curPos() + m_floorNrm * m_heightOffset;
    }

    /// @addr{0x807476D8}
    /// @brief Updates velocity and position, and checks for floor collision
    /// @details Applies @ref m_accel to @ref m_extVel. Computes the Cataquack's updated position by
    /// summing @ref m_extVel and @ref m_vel with the position staged in @ref m_workMat. Finally,
    /// calls @ref calcCollision() to check for collisions with floors and walls.
    void calcPosAndCollision() {
        m_extVel += m_accel;
        m_workMat.setBase(3, m_workMat.base(3) + (m_extVel + m_vel));
        calcCollision();
    }

    /// @addr{0x8074B0AC}
    /// @brief Computes the vector projection of v0 onto v1
    /// @param v0 The vector to be projected.
    /// @param v1 The vector onto which v0 is projected.
    /// @return The projection of v0 onto v1.
    [[nodiscard]] static EGG::Vector3f Project(const EGG::Vector3f &v0, const EGG::Vector3f &v1) {
        f32 scale = (v0.z * v1.z + (v0.x * v1.x + v0.y * v1.y)) /
                EGG::Mathf::fma(v1.z, v1.z, v1.x * v1.x + v1.y * v1.y);

        return v1 * scale;
    }

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 1> STATE_ENTRIES = {{
            {StateEntry<ObjectPoihana, &ObjectPoihana::enterWalk, &ObjectPoihana::calcWalk>(0)},
    }};
};

} // namespace Kinoko::Field
