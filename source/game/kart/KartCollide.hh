#pragma once

#include "game/kart/KartPullPath.hh"
#include "game/kart/Reaction.hh"

namespace Kinoko {

namespace EGG {

struct BoundBox3f;

} // namespace EGG

namespace Field {

struct CollisionInfo;

} // namespace Field

namespace Kart {

enum class Action;
class CollisionGroup;
class Hitbox;

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
    ~KartCollide() override;

    void init();
    void setHitboxLastPos();
    void calcHitboxes();
    void findCollision();
    void calcRebound();
    void calcFloorEffect();
    void calcTriggers(Field::KCLTypeMask *mask);
    void calcFallBoundary(Field::KCLTypeMask *mask, bool shortBoundary);
    void calcBeforeRespawnAndShrink();
    void activateOob(bool detachCamera, Field::KCLTypeMask *mask, bool somethingCPU,
            bool somethingBullet);
    void calcWheelCollision(u16 wheelIdx, CollisionGroup *hitboxGroup, const EGG::Vector3f &colVel,
            const EGG::Vector3f &center, f32 radius);
    void calcBoundingRadius();
    void calcObjectCollision();
    void applySomeFloorMoment(f32 down, f32 rate, CollisionGroup *hitboxGroup,
            const EGG::Vector3f &forward, const EGG::Vector3f &nextDir, const EGG::Vector3f &speed,
            bool b1, bool b2, bool b3);
    void calcFloorMomentScalar();
    void applyWeakFloorMomentScalar();

    /// @beginSetters

    void setTangentOff(const EGG::Vector3f &v);
    void setMovement(const EGG::Vector3f &v);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] KartPullPath &pullPath();
    [[nodiscard]] const KartPullPath &pullPath() const;
    [[nodiscard]] f32 boundingRadius() const;
    [[nodiscard]] f32 floorMomentRate() const;
    [[nodiscard]] const SurfaceFlags &surfaceFlags() const;
    [[nodiscard]] const EGG::Vector3f &tangentOff() const;
    [[nodiscard]] const EGG::Vector3f &movement() const;
    [[nodiscard]] f32 sumHitboxBottomHeightSoftWall() const;
    [[nodiscard]] u16 numSoftWallCollisions() const;
    [[nodiscard]] f32 sumHitboxBottomHeightFloorOnly() const;
    [[nodiscard]] u16 numFloorOnlyCollisions() const;
    [[nodiscard]] f32 colPerpendicularity() const;

    /// @endGetters

    static void SetFloorColInfo(CollisionData &collisionData, const EGG::Vector3f &relPos,
            const EGG::Vector3f &vel, const EGG::Vector3f &floorNrm);

private:
    /// @brief Function pointer type for a function that represents an object collision handler
    /// @param idx The index of the colliding object's hit depth in @ref
    /// Field::ObjectDirector::m_hitDepths
    typedef Action (KartCollide::*ObjectCollisionHandler)(size_t idx);

    /* ================================ *
     *     OBJECT COLLISION HANDLERS
     * ================================ */

    Action handleReactNone(size_t idx);
    Action handleReactWall(size_t idx);
    Action handleReactSpinOnce(size_t idx);
    Action handleReactSpinTwice(size_t idx);
    Action handleReactFireSpin(size_t idx);
    Action handleReactSmallLaunch(size_t idx);
    Action handleReactLaunchAwayFlipOnce(size_t idx);
    Action handleReactLaunchSpinLoseItem(size_t idx);
    Action handleReactLaunchAwayFlipTwice(size_t idx);
    Action handleReactLongCrushLoseItem(size_t idx);
    Action handleReactSmallBump(size_t idx);
    Action handleReactSpinShrink(size_t idx);
    Action handleReactHighLaunchLoseItem(size_t idx);
    Action handleReactWeakWall(size_t idx);
    Action handleReactOffroad(size_t idx);
    Action handleReactSideways(size_t idx);
    Action handleReactWall3(size_t idx);
    Action handleReactRubberWall(size_t idx);
    Action handleReactUntrickableJumpPad(size_t idx);
    Action handleReactShortCrushLoseItem(size_t idx);
    Action handleReactCrushRespawn(size_t idx);
    Action handleReactExplosionLoseItem(size_t idx);

    void applyRebound(f32 param_1, f32 param_2, bool lockXZ, bool addExtVelY);
    void calcBodyCollision(f32 totalScale, f32 sinkDepth, const EGG::Quatf &rot,
            const EGG::Vector3f &scale);
    [[nodiscard]] bool accumulateBodyCollision(CollisionData &collisionData, const Hitbox &hitbox,
            EGG::BoundBox3f &minMax, EGG::Vector3f &relPos, s32 &count,
            const Field::KCLTypeMask &maskOut, const Field::CollisionInfo &colInfo);
    void applyBodyCollision(CollisionData &collisionData, const EGG::Vector3f &movement,
            const EGG::Vector3f &posRel, s32 count);
    void calcLeanCollision(Field::KCLTypeMask *maskOut, const EGG::Vector3f &pos, bool twoPoint);
    void calcSideCollision(CollisionData &collisionData, Hitbox &hitbox,
            Field::CollisionInfo *colInfo);
    void calcPoleTimer();
    void processBody(CollisionData &collisionData, Hitbox &hitbox, Field::CollisionInfo *colInfo,
            Field::KCLTypeMask *maskOut);
    void processMovingWater(CollisionData &collisionData, Field::KCLTypeMask *maskOut);
    [[nodiscard]] bool processWall(CollisionData &collisionData, Field::KCLTypeMask *maskOut);
    void processFloor(CollisionData &collisionData, Hitbox &hitbox, Field::CollisionInfo *colInfo,
            Field::KCLTypeMask *mask, bool wheel);
    void processCannon(Field::KCLTypeMask *maskOut);
    void processWheel(CollisionData &collisionData, Hitbox &hitbox, Field::CollisionInfo *colInfo,
            Field::KCLTypeMask *mask);

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
    s16 m_poleAngVelTimer;     ///< Cooldown frames after hitting @ref Field::ObjectId::DummyPole
    f32 m_poleYaw;             ///< Yaw induced by hitting @ref Field::ObjectId::DummyPole
    f32 m_colPerpendicularity; ///< Dot product between floor and colliding wall normals.

    /// @brief The maximum number of reactions, used for array sizing
    static constexpr size_t MAX_REACTION = static_cast<size_t>(Reaction::Max);

    /// @brief Array of function pointers to the object collision handlers, indexed by @ref Reaction
    static constexpr std::array<ObjectCollisionHandler, MAX_REACTION> s_objectCollisionHandlers = {{
            &KartCollide::handleReactNone,                ///< @ref Reaction::None
            &KartCollide::handleReactNone,                ///< Unused
            &KartCollide::handleReactNone,                ///< Unused
            &KartCollide::handleReactNone,                ///< @ref Reaction::UNK_3
            &KartCollide::handleReactNone,                ///< @ref Reaction::UNK_4
            &KartCollide::handleReactNone,                ///< @ref Reaction::UNK_5
            &KartCollide::handleReactNone,                ///< Unused
            &KartCollide::handleReactNone,                ///< @ref Reaction::UNK_7
            &KartCollide::handleReactWall,                ///< @ref Reaction::Wall
            &KartCollide::handleReactSpinOnce,            ///< @ref Reaction::SpinOnce
            &KartCollide::handleReactSpinTwice,           ///< @ref Reaction::SpinTwice
            &KartCollide::handleReactFireSpin,            ///< @ref Reaction::FireSpin
            &KartCollide::handleReactNone,                ///< @ref Reaction::ClipThroughSomeSpeed
            &KartCollide::handleReactSmallLaunch,         ///< @ref Reaction::SmallLaunch
            &KartCollide::handleReactLaunchAwayFlipOnce,  ///< @ref Reaction::LaunchAwayFlipOnce
            &KartCollide::handleReactLaunchSpinLoseItem,  ///< @ref Reaction::LaunchSpinLoseItem
            &KartCollide::handleReactLaunchAwayFlipTwice, ///< @ref Reaction::LaunchAwayFlipTwice
            &KartCollide::handleReactLongCrushLoseItem,   ///< @ref Reaction::LongCrushLoseItem
            &KartCollide::handleReactSmallBump,           ///< @ref Reaction::SmallBump
            &KartCollide::handleReactNone,                ///< @ref Reaction::BigBump
            &KartCollide::handleReactSpinShrink,          ///< @ref Reaction::SpinShrink
            &KartCollide::handleReactHighLaunchLoseItem,  ///< @ref Reaction::HighLaunchLoseItem
            &KartCollide::handleReactNone,                ///< @ref Reaction::SpinHitSomeSpeed
            &KartCollide::handleReactWeakWall,            ///< @ref Reaction::WeakWall
            &KartCollide::handleReactOffroad,             ///< @ref Reaction::Offroad
            &KartCollide::handleReactSideways,            ///< @ref Reaction::Sideways
            &KartCollide::handleReactWall3,               ///< @ref Reaction::Wall3
            &KartCollide::handleReactRubberWall,          ///< @ref Reaction::RubberWall
            &KartCollide::handleReactNone,                ///< @ref Reaction::Wall2
            &KartCollide::handleReactUntrickableJumpPad,  ///< @ref Reaction::UntrickableJumpPad
            &KartCollide::handleReactShortCrushLoseItem,  ///< @ref Reaction::ShortCrushLoseItem
            &KartCollide::handleReactCrushRespawn,        ///< @ref Reaction::CrushRespawn
            &KartCollide::handleReactExplosionLoseItem,   ///< @ref Reaction::ExplosionLoseItem
    }};
};

} // namespace Kart

} // namespace Kinoko
