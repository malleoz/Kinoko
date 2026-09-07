#pragma once

#include "game/kart/KartParam.hh"
#include "game/kart/Status.hh"

#include "game/system/KPadController.hh"

#include <egg/math/Matrix.hh>

namespace Kinoko {

namespace Field {

class BoxColUnit;
class ObjectCollisionKart;

} // namespace Field

namespace Render {

class KartModel;

} // namespace Render

namespace Kart {

class CollisionGroup;
struct CollisionData;
class KartAction;
class KartBody;
class KartCollide;
class KartDynamics;
class KartHalfPipe;
class KartJump;
class KartMove;
class KartParam;
struct BSP;
class KartPhysics;
class KartScale;
class KartState;
class KartSub;
class KartSuspension;
class KartSuspensionPhysics;
class KartTire;
class WheelPhysics;

/// @brief Collection of kart subsystem pointers associated with a particular @ref KartObject
/// @details These pointers are housed in this struct so that they can be shared amongst each of the
/// subsystems, which each derive from @ref KartObjectProxy. This allows each subsystem to access
/// the other subsystems without needing to know about the other subsystems' existence.
struct KartAccessor {
    KartParam *param;         ///< Pointer to the parameter data (stats, hitboxes, etc.)
    KartBody *body;           ///< Pointer to the body subsystem (sink depth, etc.)
    Render::KartModel *model; ///< Pointer to the model subsystem (rendering, etc.)
    KartSub *sub;             ///< Pointer to the sub subsystem (suspension, tires, etc.)
    KartMove *move;           ///< Pointer to the movement subsystem (acceleration, drifting, etc.)
    KartAction *action;       ///< Pointer to the action subsystem (object collision reactions)
    KartCollide *collide;     ///< Pointer to the collision subsystem (course and object collisions)
    Field::ObjectCollisionKart *objectCollisionKart; ///< Pointer to the object collision subsystem
    KartState *state; ///< Pointer to the state subsystem (boosts, timers, etc.)
    fixed_vector<KartSuspension *> suspensions; ///< Collection of pointers to the wheel suspensions
    fixed_vector<KartTire *> tires;             ///< Collection of pointers to the tires
    Field::BoxColUnit *boxColUnit;              ///< Pointer to the box collision unit subsystem
};

/// @brief Base class for most kart-related objects
/// @details Acts as a shared access hub for all subsystems, so that each subsystem can access the
/// other subsystems without needing to know about the other subsystems' existence. This is done
/// through a @ref KartAccessor struct, which contains pointers to each of the subsystems. The @ref
/// KartObject::Create factory function will apply the @ref KartAccessor struct to @ref m_accessor
/// for all of the subsystems, so that they can access each other.
class KartObjectProxy {
    /// @brief Grants the @ref KartObject class to set the accessor list
    friend class KartObject;

public:
    KartObjectProxy();
    ~KartObjectProxy();

    /// @beginSetters
    void setPos(const EGG::Vector3f &pos);
    void setRot(const EGG::Quatf &q);
    void setInertiaScale(const EGG::Vector3f &scale);
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] KartAction *action();
    [[nodiscard]] const KartAction *action() const;
    [[nodiscard]] KartBody *body();
    [[nodiscard]] const KartBody *body() const;
    [[nodiscard]] KartCollide *collide();
    [[nodiscard]] const KartCollide *collide() const;
    [[nodiscard]] CollisionGroup *collisionGroup();
    [[nodiscard]] const CollisionGroup *collisionGroup() const;
    [[nodiscard]] KartMove *move();
    [[nodiscard]] const KartMove *move() const;
    [[nodiscard]] KartHalfPipe *halfPipe();
    [[nodiscard]] const KartHalfPipe *halfPipe() const;
    [[nodiscard]] KartJump *jump();
    [[nodiscard]] const KartJump *jump() const;
    [[nodiscard]] KartParam *param();
    [[nodiscard]] const KartParam *param() const;
    [[nodiscard]] const BSP &bsp() const;
    [[nodiscard]] KartPhysics *physics();
    [[nodiscard]] const KartPhysics *physics() const;
    [[nodiscard]] KartDynamics *dynamics();
    [[nodiscard]] const KartDynamics *dynamics() const;
    [[nodiscard]] KartState *state();
    [[nodiscard]] const KartState *state() const;
    [[nodiscard]] KartSub *sub();
    [[nodiscard]] const KartSub *sub() const;
    [[nodiscard]] KartSuspension *suspension(u16 suspIdx);
    [[nodiscard]] const KartSuspension *suspension(u16 suspIdx) const;
    [[nodiscard]] KartSuspensionPhysics *suspensionPhysics(u16 suspIdx);
    [[nodiscard]] const KartSuspensionPhysics *suspensionPhysics(u16 suspIdx) const;
    [[nodiscard]] KartTire *tire(u16 tireIdx);
    [[nodiscard]] const KartTire *tire(u16 tireIdx) const;
    [[nodiscard]] WheelPhysics *tirePhysics(u16 tireIdx);
    [[nodiscard]] const WheelPhysics *tirePhysics(u16 tireIdx) const;
    [[nodiscard]] CollisionData &collisionData();
    [[nodiscard]] const CollisionData &collisionData() const;
    [[nodiscard]] CollisionData &collisionData(u16 tireIdx);
    [[nodiscard]] const CollisionData &collisionData(u16 tireIdx) const;
    [[nodiscard]] const System::KPad *inputs() const;
    [[nodiscard]] Render::KartModel *model();
    [[nodiscard]] const Render::KartModel *model() const;
    [[nodiscard]] Field::ObjectCollisionKart *objectCollisionKart();
    [[nodiscard]] const Field::ObjectCollisionKart *objectCollisionKart() const;
    [[nodiscard]] Field::BoxColUnit *boxColUnit();
    [[nodiscard]] const Field::BoxColUnit *boxColUnit() const;

    [[nodiscard]] const EGG::Vector3f &scale() const;
    [[nodiscard]] const EGG::Matrix34f &pose() const;
    [[nodiscard]] EGG::Vector3f bodyRight() const;
    [[nodiscard]] EGG::Vector3f bodyUp() const;
    [[nodiscard]] EGG::Vector3f bodyForward() const;

    [[nodiscard]] const EGG::Vector3f &componentXAxis() const;
    [[nodiscard]] const EGG::Vector3f &componentYAxis() const;
    [[nodiscard]] const EGG::Vector3f &componentZAxis() const;

    [[nodiscard]] const EGG::Vector3f &pos() const;
    [[nodiscard]] const EGG::Vector3f &prevPos() const;
    [[nodiscard]] const EGG::Quatf &mainRot() const;
    [[nodiscard]] const EGG::Quatf &fullRot() const;
    [[nodiscard]] const EGG::Vector3f &extVel() const;
    [[nodiscard]] const EGG::Vector3f &intVel() const;
    [[nodiscard]] const EGG::Vector3f &velocity() const;
    [[nodiscard]] f32 speed() const;
    [[nodiscard]] f32 acceleration() const;
    [[nodiscard]] f32 softSpeedLimit() const;
    [[nodiscard]] const EGG::Vector3f &angVel2() const;
    [[nodiscard]] bool isBike() const;
    [[nodiscard]] u16 suspCount() const;
    [[nodiscard]] u16 tireCount() const;
    [[nodiscard]] bool hasFloorCollision(const WheelPhysics *wheelPhysics) const;
    [[nodiscard]] std::pair<EGG::Vector3f, EGG::Vector3f> getCannonPosRot();
    [[nodiscard]] f32 speedRatio() const;
    [[nodiscard]] f32 speedRatioCapped() const;
    [[nodiscard]] bool isPostRespawn() const;
    [[nodiscard]] Field::KCLTypeMask wallKclType() const;
    [[nodiscard]] u32 wallKclVariant() const;
    [[nodiscard]] Status &status();
    [[nodiscard]] const Status &status() const;
    [[nodiscard]] const EGG::Vector3f &wheelPos(u16 idx) const;
    [[nodiscard]] const EGG::Vector3f &wheelEdgePos(u16 idx) const;
    [[nodiscard]] f32 cameraDistY() const;
    [[nodiscard]] s32 hopStickX() const;
    [[nodiscard]] KartParam::Stats::DriftType vehicleType() const;

    /// @brief Gets a reference to the static list of all KartObjectProxy children
    /// @return A reference to the static list of all KartObjectProxy children
    [[nodiscard]] static alloc_list<KartObjectProxy *> &proxyList() {
        return s_proxyList;
    }
    /// @endGetters

protected:
    void apply(size_t idx);

private:
    static void ApplyAll(const KartAccessor *pointers);

    /// @brief Pointer to the struct containing all subsystem pointers for this kart object
    const KartAccessor *m_accessor;

    /// @brief List of all KartObjectProxy children
    static alloc_list<KartObjectProxy *> s_proxyList;
};

} // namespace Kart

} // namespace Kinoko
