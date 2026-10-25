#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

class KartObject;

/// @brief Manages the lifecycle of a few Kart subsystems and responsible for the high-level
/// two-pass physics calculations.
class KartSub : private KartObjectProxy {
public:
    KartSub();
    ~KartSub() override;

    void createSubsystems(bool isBike, const KartParam::Stats &stats);
    void copyPointers(KartAccessor &pointers);
    void init();
    void initAABB(KartAccessor &accessor, KartObject *object);
    void initPhysicsValues();
    void resetPhysics();

    void calcPass0();
    void calcPass1();
    void resizeAABB(f32 radiusScale);
    void addFloor(const CollisionData &, bool);
    void updateSuspOvertravel(const EGG::Vector3f &suspOvertravel);
    void calcSoftWall();
    void calcMovingObj();
    void calcMovingWater();

    /// @beginGetters

    [[nodiscard]] f32 suspScale();

    /// @endGetters

private:
    KartMove *m_move;                  ///< Pointer to the @ref KartMove subsystem
    KartAction *m_action;              ///< Pointer to the @ref KartAction subsystem
    KartCollide *m_collide;            ///< Pointer to the @ref KartCollide subsystem
    KartState *m_state;                ///< Pointer to the @ref KartState subsystem
    EGG::Vector3f m_maxSuspOvertravel; ///< Max suspension overtravel across all wheels
    EGG::Vector3f m_minSuspOvertravel; ///< Min suspension overtravel across all wheels
    u16 m_floorCollisionCount;         ///< Num of floors collided this frame by the body or wheels
    u16 m_movingObjCollisionCount;     ///< Number of moving objects collided this frame
    u16 m_movingWaterCollisionCount;   ///< Number of moving water collisions this frame
    EGG::Vector3f m_objVel;    ///< Accumulated road velocity from moving object floor collisions
    s16 m_sideCollisionTimer;  ///< Number of frames to apply movement from wall collision
    f32 m_colPerpendicularity; ///< Dot product between floor and colliding wall normals
    f32 m_suspScale; ///< Vertical scale clamped to the vehicle's shrink scale; scales suspension

    static constexpr f32 GRAVITY = -1.3f; ///< Default gravitational force
};

} // namespace Kinoko::Kart
