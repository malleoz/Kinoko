#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectProjectile.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief Represents the smaller fireballs that trail behind a @ref ObjectFireSnake
class ObjectFireSnakeKid final : public ObjectCollidable {
public:
    /// @addr{0x806C0D18}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectFireSnakeKid(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x806C2ACC}
    /// @brief Default virtual destructor
    ~ObjectFireSnakeKid() override = default;

    /// @addr{0x806C2B60}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }
};

/// @brief Represents the bouncing fire snakes on DS Desert Hills and is the base class for the @ref
/// ObjectFireSnakeV objects on Grumble Volcano
/// @details Each firesnake has two children that follow behind. The children's position is set by
/// copying the transformation matrix of the parent snake from previous frames (10 frames ago for
/// the first child and 20 frames ago for the second child). @ref ObjectFireSnakeV manages its own
/// state lifecycle, whereas @ref ObjectFireSnake instances rely on @ref ObjectSunDS to advance the
/// state cycle by launching the fire snake and are registered as managed objects so that @ref
/// ObjectSunManager can fetch the FireSnakes.
class ObjectFireSnake : public ObjectProjectile, virtual protected StateManager {
public:
    ObjectFireSnake(const System::MapdataGeoObj &params);

    /// @addr{0x806C1284}
    /// @brief Default virtual destructor
    ~ObjectFireSnake() override = default;

    /// @addr{0x806C13B0}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the fire snake to the despawned state. Also caches the initial rotation
    /// and bounce direction of the fire snake.
    void init() override {
        m_nextStateId = 0;
        enterDespawned();

        m_spawnPos.setZero();

        calcTransform();
        m_initRot = transform().base(0);
        m_bounceDir = transform().base(0);
    }

    /// @addr{0x806C14E4}
    /// @copybrief ObjectBase::calc()
    /// @details Evaluates the fire snake's state machine. If the fire snake has exceeded its
    /// maximum age while in the active state, it transitions to the despawned state. Also updates
    /// the state of all children.
    void calc() override {
        StateManager::calc();

        if (isCollisionEnabled()) {
            if (++m_age >= m_maxAge && m_currentStateId == 3) {
                m_nextStateId = 0;
            }
        }

        calcChildren();
    }

    /// @addr{0x806C2A5C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    void initProjectile(const EGG::Vector3f &pos) override;

    /// @addr{0x806C29FC}
    /// @brief Callback function use by the @ref ObjectSunDS that wants to throw this projectile.
    /// @details Sets the next state to the falling state when the fire snake is launched.
    void onLaunch() override {
        m_nextStateId = 1;
    }

    /// @addr{0x806C1930}
    /// @brief Runs once when the fire snake despawns
    /// @details Resets the trajectory position to the spawn position and disables collision.
    void enterDespawned() {
        if (getUnit()) {
            unregisterCollision();
        }

        m_trajectoryPos = m_spawnPos;
        setPos(m_spawnPos);
    }

    /// @addr{0x806C19E8}
    /// @brief Runs once when the fire snake respawns
    /// @details Loads the collision bounding box if it hasn't been loaded yet.
    void enterFalling() {
        if (!getUnit()) {
            loadAABB(0.0f);
        }
    }

    /// @addr{0x806C1DCC}
    /// @brief Runs once after landing from the sun (for the case of @ref ObjectFireSnake), or runs
    /// once upon re-spawning (for the case of @ref ObjectFireSnakeV).
    /// @details Sets the trajectory position to the initial landing position, snaps the fire
    /// snake's position to the landing position, and determines the bounce direction with a 50%
    /// chance of flipping it.
    void enterHighBounce() {
        m_trajectoryPos = m_initPos;
        setPos(m_initPos);
        f32 rand = System::RaceManager::Instance()->random().getF32();
        m_bounceDir = rand >= 0.5f ? m_initRot : -m_initRot;
        m_age = 0;
    }

    void enterRest();

    void calcFalling();

    /// @addr{0x806C1E90}
    /// @brief Runs every frame during the first bounce
    /// @details Dispatches to calcBounce(float) with a 60% increase in the bounce velocity.
    void calcHighBounce() {
        constexpr f32 HIGH_BOUNCE_SCALAR = 1.6f;

        calcBounce(HIGH_BOUNCE_SCALAR * BOUNCE_VELOCITY);
    }

    void calcRest();

    /// @addr{0x806C2254}
    /// @brief Runs every frame the fire snake is bouncing, except the first bounce
    /// @details Dispatches to calcBounc(float) with a velocity of @ref BOUNCE_VELOCITY.
    void calcBounce() {
        calcBounce(BOUNCE_VELOCITY);
    }

protected:
    void calcChildren();

    EGG::Vector3f m_spawnPos;      ///< Starting position when a fire snake begins falling
    EGG::Vector3f m_initPos;       ///< Landing position after spawning
    EGG::Vector3f m_initRot;       ///< Initial rotation
    EGG::Vector3f m_trajectoryPos; ///< True position used for physics, not affected by spiral fall
    EGG::Vector3f m_bounceDir;     ///< Direction of current bounce
    u16 m_age;                     ///< How long the firesnake has been spawned
    u16 m_delayFrame;              ///< Initial delay before state lifecycle starts

private:
    void calcBounce(f32 initialVel);

    /// @brief Collision is enabled if the snake has landed from the spawner
    /// @return `true` if collision is enabled (when bouncing or resting), `false` otherwise.
    bool isCollisionEnabled() const {
        return m_currentStateId == 2 || m_currentStateId == 3 || m_currentStateId == 4;
    }

    std::array<ObjectFireSnakeKid *, 2> m_kids; ///< Pointers to the smaller fireball followers
    const s16 m_maxAge;                         ///< FireSnake despawns after this number of frames
    EGG::Vector3f m_xzSunDist;                  ///< Distance between m_spawnPos and m_initPos
    EGG::Vector3f m_fallDir;                    ///< Direction of fall from the sun
    f32 m_xzFallSpeed;                          ///< Speed along the xz-plane while falling
    u16 m_fallDuration;                         ///< How long the firesnake falls from the sun
    std::array<EGG::Matrix34f, 21> m_prevTransforms; ///< The last 21 transformation matrices

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 6> STATE_ENTRIES = {{
            {StateEntry<ObjectFireSnake, &ObjectFireSnake::enterDespawned, nullptr>(0)},
            {StateEntry<ObjectFireSnake, &ObjectFireSnake::enterFalling,
                    &ObjectFireSnake::calcFalling>(1)},
            {StateEntry<ObjectFireSnake, &ObjectFireSnake::enterHighBounce,
                    &ObjectFireSnake::calcHighBounce>(2)},
            {StateEntry<ObjectFireSnake, &ObjectFireSnake::enterRest, &ObjectFireSnake::calcRest>(
                    3)},
            {StateEntry<ObjectFireSnake, nullptr, &ObjectFireSnake::calcBounce>(4)},
            {StateEntry<ObjectFireSnake, nullptr, nullptr>(5)},
    }};

    static constexpr f32 GRAVITY = 3.0f;          ///< Gravitational constant of acceleration
    static constexpr f32 BOUNCE_VELOCITY = 60.0f; ///< Initial upwards speed of a bounce
};

} // namespace Kinoko::Field
