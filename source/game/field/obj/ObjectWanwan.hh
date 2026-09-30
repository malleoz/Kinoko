#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The wooden stake that the chain chomp is chained to
/// @details The stake is a primitive @ref ObjectCollidable that acts as a wall. @ref ObjectWanwan
/// references the stake's position to keep the Chain Chomp tethered to the stake.
class ObjectWanwanPile final : public ObjectCollidable {
public:
    /// @addr{Inlined in 0x806E4224}
    /// @brief Constructor
    /// @param pos The initial position of the stake
    /// @param rot The initial rotation of the stake
    /// @param scale The initial scale of the stake
    ObjectWanwanPile(const EGG::Vector3f &pos, const EGG::Vector3f &rot, const EGG::Vector3f &scale)
        : ObjectCollidable("pile", pos, rot, scale) {}

    /// @addr{0x806E9568}
    /// @brief Default virtual destructor
    ~ObjectWanwanPile() override = default;

    /// @addr{0x806E9560}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that the object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806E9554}
    /// @copybrief ObjectBase::getResources()
    /// @details Returns the resource name for the Chain Chomp.
    /// @return The resource name for the Chain Chomp, `wanwan`.
    [[nodiscard]] const char *getResources() const override {
        return "wanwan";
    }

    /// @addr{0x806E9548}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the stake, `pile`.
    [[nodiscard]] const char *getKclName() const override {
        return "pile";
    }
};

/// @brief Represents Chain Chomps chained to a stake that attacks in a limited arc
/// @details Cycles between three different states:
/// - In the wait state, the Chain Chomp randomly wanders within a bounded region, periodically
/// changing targets
/// - In the attack state, the Chain Chomp lunges towards its target until its chain is taut
/// - In the back state, the Chain Chomp retreats back towards its anchor (the wooden stake)
/// @desync @ref ObjectDirector::s_wanwanMaxPitch is used to describe the maximum pitch of the Chain
/// Chomp. In the base game, this value is statically initialized to -30.0f and is only overwritten
/// if you load GCN Mario Circuit, at which point it is changed to -20.0f. If you then load into
/// Mario Circuit Wii, it will continue to be set to the overwritten value of -20.0f. This means
/// that the Chain Chomp's behavior will desync if GCN Mario Circuit is loaded before playing Mario
/// Circuit Wii. To compensate for this, we always set the max pitch in @ref
/// ObjectDirector::createObjects().
class ObjectWanwan final : public ObjectCollidable, private StateManager {
public:
    ObjectWanwan(const System::MapdataGeoObj &params);

    /// @addr{0x806E4AEC}
    /// @brief Default virtual destructor
    ~ObjectWanwan() override = default;

    void init() override;
    void calc() override;

    /// @addr{0x806E94BC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags().setBit(eLoadFlags::Calc, eLoadFlags::Draw);
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    /// @brief Helper function that handles the case when the param's wander duration is zero
    /// @param duration The duration specified by the param settings
    /// @return `duration` if it is non-zero, otherwise `300.0f`
    [[nodiscard]] f32 initWanderDuration(f32 duration) const {
        if (duration == 0.0f) {
            duration = 300.0f;
        }
        return duration;
    }

    /// @brief Helper function that handles the case when the param's attack arc is zero
    /// @param arc The attack arc specified by the param settings
    /// @return `arc` if it is non-zero, otherwise `30.0f`
    [[nodiscard]] f32 initAttackArc(f32 arc) const {
        if (arc == 0.0f) {
            arc = 30.0f;
        }
        return arc;
    }

    void enterWait();
    void enterAttack();
    void enterBack();

    void calcWait();
    void calcAttack();
    void calcBack();

    /// @addr{0x806E59BC}
    /// @brief Applies acceleration to velocity and adds velocity to the position
    /// @details Updates @ref m_vel based on @ref m_accel and applies a downward gravitational force
    /// of @ref GRAVITY. Updates the Chain Chomp's position accordingly and resets @ref m_accel to
    /// zero.
    void calcPos() {
        m_vel += m_accel - GRAVITY;
        addPos(m_vel);
        m_accel.setZero();
    }

    void calcCollision();
    void calcMat();
    void calcChainAttachMat();
    void calcSpeed();

    /// @addr{0x806E794C}
    /// @brief Checks if the Chain Chomp is touching the floor and applies a bounce if it is
    /// @details If the Chain Chomp is not touching the floor, then clears the Y-component of @ref
    /// m_accel. Otherwise, if the Chain Chomp is touching the floor, then clears the Y-component of
    /// @ref m_vel.y and applies an upward acceleration of `12.0f`.
    void calcBounce() {
        constexpr f32 BOUNCE_ACCEL = 12.0f;

        if (m_touchingFloor) {
            m_vel.y = 0.0f;
            m_accel += EGG::Vector3f::ey * BOUNCE_ACCEL;
        } else {
            m_accel.y = 0.0f;
        }
    }

    /// @addr{0x806E79E4}
    /// @brief Calculates the wandering behavior of the Chain Chomp
    /// @details In the base game, this behavior changes depending on whether we are in a race or a
    /// time trial. For Kinoko, we simply call @ref calcWanderTimeTrial().
    void calcWander() {
        calcWanderTimeTrial();
    }

    /// @addr{0x806E7B7C}
    /// @brief Calculates the wandering behavior of the Chain Chomp specifically for time trial mode
    /// @details Increments @ref m_wanderTimer and transitions the Chain Chomp to the attack state
    /// if the timer exceeds @ref m_wanderDuration.
    void calcWanderTimeTrial() {
        if (m_wanderTimer++ >= m_wanderDuration) {
            m_nextStateId = 1;
        }
    }

    /// @addr{0x806E7BA4}
    /// @brief Calculates the world position of the chain attachment point on the Chain Chomp
    /// @details Simply calls @ref calcChainAttachPos() with the current chain attachment matrix.
    void calcChain() {
        if (m_chainAttachMat.base(3).squaredLength() > std::numeric_limits<f32>::epsilon()) {
            calcChainAttachPos(m_chainAttachMat);
        }
    }

    /// @addr{0x806E7E38}
    /// @brief Interpolates the Chain Chomp's tangent vector towards the target direction
    /// @param interpRate The interpolation factor
    /// @details Updates @ref m_tangent by interpolating it towards @ref m_targetDir.
    void calcTangent(f32 interpRate) {
        m_tangent = Interpolate(interpRate, m_tangent, m_targetDir);
        if (m_tangent.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            m_tangent.normalise2();
        } else {
            m_tangent = m_targetDir;
        }
    }

    /// @addr{0x806E7F64}
    /// @brief Interpolates the Chain Chomp's up vector towards the target up direction
    /// @param interpRate The interpolation factor
    /// @details Updates @ref m_up by interpolating it towards @ref m_targetUp.
    void calcUp(f32 interpRate) {
        m_up = Interpolate(interpRate, m_up, m_targetUp);
        if (m_up.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            m_up.normalise2();
        } else {
            m_up = EGG::Vector3f::ey;
        }
    }

    void calcRandomTarget();
    void initTransformKeyframes();
    void calcAttackPos();

    /// @brief Calculates the world position of the chain attachment point on the Chain Chomp
    /// @param mat The world transform matrix of the Chain Chomp
    /// @details The attachment point is found by taking the chain chomp's position, subtracting a
    /// neck offset of `250.0f` from the Chain Chomp's forward direction, subtracting an additional
    /// offset of `140.0f` from the forward direction, and adding a vertical offset of `20.0f` to
    /// the up direction. Saves the result to @ref m_chainAttachPos.
    void calcChainAttachPos(EGG::Matrix34f mat) {
        constexpr f32 NECK_OFFSET = 250.0f;
        constexpr f32 CHAIN_BACK_OFFSET = 140.0f;
        constexpr f32 CHAIN_UP_OFFSET = 20.0f;

        EGG::Vector3f pos = mat.base(3);
        pos -= mat.base(2) * NECK_OFFSET * scale().x;
        mat.setBase(3, pos);

        EGG::Vector3f backOffset = mat.base(2) * CHAIN_BACK_OFFSET * scale().x;
        EGG::Vector3f verticalOffset = mat.base(1) * CHAIN_UP_OFFSET * scale().x;
        m_chainAttachPos = pos - backOffset + verticalOffset;
    }

    /// @addr{0x806B38A8}
    /// @brief Calculates the cross product of the XZ components of three vectors
    /// @param v0 The first vector
    /// @param v1 The second vector
    /// @param v2 The third vector
    /// @return The cross product of the XZ components of the three vectors
    [[nodiscard]] static f32 CrossXZ(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            const EGG::Vector3f &v2) {
        return (v2.x - v1.x) * (v0.z - v1.z) - (v0.x - v1.x) * (v2.z - v1.z);
    }

    /// @addr{0x806E8F4C}
    /// @brief Samples a Hermite interpolation between two values with the number of samples
    /// specified by the size of the destination span
    /// @param start The starting value of the interpolation
    /// @param end The ending value of the interpolation
    /// @param startTangent The tangent at the starting value
    /// @param endTangent The tangent at the ending value
    /// @param dst The destination span to store the sampled values
    static void SampleHermiteInterp(f32 start, f32 end, f32 startTangent, f32 endTangent,
            std::span<f32> dst) {
        dst.front() = start;
        dst.back() = end;

        f32 scalar = 1.0f / static_cast<f32>(dst.size() - 1);

        for (u8 i = 1; i < dst.size() - 1; ++i) {
            dst[i] = EGG::Mathf::Hermite(start, startTangent, end, endTangent,
                    scalar * static_cast<f32>(i));
        }
    }

    EGG::Vector3f m_vel;      ///< The current velocity of the Chain Chomp
    EGG::Vector3f m_accel;    ///< The current acceleration of the Chain Chomp
    f32 m_speed;              ///< The current speed of the Chain Chomp, only used in the back state
    f32 m_pitch;              ///< The current pitch of the Chain Chomp
    EGG::Vector3f m_tangent;  ///< Smoothed forward direction of the Chain Chomp
    EGG::Vector3f m_up;       ///< Smoothed upward direction of the Chain Chomp
    EGG::Vector3f m_targetUp; ///< Current floor normal
    std::array<EGG::Matrix34f, 15> m_transformKeyframes; ///< Chain attachment animation keyframes
    const f32 m_chainLength;                             ///< The length of the Chain Chomp's chain
    const f32 m_attackDistance;   ///< The distance at which the Chain Chomp will initiate an attack
    const f32 m_attackDirectionX; ///< X-axis center of the direction of attack
    const f32 m_attackDirectionZ; ///< Z-axis center of the direction of attack
    u32 m_chainCount;             ///< The number of chain segments attached to the Chain Chomp
    EGG::Vector3f m_chainAttachPos;        ///< Where the chain attaches to the Chain Chomp
    EGG::Vector3f m_initPos;               ///< The initial position of the Chain Chomp
    EGG::Vector3f m_anchor;                ///< Effectively the position of the wooden stake
    EGG::Vector3f m_wanderConstraintPoint; ///< Constrains the wandering area of the Chain Chomp
    bool m_touchingFloor;                  ///< True if the Chain Chomp is colliding with the floor
    bool m_chainTaut;          ///< Chain Chomp cannot move further because the chain is fully taut
    u32 m_frame;               ///< Counts up every frame
    EGG::Vector3f m_target;    ///< Where the chain chomp is currently moving towards
    EGG::Vector3f m_targetDir; ///< The direction towards the current target position
    bool m_retarget;           ///< Tracks whether the target position has changed in the wait state
    u32 m_wanderTimer;         ///< How long the chain chomp has been wandering for
    bool m_attackStill;        ///< True when lurched forward and stationary
    const u32 m_wanderDuration; ///< How long the Chain Chomp wanders before attacking
    const f32 m_attackArc;   ///< Half size of the arc the chain chomp can lurch within (in degrees)
    EGG::Vector3f m_backDir; ///< Direction the Chain Chomp moves when retreating

    /// @brief Represents the position and rotation of where the Chain Chomp attaches to the chain.
    /// @details This is normally housed in the DrawMdl's ScnObj, but for the purposes of time trial
    /// physics, it can be defined here for simplicity.
    EGG::Matrix34f m_chainAttachMat;

    /// @brief Scale of the Chain Chomp compared to, say, the DS Peach Gardens Chain Chomps
    static constexpr f32 SCALE = 2.0f;

    /// @brief Gravity vector applied to the Chain Chomp's acceleration
    static constexpr EGG::Vector3f GRAVITY = EGG::Vector3f(0.0f, 2.5f, 0.0f);

    /// @brief Length of a single chain segment, before scaling
    static constexpr f32 CHAIN_LENGTH = 135.0f;

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectWanwan, &ObjectWanwan::enterWait, &ObjectWanwan::calcWait>(0)},
            {StateEntry<ObjectWanwan, &ObjectWanwan::enterAttack, &ObjectWanwan::calcAttack>(1)},
            {StateEntry<ObjectWanwan, &ObjectWanwan::enterBack, &ObjectWanwan::calcBack>(2)},
    }};
};

} // namespace Kinoko::Field
