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
/// copying the transformation matrix of the parent snake from previous frames. @ref
/// ObjectFireSnakeV manages its own state lifecycle, whereas @ref ObjectFireSnake instances rely on
/// @ref ObjectSunDS to advance the state cycle by launching the fire snake and are registered as
/// managed objects so that @ref ObjectSunManager can fetch the FireSnakes.
class ObjectFireSnake : public ObjectProjectile, virtual protected StateManager {
public:
    ObjectFireSnake(const System::MapdataGeoObj &params);
    ~ObjectFireSnake() override;

    void init() override;
    void calc() override;

    /// @addr{0x806C2A5C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    void initProjectile(const EGG::Vector3f &pos) override;

    /// @addr{0x806C29FC}
    /// @brief Callback function use by the @ref ObjectSunDS that wants to throw this projectile.
    void onLaunch() override {
        m_nextStateId = 1;
    }

    void enterDespawned();

    /// @addr{0x806C19E8}
    /// @brief Runs once when the fire snake respawns, loading the collision bounding box
    void enterFalling() {
        if (!getUnit()) {
            loadAABB(0.0f);
        }
    }

    /// @addr{0x806C1DCC}
    /// @brief Runs once after landing from the sun (for the case of @ref ObjectFireSnake), or runs
    /// once upon re-spawning (for the case of @ref ObjectFireSnakeV).
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
    void calcHighBounce() {
        constexpr f32 HIGH_BOUNCE_SCALAR = 1.6f;

        calcBounce(HIGH_BOUNCE_SCALAR * BOUNCE_VELOCITY);
    }

    void calcRest();

    /// @addr{0x806C2254}
    /// @brief Runs every frame the fire snake is bouncing, except the first bounce
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
            {StateEntry<ObjectFireSnake, &ObjectFireSnake::enterDespawned,
                    nullptr>(0)},
            {StateEntry<ObjectFireSnake, &ObjectFireSnake::enterFalling,
                    &ObjectFireSnake::calcFalling>(1)},
            {StateEntry<ObjectFireSnake, &ObjectFireSnake::enterHighBounce,
                    &ObjectFireSnake::calcHighBounce>(2)},
            {StateEntry<ObjectFireSnake, &ObjectFireSnake::enterRest, &ObjectFireSnake::calcRest>(
                    3)},
            {StateEntry<ObjectFireSnake, nullptr,
                    &ObjectFireSnake::calcBounce>(4)},
            {StateEntry<ObjectFireSnake, nullptr,
                    nullptr>(5)},
    }};

    static constexpr f32 GRAVITY = 3.0f;          ///< Gravitational constant of acceleration
    static constexpr f32 BOUNCE_VELOCITY = 60.0f; ///< Initial upwards speed of a bounce
};

} // namespace Kinoko::Field
