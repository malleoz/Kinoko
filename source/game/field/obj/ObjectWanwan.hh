#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The wooden stake that the chain chomp is chained to
class ObjectWanwanPile final : public ObjectCollidable {
public:
    /// @addr{Inlined in 0x806E4224}
    ObjectWanwanPile(const EGG::Vector3f &pos, const EGG::Vector3f &rot, const EGG::Vector3f &scale)
        : ObjectCollidable("pile", pos, rot, scale) {}

    /// @addr{0x806E9568}
    ~ObjectWanwanPile() override = default;

    /// @addr{0x806E9560}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806E9554}
    [[nodiscard]] const char *getResources() const override {
        return "wanwan";
    }

    /// @addr{0x806E9548}
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
class ObjectWanwan final : public ObjectCollidable, public StateManager {
public:
    ObjectWanwan(const System::MapdataGeoObj &params);
    ~ObjectWanwan() override;

    void init() override;
    void calc() override;

    /// @addr{0x806E94BC}
    [[nodiscard]] u32 loadFlags() const override {
        return 3;
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    void enterWait();
    void enterAttack();
    void enterBack();

    void calcWait();
    void calcAttack();
    void calcBack();

    void calcPos();
    void calcCollision();
    void calcMat();
    void calcChainAttachMat();
    void calcSpeed();
    void calcBounce();

    /// @addr{0x806E79E4}
    /// @brief Calculates the wandering behavior of the Chain Chomp
    void calcWander() {
        calcWanderTimeTrial();
    }

    /// @addr{0x806E7B7C}
    /// @brief Calculates the wandering behavior of the Chain Chomp specifically for time trial mode
    void calcWanderTimeTrial() {
        if (m_wanderTimer++ >= m_wanderDuration) {
            m_nextStateId = 1;
        }
    }

    /// @addr{0x806E7BA4}
    /// @brief Calculates the world position of the chain attachment point on the Chain Chomp
    void calcChain() {
        if (m_chainAttachMat.base(3).squaredLength() > std::numeric_limits<f32>::epsilon()) {
            calcChainAttachPos(m_chainAttachMat);
        }
    }

    void calcTangent(f32 t);
    void calcUp(f32 t);
    void calcRandomTarget();
    void initTransformKeyframes();
    void calcAttackPos();
    void calcChainAttachPos(EGG::Matrix34f mat);

    /// @addr{0x806B38A8}
    /// @brief Calculates the cross product of the XZ components of three vectors
    [[nodiscard]] static f32 CrossXZ(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            const EGG::Vector3f &v2) {
        return (v2.x - v1.x) * (v0.z - v1.z) - (v0.x - v1.x) * (v2.z - v1.z);
    }

    static void SampleHermiteInterp(f32 start, f32 end, f32 startTangent, f32 endTangent,
            std::span<f32> dst);

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
    u32 m_wanderDuration;      ///< How long the Chain Chomp wanders before attacking
    f32 m_attackArc; ///< Half of size of the arc the chain chomp can lurch within (in degrees)
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

    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectWanwan, &ObjectWanwan::enterWait, &ObjectWanwan::calcWait>(0)},
            {StateEntry<ObjectWanwan, &ObjectWanwan::enterAttack, &ObjectWanwan::calcAttack>(1)},
            {StateEntry<ObjectWanwan, &ObjectWanwan::enterBack, &ObjectWanwan::calcBack>(2)},
    }};
};

} // namespace Kinoko::Field
