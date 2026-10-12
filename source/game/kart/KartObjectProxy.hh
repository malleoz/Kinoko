#pragma once

#include "game/kart/KartDynamics.hh"
#include "game/kart/KartParam.hh"
#include "game/kart/Status.hh"

#include "game/system/KPadController.hh"

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
class KartHalfPipe;
class KartJump;
class KartMove;
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
    /// @addr{0x8059018C}
    /// @brief Constructor which adds `this` to the static proxy list so it can be shared later
    KartObjectProxy() : m_accessor(nullptr) {
        s_proxyList.push_back(this);
    }

    /// @brief Default virtual destructor
    virtual ~KartObjectProxy() = default;

    /// @beginSetters

    /// @addr{0x80590238}
    /// @brief Sets the kart's position in the kart's @ref KartDynamics subsystem
    void setPos(const EGG::Vector3f &pos) {
        dynamics()->setPos(pos);
    }

    /// @addr{0x80590288}
    /// @brief Sets the kart's rotation in the kart's @ref KartDynamics subsystem
    void setRot(const EGG::Quatf &q) {
        dynamics()->setFullRot(q);
        dynamics()->setMainRot(q);
    }

    /// @addr{0x80591664}
    /// @brief Sets the kart's inertia scale in the kart's @ref KartDynamics subsystem
    void setInertiaScale(const EGG::Vector3f &scale) {
        const EGG::Vector3f *cuboids = bsp().cuboids;
        dynamics()->setInertia(cuboids[0] * scale, cuboids[1] * scale);
    }

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
    [[nodiscard]] const EGG::Vector3f &componentXAxis() const;
    [[nodiscard]] const EGG::Vector3f &componentYAxis() const;
    [[nodiscard]] const EGG::Vector3f &componentZAxis() const;
    [[nodiscard]] const EGG::Vector3f &prevPos() const;
    [[nodiscard]] f32 speed() const;
    [[nodiscard]] f32 acceleration() const;
    [[nodiscard]] f32 softSpeedLimit() const;
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
    [[nodiscard]] s32 hopStickX() const;

    /// @addr{0x80590C44}
    /// @brief Computes the right direction vector of the kart's body from the kart's pose matrix
    /// @return The right direction vector of the kart's body
    [[nodiscard]] EGG::Vector3f bodyRight() const {
        const EGG::Matrix34f &mtx = pose();
        return EGG::Vector3f(mtx[0, 0], mtx[1, 0], mtx[2, 0]);
    }

    /// @addr{0x80590C6C}
    /// @brief Computes the up direction vector of the kart's body from the kart's pose matrix
    /// @return The up direction vector of the kart's body
    [[nodiscard]] EGG::Vector3f bodyUp() const {
        const EGG::Matrix34f &mtx = pose();
        return EGG::Vector3f(mtx[0, 1], mtx[1, 1], mtx[2, 1]);
    }

    /// @addr{0x80590C94}
    /// @brief Computes the forward direction vector of the kart's body from the kart's pose matrix
    /// @return The forward direction vector of the kart's body
    [[nodiscard]] EGG::Vector3f bodyForward() const {
        const EGG::Matrix34f &mtx = pose();
        return EGG::Vector3f(mtx[0, 2], mtx[1, 2], mtx[2, 2]);
    }

    /// @addr{0x8059020C}
    /// @brief Gets the kart's position from the @ref KartDynamics subsystem
    /// @return Const ref to the kart's position vector, @ref KartDynamics::m_pos
    [[nodiscard]] const EGG::Vector3f &pos() const {
        return dynamics()->pos();
    }

    /// @brief Gets the kart's main rotation quaternion from the kart's @ref KartDynamics subsystem
    /// @return Const ref to the kart's main rotation quaternion, @ref KartDynamics::m_mainRot
    [[nodiscard]] const EGG::Quatf &mainRot() const {
        return dynamics()->mainRot();
    }

    /// @brief Gets the kart's full rotation quaternion from the kart's @ref KartDynamics subsystem
    /// @return Const ref to the kart's full rotation quaternion, @ref KartDynamics::m_fullRot
    [[nodiscard]] const EGG::Quatf &fullRot() const {
        return dynamics()->fullRot();
    }

    /// @brief Gets the kart's main rotation quaternion from the kart's @ref KartDynamics subsystem
    /// @return Const ref to the kart's main rotation quaternion, @ref KartDynamics::m_extVel
    [[nodiscard]] const EGG::Vector3f &extVel() const {
        return dynamics()->extVel();
    }

    /// @brief Gets the kart's internal velocity vector from the kart's @ref KartDynamics subsystem
    /// @return Const ref to the kart's internal velocity vector, @ref KartDynamics::m_intVel
    [[nodiscard]] const EGG::Vector3f &intVel() const {
        return dynamics()->intVel();
    }

    /// @brief Gets the kart's overall velocity vector from the kart's @ref KartDynamics subsystem
    /// @return Const ref to the kart's overall velocity vector, @ref KartDynamics::m_velocity
    [[nodiscard]] const EGG::Vector3f &velocity() const {
        return dynamics()->velocity();
    }

    /// @brief Gets the main component of the kart's angular velocity from the @ref KartDynamics
    /// subsystem
    /// @return Const ref to the kart's angVel2 vector, @ref KartDynamics::m_angVel2
    /// @rename
    [[nodiscard]] const EGG::Vector3f &angVel2() const {
        return dynamics()->angVel2();
    }

    /// @addr{0x80590A6C}
    /// @brief Checks if the vehicle is a bike
    /// @return `true` if the vehicle is a bike, `false` if it's a kart
    [[nodiscard]] bool isBike() const {
        return param()->isBike();
    }

    /// @addr{0x805902DC}
    /// @brief Gets the number of suspensions in the kart from the kart's @ref KartParam subsystem
    /// @return The number of suspensions in the kart, @ref KartParam::m_suspCount
    [[nodiscard]] u16 suspCount() const {
        return param()->suspCount();
    }

    /// @addr{0x805902EC}
    /// @brief Gets the number of tires in the kart from the kart's @ref KartParam subsystem
    /// @return The number of tires in the kart, @ref KartParam::m_tireCount
    [[nodiscard]] u16 tireCount() const {
        return param()->tireCount();
    }

    /// @addr{0x805909C8}
    /// @brief Gets the camera Y-axis distance for the kart based on its parameters
    /// @return The camera Y-axis distance for the kart
    [[nodiscard]] f32 cameraDistY() const {
        if (isBike()) {
            return param()->bikeDisp().m_cameraDistY;
        } else {
            return param()->kartDisp().m_cameraDistY;
        }
    }

    /// @addr{0x80590A10}
    /// @brief Gets the @ref KartParam::Stats::DriftType of the kart from its parameters
    /// @return The drift type of the kart, @ref KartParam::Stats::driftType
    [[nodiscard]] KartParam::Stats::DriftType vehicleType() const {
        return param()->stats().driftType;
    }

    /// @brief Gets a reference to the static list of all KartObjectProxy children
    /// @return A reference to the static list of all KartObjectProxy children
    [[nodiscard]] static alloc_list<KartObjectProxy *> &proxyList() {
        return s_proxyList;
    }

    /// @endGetters

protected:
    void apply(size_t idx);

private:
    /// @addr{0x80590138}
    /// @brief For all proxies in the static list, synchronizes all pointers to the KartAccessor.
    /// @param pointers The pointer to synchronize all other proxies to.
    /// @details Iterates the @ref s_proxyList and sets each proxy's @ref m_accessor to the provided
    /// `pointers`. This effectively allows all proxies to share the same pointers to the other
    /// subsystems.
    static void ApplyAll(const KartAccessor *pointers) {
        for (auto iter = s_proxyList.begin(); iter != s_proxyList.end(); ++iter) {
            (*iter)->m_accessor = pointers;
        }
    }

    /// @brief Pointer to the struct containing all subsystem pointers for this kart object
    const KartAccessor *m_accessor;

    /// @brief List of all KartObjectProxy children
    static alloc_list<KartObjectProxy *> s_proxyList;
};

} // namespace Kart

} // namespace Kinoko
