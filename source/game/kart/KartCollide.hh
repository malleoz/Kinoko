#pragma once

#include "game/kart/CollisionGroup.hh"
#include "game/kart/KartAction.hh"
#include "game/kart/KartMove.hh"
#include "game/kart/KartPullPath.hh"

#include "game/field/ObjectCollisionKart.hh"

#include <egg/math/BoundBox.hh>

namespace Kinoko::Kart {

/// @brief Represents the different reactions that can occur when a kart collides with an object.
/// @details @ref ObjectCollidable objects can specify a reaction in their onCollision function in
/// order to determine how the kart should respond to the collision.
enum class Reaction {
    None = 0,                  ///< No reaction occurs, the kart continues as normal
    UNK_3 = 3,                 ///< Unknown
    UNK_4 = 4,                 ///< Unknown
    UNK_5 = 5,                 ///< Unknown
    UNK_7 = 7,                 ///< Unknown
    Wall = 8,                  ///< The kart collides with a wall and is knocked back
    SpinOnce = 9,              ///< The kart spins out once but keeps its items
    SpinTwice = 10,            ///< The kart spins out twice but keeps its items
    FireSpin = 11,             ///< The kart spins out due to a fireball collision
    ClipThroughSomeSpeed = 12, ///< Unused in Kinoko
    SmallLaunch = 13,          ///< The kart launches forward into the air
    LaunchAwayFlipOnce = 14,   ///< Launches into the air away from the collision and flips once
    LaunchSpinLoseItem = 15,   ///< The kart launches into the air, flips once, and loses its items
    LaunchAwayFlipTwice = 16,  ///< Launches into the air away from the collision and flips twice
    LongCrushLoseItem = 17,    ///< The kart is crushed for 8 seconds and loses its items
    SmallBump = 18,            ///< The kart is bumped away by the colliding object
    BigBump = 19,              ///< Unused in Kinoko
    SpinShrink = 20,           ///< The kart spins out and shrinks in size temporarily
    HighLaunchLoseItem = 21,   ///< The kart launches high into the air, and loses its items
    SpinHitSomeSpeed = 22,     ///< Decays to SpinTwice or WallAllSpeed based on the kart's speed
    WeakWall = 23,             ///< The kart's speed drops by 82% due to a weak wall
    Offroad = 24,              ///< The kart will slowdown due to offroad collision
    idewaysFlipTwice = 25,     ///< The kart launches sideways and flips twice
    Wall3 = 26,                ///< The kart is colliding with Wall3
    RubberWall = 27,         ///< Elastic collision sending the kart away from the colliding object
    Wall2 = 28,              ///< Unused in Kinoko
    UntrickableJumpPad = 29, ///< The kart bounces into the air like a jump pad but cannot trick
    ShortCrushLoseItem = 30, ///< The kart is crushed for 4 seconds and loses its items
    CrushRespawn = 31,       ///< The kart is crushed and respawns @unused
    ExplosionLoseItem = 32,  ///< The kart loses its items and is knocked upwards
    Max = 33,                ///< The maximum number of reactions, used for array sizing
};

/// @brief Manages body+wheel collision and its influence on position/velocity/etc.
class KartCollide : private KartObjectProxy {
public:
    /// @brief Represents the different surface types that a kart is currently colliding with
    enum class eSurfaceFlags {
        Wall = 0,          ///< The kart is currently colliding with wall KCL
        SolidOOB = 1,      ///< The kart is currently colliding with out-of-bounds KCL
        ObjectWall = 2,    ///< The kart is currently colliding with an object that acts as a wall
        ObjectWall3 = 3,   ///< The kart is currently colliding with an object that acts as Wall3
        BoostRamp = 4,     ///< The kart is currently colliding with boost ramp KCL
        Offroad = 6,       ///< The kart is currently colliding with offroad KCL @unused
        Trickable = 11,    ///< The kart is currently colliding with trickable KCL
        NotTrickable = 12, ///< The kart is currently colliding with non-trickable KCL
        EndHalfPipe = 16,  ///< The kart is colliding with the end of half-pipe KCL
    };

    /// @brief A bitfield of @ref eSurfaceFlags that represents the surface types that a kart is
    /// currently colliding with
    typedef EGG::TBitFlag<u32, eSurfaceFlags> SurfaceFlags;

    KartCollide();
    ~KartCollide();

    void init();
    void setHitboxLastPos();

    void calcHitboxes();

    void findCollision();
    void calcRebound();
    void applyRebound(f32 param_1, f32 param_2, bool lockXZ, bool addExtVelY);
    void calcBodyCollision(f32 totalScale, f32 sinkDepth, const EGG::Quatf &rot,
            const EGG::Vector3f &scale);
    void calcFloorEffect();
    void calcLeanCollision(Field::KCLTypeMask *mask, const EGG::Vector3f &pos, bool twoPoint);
    void calcTriggers(Field::KCLTypeMask *mask);
    void calcFallBoundary(Field::KCLTypeMask *mask, bool shortBoundary);
    void calcBeforeRespawnAndShrink();
    void activateOob(bool detachCamera, Field::KCLTypeMask *mask, bool somethingCPU,
            bool somethingBullet);
    void calcWheelCollision(u16 wheelIdx, CollisionGroup *hitboxGroup, const EGG::Vector3f &colVel,
            const EGG::Vector3f &center, f32 radius);
    void calcSideCollision(CollisionData &collisionData, Hitbox &hitbox,
            Field::CollisionInfo *colInfo);
    void calcBoundingRadius();
    void calcObjectCollision();
    void calcPoleTimer();

    /// @addr{0x8056E8D4}
    /// @brief Processes moving water and floor collision effects
    void processWheel(CollisionData &collisionData, Hitbox &hitbox, Field::CollisionInfo *colInfo,
            Field::KCLTypeMask *maskOut) {
        processMovingWater(collisionData, maskOut);
        processFloor(collisionData, hitbox, colInfo, maskOut, true);
    }

    void processBody(CollisionData &collisionData, Hitbox &hitbox, Field::CollisionInfo *colInfo,
            Field::KCLTypeMask *maskOut);
    void processMovingWater(CollisionData &collisionData, Field::KCLTypeMask *maskOut);
    [[nodiscard]] bool processWall(CollisionData &collisionData, Field::KCLTypeMask *maskOut);
    void processFloor(CollisionData &collisionData, Hitbox &hitbox, Field::CollisionInfo *colInfo,
            Field::KCLTypeMask *maskOut, bool wheel);
    void processCannon(Field::KCLTypeMask *maskOut);

    void applySomeFloorMoment(f32 down, f32 rate, CollisionGroup *hitboxGroup,
            const EGG::Vector3f &forward, const EGG::Vector3f &nextDir, const EGG::Vector3f &speed,
            bool b1, bool b2, bool b3);

    [[nodiscard]] bool accumulateBodyCollision(CollisionData &collisionData, const Hitbox &hitbox,
            EGG::BoundBox3f &minMax, EGG::Vector3f &relPos, s32 &count,
            const Field::KCLTypeMask &maskOut, const Field::CollisionInfo &colInfo);
    void applyBodyCollision(CollisionData &collisionData, const EGG::Vector3f &movement,
            const EGG::Vector3f &posRel, s32 count);

    /// @addr{0x805713D8}
    /// @brief Sets the floor moment scalar to a small value
    void applyWeakFloorMomentScalar() {
        m_floorMomentScalar = 0.01f;
    }

    void calcFloorMomentScalar();

    /* ================================ *
     *     OBJECT COLLISION HANDLERS
     * ================================ */

    /// @addr{0x8056E564}
    /// @brief Handles the case where a kart collides with an object that has no reaction
    Action handleReactNone(size_t /*idx*/) {
        return Action::None;
    }

    /// @addr{0x8057363C}
    /// @brief Adds the wall's hit direction to the total wall normal
    Action handleReactWall(size_t idx) {
        m_totalReactionWallNrm += Field::ObjectCollisionKart::GetHitDirection(idx);
        m_surfaceFlags.setBit(eSurfaceFlags::ObjectWall);

        return Action::None;
    }

    /// @addr{0x805733CC}
    /// @brief Maps @ref Reaction::SpinOnce to @ref Action::SpinOnce
    Action handleReactSpinOnce(size_t /*idx*/) {
        return Action::SpinOnce;
    }

    /// @addr{0x805733D4}
    /// @brief Maps @ref Reaction::SpinTwice to @ref Action::SpinTwice
    Action handleReactSpinTwice(size_t /*idx*/) {
        return Action::SpinTwice;
    }

    /// @addr{0x805735AC}
    /// @brief Maps @ref Reaction::FireSpin to @ref Action::FireSpin
    Action handleReactFireSpin(size_t /*idx*/) {
        return Action::FireSpin;
    }

    /// @addr{0x805733C4}
    /// @brief Maps @ref Reaction::SmallLaunch to @ref Action::ForwardLaunch
    Action handleReactSmallLaunch(size_t /*idx*/) {
        return Action::ForwardLaunch;
    }

    /// @addr{0x805733DC}
    /// @brief Maps @ref Reaction::LaunchAwayFlipOnce to @ref Action::AwayFlipOnce
    Action handleReactLaunchAwayFlipOnce(size_t /*idx*/) {
        return Action::AwayFlipOnce;
    }

    /// @addr{0x8057353C}
    /// @brief Maps @ref Reaction::LaunchSpinLoseItem to @ref Action::LaunchSpinLoseItem
    Action handleReactLaunchSpinLoseItem(size_t /*idx*/) {
        return Action::LaunchSpinLoseItem;
    }

    /// @addr{0x805733EC}
    /// @brief Maps @ref Reaction::LaunchAwayFlipTwice to @ref Action::AwayFlipTwice
    Action handleReactLaunchAwayFlipTwice(size_t /*idx*/) {
        return Action::AwayFlipTwice;
    }

    /// @addr{0x805735B4}
    /// @brief Maps @ref Reaction::LongCrushLoseItem to @ref Action::LongCrushLoseItem
    Action handleReactLongCrushLoseItem(size_t /*idx*/) {
        return Action::LongCrushLoseItem;
    }

    /// @addr{0x805737B8}
    /// @brief Applies a small force to the kart away from the colliding object
    Action handleReactSmallBump(size_t idx) {
        move()->applyForce(30.0f, objectCollisionKart()->GetHitDirection(idx), false);
        return Action::None;
    }

    /// @addr{0x805735BC}
    /// @brief Maps @ref Reaction::SpinShrink to @ref Action::SpinShrink if no cooldown
    Action handleReactSpinShrink(size_t /*idx*/) {
        return m_shrinkTimer <= 0 ? Action::SpinShrink : Action::None;
    }

    /// @addr{0x805733E4}
    /// @brief Maps @ref Reaction::HighLaunchLoseItem to @ref Action::HighLaunchLoseItem
    Action handleReactHighLaunchLoseItem(size_t /*idx*/) {
        return Action::HighLaunchLoseItem;
    }

    /// @addr{0x80573754}
    /// @brief Reduces the kart's speed by 18% due to a weak wall collision
    Action handleReactWeakWall(size_t /*idx*/) {
        constexpr f32 WEAK_WALL_SPEED_SCALAR = 0.82f;

        move()->setSpeed(speed() * WEAK_WALL_SPEED_SCALAR);
        return Action::None;
    }

    /// @addr{0x80573790}
    /// @brief Sets the appropriate status bits to reflect colliding with offroad KCL
    Action handleReactOffroad(size_t /*idx*/) {
        status().setBit(eStatus::CollidingOffroad);
        m_surfaceFlags.setBit(eSurfaceFlags::Offroad);
        return Action::None;
    }

    /// @addr{0x805733F4}
    /// @brief Maps @ref Reaction::SidewaysFlipTwice to @ref Action::SidewaysFlipTwice
    /// @param idx The index of the colliding object in the @ref ObjectDirector's array of colliding
    /// objects
    Action handleReactSidewaysFlipTwice(size_t idx) {
        action()->setVelocity(objectCollisionKart()->translation(idx));
        return Action::SidewaysFlipTwice;
    }

    /// @addr{0x805736C8}
    /// @brief Adds the wall's hit direction to the total wall normal
    Action handleReactWall3(size_t idx) {
        m_totalReactionWallNrm += Field::ObjectCollisionKart::GetHitDirection(idx);
        m_surfaceFlags.setBit(eSurfaceFlags::ObjectWall3);

        return Action::None;
    }

    Action handleReactRubberWall(size_t idx);

    /// @addr{0x805735EC}
    /// @brief Handles the case where a kart collides with an untrickable jump pad
    Action handleReactUntrickableJumpPad(size_t /*idx*/) {
        move()->setPadType(KartMove::PadType(KartMove::ePadType::JumpPad));
        state()->setJumpPadVariant(0);

        return Action::None;
    }

    /// @addr{0x805735D4}
    /// @brief Maps @ref Reaction::ShortCrushLoseItem to @ref Action::ShortCrushLoseItem
    Action handleReactShortCrushLoseItem(size_t /*idx*/) {
        return Action::ShortCrushLoseItem;
    }

    /// @addr{0x805735DC}
    /// @brief Maps @ref Reaction::CrushRespawn to @ref Action::CrushRespawn
    Action handleReactCrushRespawn(size_t /*idx*/) {
        return Action::CrushRespawn;
    }

    /// @addr{0x805735E4}
    /// @brief Maps @ref Reaction::ExplosionLoseItem to @ref Action::ExplosionLoseItem
    Action handleReactExplosionLoseItem(size_t /*idx*/) {
        return Action::ExplosionLoseItem;
    }

    /// @addr{0x805B78D0}
    /// @brief Sets the floor collision information in a @ref CollisionData struct
    /// @param collisionData The @ref CollisionData struct to set
    /// @param relPos The position of the colliding hitbox relative to the kart's position
    /// @param vel The velocity of the kart at the time of collision
    /// @param floorNrm The normal of the colliding floor KCL
    void setFloorColInfo(CollisionData &collisionData, const EGG::Vector3f &relPos,
            const EGG::Vector3f &vel, const EGG::Vector3f &floorNrm) {
        collisionData.relPos = relPos;
        collisionData.vel = vel;
        collisionData.floorNrm = floorNrm;
        collisionData.bFloor = true;
    }

    /// @beginSetters
    void setTangentOff(const EGG::Vector3f &v) {
        m_tangentOff = v;
    }
    void setMovement(const EGG::Vector3f &v) {
        m_movement = v;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] KartPullPath &pullPath() {
        return m_pullPath;
    }

    [[nodiscard]] const KartPullPath &pullPath() const {
        return m_pullPath;
    }

    [[nodiscard]] f32 boundingRadius() const {
        return m_boundingRadius;
    }

    [[nodiscard]] f32 floorMomentRate() const {
        return m_floorMomentScalar;
    }

    [[nodiscard]] const SurfaceFlags &surfaceFlags() const {
        return m_surfaceFlags;
    }

    [[nodiscard]] const EGG::Vector3f &tangentOff() const {
        return m_tangentOff;
    }

    [[nodiscard]] const EGG::Vector3f &movement() const {
        return m_movement;
    }

    [[nodiscard]] f32 sumHitboxBottomHeightSoftWall() const {
        return m_sumSoftwallBottomHeight;
    }

    [[nodiscard]] u16 numSoftWallCollisions() const {
        return m_numSoftWallCollisions;
    }

    [[nodiscard]] f32 sumHitboxBottomHeightFloorOnly() const {
        return m_sumFloorBottomHeight;
    }

    [[nodiscard]] u16 numFloorOnlyCollisions() const {
        return m_numFloorOnlyCollisions;
    }

    [[nodiscard]] f32 colPerpendicularity() const {
        return m_colPerpendicularity;
    }
    /// @endGetters

private:
    /// @brief Function pointer type for a function that represents an object collision handler
    /// @param idx The index of the colliding object's hit depth in @ref
    /// Field::ObjectDirector::m_hitDepths
    typedef Action (KartCollide::*ObjectCollisionHandler)(size_t idx);

    KartPullPath m_pullPath; ///< The state of the pull path that the kart is currently following
    f32 m_boundingRadius; ///< The radius of the sphere that encompasses the kart's body and wheels
    f32 m_floorMomentScalar; ///< Controls how fast the kart's floor alignment is corrected
    EGG::Vector3f m_totalReactionWallNrm; ///< Accumulate hit direction of all colliding walls
    SurfaceFlags m_surfaceFlags;   ///< The surface types that the kart is currently colliding with
    EGG::Vector3f m_tangentOff;    ///< Accumulated hit depths of all colliding objects
    EGG::Vector3f m_movement;      ///< Represents the actual movement of the kart after collision
    s16 m_respawnTimer;            ///< After triggering OOB, frames until the kart is respawned
    s16 m_solidOobTimer;           ///< Frames the kart has been colliding with solid OOB collision
    s16 m_shrinkTimer;             ///< Cooldown timer to prevents repeated shrink ejections
    f32 m_smoothedBack;            ///< Smoothed back direction of the kart
    f32 m_sumSoftwallBottomHeight; ///< Accumulated bottom height of all colliding soft walls
    u16 m_numSoftWallCollisions;   ///< Number of soft wall collisions that have occurred this frame
    f32 m_sumFloorBottomHeight;    ///< Accumulated bottom height of all colliding floor KCL
    u16 m_numFloorOnlyCollisions;  ///< Number of floor collisions without soft wall collisions
    s16 m_poleAngVelTimer;     ///< Cooldown frames after hitting @enum Field::ObjectId::DummyPole
    f32 m_poleYaw;             ///< Yaw induced by hitting @enum Field::ObjectId::DummyPole
    f32 m_colPerpendicularity; ///< Dot product between floor and colliding wall normals.

    /// @brief The maximum number of reactions, used for array sizing
    static constexpr size_t MAX_REACTION = static_cast<size_t>(Reaction::Max);

    /// @brief Array of function pointers to the object collision handlers, indexed by @ref Reaction
    static constexpr std::array<ObjectCollisionHandler, MAX_REACTION> s_objectCollisionHandlers = {{
            &KartCollide::handleReactNone,                ///< @enum Reaction::None
            &KartCollide::handleReactNone,                ///< Unused
            &KartCollide::handleReactNone,                ///< Unused
            &KartCollide::handleReactNone,                ///< @enum Reaction::UNK_3
            &KartCollide::handleReactNone,                ///< @enum Reaction::UNK_4
            &KartCollide::handleReactNone,                ///< @enum Reaction::UNK_5
            &KartCollide::handleReactNone,                ///< Unused
            &KartCollide::handleReactNone,                ///< @enum Reaction::UNK_7
            &KartCollide::handleReactWall,                ///< @enum Reaction::Wall
            &KartCollide::handleReactSpinOnce,            ///< @enum Reaction::SpinOnce
            &KartCollide::handleReactSpinTwice,           ///< @enum Reaction::SpinTwice
            &KartCollide::handleReactFireSpin,            ///< @enum Reaction::FireSpin
            &KartCollide::handleReactNone,                ///< @enum Reaction::ClipThroughSomeSpeed
            &KartCollide::handleReactSmallLaunch,         ///< @enum Reaction::SmallLaunch
            &KartCollide::handleReactLaunchAwayFlipOnce,  ///< @enum Reaction::LaunchAwayFlipOnce
            &KartCollide::handleReactLaunchSpinLoseItem,  ///< @enum Reaction::LaunchSpinLoseItem
            &KartCollide::handleReactLaunchAwayFlipTwice, ///< @enum Reaction::LaunchAwayFlipTwice
            &KartCollide::handleReactLongCrushLoseItem,   ///< @enum Reaction::LongCrushLoseItem
            &KartCollide::handleReactSmallBump,           ///< @enum Reaction::SmallBump
            &KartCollide::handleReactNone,                ///< @enum Reaction::BigBump
            &KartCollide::handleReactSpinShrink,          ///< @enum Reaction::SpinShrink
            &KartCollide::handleReactHighLaunchLoseItem,  ///< @enum Reaction::HighLaunchLoseItem
            &KartCollide::handleReactNone,                ///< @enum Reaction::SpinHitSomeSpeed
            &KartCollide::handleReactWeakWall,            ///< @enum Reaction::WeakWall
            &KartCollide::handleReactOffroad,             ///< @enum Reaction::Offroad
            &KartCollide::handleReactSidewaysFlipTwice,   ///< @enum Reaction::SidewaysFlipTwice
            &KartCollide::handleReactWall3,               ///< @enum Reaction::Wall3
            &KartCollide::handleReactRubberWall,          ///< @enum Reaction::RubberWall
            &KartCollide::handleReactNone,                ///< @enum Reaction::Wall2
            &KartCollide::handleReactUntrickableJumpPad,  ///< @enum Reaction::UntrickableJumpPad
            &KartCollide::handleReactShortCrushLoseItem,  ///< @enum Reaction::ShortCrushLoseItem
            &KartCollide::handleReactCrushRespawn,        ///< @enum Reaction::CrushRespawn
            &KartCollide::handleReactExplosionLoseItem,   ///< @enum Reaction::ExplosionLoseItem
    }};
};

} // namespace Kinoko::Kart
