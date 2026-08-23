#include "KartObjectProxy.hh"

#include "game/kart/KartSuspension.hh"
#include "game/kart/KartTire.hh"

#include "game/system/CourseMap.hh"
#include "game/system/RaceManager.hh"

namespace Kinoko::Kart {

/// @addr{0x8059018C}
KartObjectProxy::KartObjectProxy() : m_accessor(nullptr) {
    s_proxyList.push_back(this);
}

KartObjectProxy::~KartObjectProxy() = default;

/// @addr{0x80590238}
/// @brief Sets the kart's position in the kart's @ref KartDynamics subsystem
void KartObjectProxy::setPos(const EGG::Vector3f &pos) {
    dynamics()->setPos(pos);
}

/// @addr{0x80590288}
/// @brief Sets the kart's rotation in the kart's @ref KartDynamics subsystem
void KartObjectProxy::setRot(const EGG::Quatf &q) {
    dynamics()->setFullRot(q);
    dynamics()->setMainRot(q);
}

/// @addr{0x80591664}
/// @brief Sets the kart's inertia scale in the kart's @ref KartDynamics subsystem
void KartObjectProxy::setInertiaScale(const EGG::Vector3f &scale) {
    const EGG::Vector3f *cuboids = bsp().cuboids;
    dynamics()->setInertia(cuboids[0] * scale, cuboids[1] * scale);
}

/// @addr{0x80590D20}
/// @brief Gets a pointer to the kart's @ref KartAction subsystem
/// @ref A pointer to the kart's @ref KartAction subsystem
KartAction *KartObjectProxy::action() {
    return m_accessor->action;
}

/// @addr{0x80590D20}
/// @brief Gets a pointer to the kart's @ref KartAction subsystem
/// @ref A const pointer to the kart's @ref KartAction subsystem
const KartAction *KartObjectProxy::action() const {
    return m_accessor->action;
}

/// @addr{0x8059069C}
/// @brief Gets a pointer to the kart's @ref KartBody subsystem
/// @ref A pointer to the kart's @ref KartBody subsystem
KartBody *KartObjectProxy::body() {
    return m_accessor->body;
}

/// @addr{0x8059069C}
/// @brief Gets a pointer to the kart's @ref KartBody subsystem
/// @ref A const pointer to the kart's @ref KartBody subsystem
const KartBody *KartObjectProxy::body() const {
    return m_accessor->body;
}

/// @addr{0x8059084C}
/// @brief Gets a pointer to the kart's @ref KartCollide subsystem
/// @ref A pointer to the kart's @ref KartCollide subsystem
KartCollide *KartObjectProxy::collide() {
    return m_accessor->collide;
}

/// @addr{0x8059084C}
/// @brief Gets a pointer to the kart's @ref KartCollide subsystem
/// @ref A const pointer to the kart's @ref KartCollide subsystem
const KartCollide *KartObjectProxy::collide() const {
    return m_accessor->collide;
}

/// @addr{0x805907D8}
/// @brief Gets a pointer to the kart's @ref CollisionGroup subsystem
/// @ref A pointer to the kart's @ref CollisionGroup subsystem
CollisionGroup *KartObjectProxy::collisionGroup() {
    return m_accessor->body->physics()->hitboxGroup();
}

/// @addr{0x805907D8}
/// @brief Gets a pointer to the kart's @ref CollisionGroup subsystem
/// @ref A const pointer to the kart's @ref CollisionGroup subsystem
const CollisionGroup *KartObjectProxy::collisionGroup() const {
    return m_accessor->body->physics()->hitboxGroup();
}

/// @addr{0x8059077C}
/// @brief Gets a pointer to the kart's @ref KartMove subsystem
/// @ref A pointer to the kart's @ref KartMove subsystem
KartMove *KartObjectProxy::move() {
    return m_accessor->move;
}

/// @addr{0x8059077C}
/// @brief Gets a pointer to the kart's @ref KartMove subsystem
/// @ref A const pointer to the kart's @ref KartMove subsystem
const KartMove *KartObjectProxy::move() const {
    return m_accessor->move;
}

/// @addr{0x80591904}
/// @brief Gets a pointer to the kart's @ref KartHalfPipe subsystem
/// @return A pointer to the kart's @ref KartHalfPipe subsystem
KartHalfPipe *KartObjectProxy::halfPipe() {
    return m_accessor->move->halfPipe();
}

/// @addr{0x80591904}
/// @brief Gets a pointer to the kart's @ref KartHalfPipe subsystem
/// @return A const pointer to the kart's @ref KartHalfPipe subsystem
const KartHalfPipe *KartObjectProxy::halfPipe() const {
    return m_accessor->move->halfPipe();
}

/// @addr{0x80591914}
/// @brief Gets a pointer to the kart's @ref KartJump subsystem
/// @return A pointer to the kart's @ref KartJump subsystem
KartJump *KartObjectProxy::jump() {
    return m_accessor->move->jump();
}

/// @addr{0x80591914}
/// @brief Gets a pointer to the kart's @ref KartJump subsystem
/// @return A const pointer to the kart's @ref KartJump subsystem
const KartJump *KartObjectProxy::jump() const {
    return m_accessor->move->jump();
}

/// @addr{0x80590864}
/// @brief Gets a pointer to the kart's @ref KartParam subsystem
/// @return A pointer to the kart's @ref KartParam subsystem
KartParam *KartObjectProxy::param() {
    return m_accessor->param;
}

/// @addr{0x80590864}
/// @brief Gets a pointer to the kart's @ref KartParam subsystem
/// @return A const pointer to the kart's @ref KartParam subsystem
const KartParam *KartObjectProxy::param() const {
    return m_accessor->param;
}

/// @addr{0x80590888}
/// @brief Gets a reference to the kart's @ref BSP data
/// @return A const reference to the kart's @ref BSP data
const BSP &KartObjectProxy::bsp() const {
    return param()->bsp();
}

/// @addr{0x805903AC}
/// @brief Gets a pointer to the kart's @ref KartPhysics subsystem
/// @return A pointer to the kart's @ref KartPhysics subsystem
KartPhysics *KartObjectProxy::physics() {
    return body()->physics();
}

/// @addr{0x805903AC}
/// @brief Gets a pointer to the kart's @ref KartPhysics subsystem
/// @return A const pointer to the kart's @ref KartPhysics subsystem
const KartPhysics *KartObjectProxy::physics() const {
    return body()->physics();
}

/// @addr{0x805903E0}
/// @brief Gets a pointer to the kart's @ref KartDynamics subsystem
/// @return A pointer to the kart's @ref KartDynamics subsystem
KartDynamics *KartObjectProxy::dynamics() {
    return physics()->dynamics();
}

/// @addr{0x805903E0}
/// @brief Gets a pointer to the kart's @ref KartDynamics subsystem
/// @return A const pointer to the kart's @ref KartDynamics subsystem
const KartDynamics *KartObjectProxy::dynamics() const {
    return physics()->dynamics();
}

/// @brief Gets a pointer to the kart's @ref KartState subsystem
/// @return A pointer to the kart's @ref KartState subsystem
KartState *KartObjectProxy::state() {
    return m_accessor->state;
}

/// @brief Gets a pointer to the kart's @ref KartState subsystem
/// @return A const pointer to the kart's @ref KartState subsystem
const KartState *KartObjectProxy::state() const {
    return m_accessor->state;
}

/// @addr{0x80590764}
/// @brief Gets a pointer to the kart's @ref KartSub subsystem
/// @return A pointer to the kart's @ref KartSub subsystem
KartSub *KartObjectProxy::sub() {
    return m_accessor->sub;
}

/// @addr{0x80590764}
/// @brief Gets a pointer to the kart's @ref KartSub subsystem
/// @return A const pointer to the kart's @ref KartSub subsystem
const KartSub *KartObjectProxy::sub() const {
    return m_accessor->sub;
}

/// @addr{0x805906B4}
/// @brief Gets a pointer to the kart's @ref KartSuspension subsystem for the given index
/// @param suspIdx The index of the suspension to get
/// @return A pointer to the kart's @ref KartSuspension subsystem for the given index
KartSuspension *KartObjectProxy::suspension(u16 suspIdx) {
    return m_accessor->suspensions[suspIdx];
}

/// @addr{0x805906B4}
/// @brief Gets a pointer to the kart's @ref KartSuspension subsystem for the given index
/// @param suspIdx The index of the suspension to get
/// @return A const pointer to the kart's @ref KartSuspension subsystem for the given index
const KartSuspension *KartObjectProxy::suspension(u16 suspIdx) const {
    return m_accessor->suspensions[suspIdx];
}

/// @addr{0x80590704}
/// @brief Gets a pointer to the kart's @ref KartSuspensionPhysics subsystem for the given index
/// @param suspIdx The index of the suspension physics to get
/// @return A pointer to the kart's @ref KartSuspensionPhysics subsystem for the given index
KartSuspensionPhysics *KartObjectProxy::suspensionPhysics(u16 suspIdx) {
    return m_accessor->suspensions[suspIdx]->suspPhysics();
}

/// @addr{0x80590704}
/// @brief Gets a pointer to the kart's @ref KartSuspensionPhysics subsystem for the given index
/// @param suspIdx The index of the suspension physics to get
/// @return A const pointer to the kart's @ref KartSuspensionPhysics subsystem for the given index
const KartSuspensionPhysics *KartObjectProxy::suspensionPhysics(u16 suspIdx) const {
    return m_accessor->suspensions[suspIdx]->suspPhysics();
}

/// @addr{0x805906DC}
/// @brief Gets a pointer to the kart's @ref KartTire subsystem for the given index
/// @param tireIdx The index of the tire to get
/// @return A pointer to the kart's @ref KartTire subsystem for the given index
KartTire *KartObjectProxy::tire(u16 tireIdx) {
    return m_accessor->tires[tireIdx];
}

/// @addr{0x805906DC}
/// @brief Gets a pointer to the kart's @ref KartTire subsystem for the given index
/// @param tireIdx The index of the tire to get
/// @return A const pointer to the kart's @ref KartTire subsystem for the given index
const KartTire *KartObjectProxy::tire(u16 tireIdx) const {
    return m_accessor->tires[tireIdx];
}

/// @addr{0x80590734}
/// @brief Gets a pointer to the kart's @ref WheelPhysics subsystem for the given tire index
/// @param tireIdx The index of the tire to get the wheel physics for
/// @return A pointer to the kart's @ref WheelPhysics subsystem for the given tire index
WheelPhysics *KartObjectProxy::tirePhysics(u16 tireIdx) {
    return tire(tireIdx)->wheelPhysics();
}

/// @addr{0x80590734}
/// @brief Gets a pointer to the kart's @ref WheelPhysics subsystem for the given tire index
/// @param tireIdx The index of the tire to get the wheel physics for
/// @return A const pointer to the kart's @ref WheelPhysics subsystem for the given tire index
const WheelPhysics *KartObjectProxy::tirePhysics(u16 tireIdx) const {
    return m_accessor->tires[tireIdx]->wheelPhysics();
}

/// @addr{0x8059081C}
/// @brief Gets a reference to the kart's @ref CollisionData for the kart's body
/// @return A reference to the kart's @ref CollisionData for the kart's body
CollisionData &KartObjectProxy::collisionData() {
    return physics()->hitboxGroup()->collisionData();
}

/// @addr{0x8059081C}
/// @brief Gets a reference to the kart's @ref CollisionData for the kart's body
/// @return A const reference to the kart's @ref CollisionData for the kart's body
const CollisionData &KartObjectProxy::collisionData() const {
    return m_accessor->body->physics()->hitboxGroup()->collisionData();
}

/// @addr{0x805903F4}
/// @brief Gets a pointer to the kart's @ref System::KPad subsystem
/// @return A const pointer to the kart's @ref System::KPad subsystem
const System::KPad *KartObjectProxy::inputs() const {
    return System::RaceManager::Instance()->player().inputs();
}

/// @addr{0x80590A40}
/// @brief Gets a pointer to the kart's @ref Render::KartModel subsystem
/// @return A pointer to the kart's @ref Render::KartModel subsystem
Render::KartModel *KartObjectProxy::model() {
    return m_accessor->model;
}

/// @addr{0x80590A40}
/// @brief Gets a pointer to the kart's @ref Render::KartModel subsystem
/// @return A const pointer to the kart's @ref Render::KartModel subsystem
const Render::KartModel *KartObjectProxy::model() const {
    return m_accessor->model;
}

/// @addr{0x805907C0}
/// @brief Gets a pointer to the kart's @ref Field::ObjectCollisionKart subsystem
/// @return A pointer to the kart's @ref Field::ObjectCollisionKart subsystem
Field::ObjectCollisionKart *KartObjectProxy::objectCollisionKart() {
    return m_accessor->objectCollisionKart;
}

/// @addr{0x805907C0}
/// @brief Gets a pointer to the kart's @ref Field::ObjectCollisionKart subsystem
/// @return A const pointer to the kart's @ref Field::ObjectCollisionKart subsystem
const Field::ObjectCollisionKart *KartObjectProxy::objectCollisionKart() const {
    return m_accessor->objectCollisionKart;
}

/// @addr{0x80591520}
/// @brief Gets a pointer to the kart's @ref Field::BoxColUnit subsystem
/// @return A pointer to the kart's @ref Field::BoxColUnit subsystem
Field::BoxColUnit *KartObjectProxy::boxColUnit() {
    return m_accessor->boxColUnit;
}

/// @addr{0x80591520}
/// @brief Gets a pointer to the kart's @ref Field::BoxColUnit subsystem
/// @return A const pointer to the kart's @ref Field::BoxColUnit subsystem
const Field::BoxColUnit *KartObjectProxy::boxColUnit() const {
    return m_accessor->boxColUnit;
}

/// @addr{0x80590834}
/// @brief Gets a reference to the kart's tire's @ref CollisionData for the given tire index
/// @param tireIdx The index of the tire to get the collision data for
/// @return A reference to the kart's tire's @ref CollisionData for the given tire index
CollisionData &KartObjectProxy::collisionData(u16 tireIdx) {
    return tirePhysics(tireIdx)->hitboxGroup()->collisionData();
}

/// @addr{0x80590834}
/// @brief Gets a reference to the kart's tire's @ref CollisionData for the given tire index
/// @param tireIdx The index of the tire to get the collision data for
/// @return A const reference to the kart's tire's @ref CollisionData for the given tire index
const CollisionData &KartObjectProxy::collisionData(u16 tireIdx) const {
    return m_accessor->tires[tireIdx]->wheelPhysics()->hitboxGroup()->collisionData();
}

/// @addr{0x805914BC}
/// @brief Gets a reference to the kart's scale vector from the kart's @ref KartMove subsystem
/// @return A const reference to the kart's scale vector from the kart's @ref Kart
const EGG::Vector3f &KartObjectProxy::scale() const {
    return move()->scale();
}

/// @addr{0x80590264}
/// @brief Gets a reference to the kart's pose matrix from the kart's @ref KartPhysics subsystem
/// @return A const reference to the kart's pose matrix from the kart's @ref Kart
const EGG::Matrix34f &KartObjectProxy::pose() const {
    return physics()->pose();
}

/// @addr{0x80590C44}
/// @brief Computes the right direction vector of the kart's body from the kart's pose matrix
/// @return The right direction vector of the kart's body
EGG::Vector3f KartObjectProxy::bodyRight() const {
    const EGG::Matrix34f &mtx = pose();
    return EGG::Vector3f(mtx[0, 0], mtx[1, 0], mtx[2, 0]);
}

/// @addr{0x80590C6C}
/// @brief Computes the up direction vector of the kart's body from the kart's pose matrix
/// @return The up direction vector of the kart's body
EGG::Vector3f KartObjectProxy::bodyUp() const {
    const EGG::Matrix34f &mtx = pose();
    return EGG::Vector3f(mtx[0, 1], mtx[1, 1], mtx[2, 1]);
}

/// @addr{0x80590C94}
/// @brief Computes the forward direction vector of the kart's body from the kart's pose matrix
/// @return The forward direction vector of the kart's body
EGG::Vector3f KartObjectProxy::bodyForward() const {
    const EGG::Matrix34f &mtx = pose();
    return EGG::Vector3f(mtx[0, 2], mtx[1, 2], mtx[2, 2]);
}

/// @addr{0x80590CBC}
/// @brief Gets the local X-axis direction vector from the kart's @ref KartPhysics subsystem
/// @return Const reference to the local X-axis direction vector of the kart
const EGG::Vector3f &KartObjectProxy::componentXAxis() const {
    return physics()->xAxis();
}

/// @addr{0x80590CD0}
/// @brief Gets the local Y-axis direction vector from the kart's @ref KartPhysics subsystem
/// @return Const reference to the local Y-axis direction vector of the kart
const EGG::Vector3f &KartObjectProxy::componentYAxis() const {
    return physics()->yAxis();
}

/// @addr{0x80590CE4}
/// @brief Gets the local Z-axis direction vector from the kart's @ref KartPhysics subsystem
/// @return Const reference to the local Z-axis direction vector of the kart
const EGG::Vector3f &KartObjectProxy::componentZAxis() const {
    return physics()->zAxis();
}

/// @addr{0x8059020C}
/// @brief Gets the kart's position from the kart's @ref KartDynamics subsystem
/// @return Const reference to the kart's position vector
const EGG::Vector3f &KartObjectProxy::pos() const {
    return dynamics()->pos();
}

/// @addr{0x80590224}
/// @brief Gets the kart's previous position from the kart's @ref KartPhysics subsystem
/// @return Const reference to the kart's previous position vector
const EGG::Vector3f &KartObjectProxy::prevPos() const {
    return physics()->pos();
}

/// @brief Gets the kart's main rotation quaternion from the kart's @ref KartDynamics subsystem
/// @return Const reference to the kart's main rotation quaternion
const EGG::Quatf &KartObjectProxy::mainRot() const {
    return dynamics()->mainRot();
}

/// @brief Gets the kart's full rotation quaternion from the kart's @ref KartDynamics subsystem
/// @return Const reference to the kart's full rotation quaternion
const EGG::Quatf &KartObjectProxy::fullRot() const {
    return dynamics()->fullRot();
}

/// @brief Gets the kart's main rotation quaternion from the kart's @ref KartDynamics subsystem
/// @return Const reference to the kart's main rotation quaternion
const EGG::Vector3f &KartObjectProxy::extVel() const {
    return dynamics()->extVel();
}

/// @brief Gets the kart's internal velocity vector from the kart's @ref KartDynamics subsystem
/// @return Const reference to the kart's internal velocity vector
const EGG::Vector3f &KartObjectProxy::intVel() const {
    return dynamics()->intVel();
}

/// @brief Gets the kart's overall velocity vector from the kart's @ref KartDynamics subsystem
/// @return Const reference to the kart's overall velocity vector
const EGG::Vector3f &KartObjectProxy::velocity() const {
    return dynamics()->velocity();
}

/// @addr{0x80590CF8}
/// @brief Gets the kart's speed from the kart's @ref KartMove subsystem
/// @return The kart's speed
f32 KartObjectProxy::speed() const {
    return move()->speed();
}

/// @brief Gets the kart's acceleration from the kart's @ref KartMove subsystem
/// @return The kart's acceleration
f32 KartObjectProxy::acceleration() const {
    return move()->acceleration();
}

/// @brief Gets the kart's soft speed limit from the kart's @ref KartMove subsystem
/// @return The kart's soft speed limit
f32 KartObjectProxy::softSpeedLimit() const {
    return move()->softSpeedLimit();
}

/// @brief Gets the kart's angVel2 vector from the kart's @ref KartDynamics subsystem
/// @return Const reference to the kart's angVel2 vector
/// @rename
const EGG::Vector3f &KartObjectProxy::angVel2() const {
    return dynamics()->angVel2();
}

/// @addr{0x80590A6C}
/// @brief Checks if the kart is a bike based on its parameters
/// @return True if the kart is a bike, false otherwise
bool KartObjectProxy::isBike() const {
    return param()->isBike();
}

/// @addr{0x805902DC}
/// @brief Gets the number of suspensions in the kart from the kart's @ref KartParam subsystem
/// @return The number of suspensions in the kart
u16 KartObjectProxy::suspCount() const {
    return param()->suspCount();
}

/// @addr{0x805902EC}
/// @brief Gets the number of tires in the kart from the kart's @ref KartParam subsystem
/// @return The number of tires in the kart
u16 KartObjectProxy::tireCount() const {
    return param()->tireCount();
}

/// @addr{0x80590338}
/// @brief Checks if the given wheel physics has floor collision based on its @ref CollisionData
/// @param wheelPhysics Pointer to the wheel physics to check for floor collision
bool KartObjectProxy::hasFloorCollision(const WheelPhysics *wheelPhysics) const {
    return wheelPhysics->hitboxGroup()->collisionData().bFloor;
}

/// @addr{0x8058539C}
/// @brief While in a cannon, computes the lateral position offset between the kart and the cannon's
/// center and computes the cannon's direction
/// @return A pair containing the lateral position offset and direction of the cannon
std::pair<EGG::Vector3f, EGG::Vector3f> KartObjectProxy::getCannonPosRot() {
    auto *cannon = System::CourseMap::Instance()->getCannonPoint(state()->cannonPointId());
    const EGG::Vector3f &cannonPos = cannon->pos();

    EGG::Matrix34f rotMat;
    rotMat.makeR(cannon->rot() * DEG2RAD);

    EGG::Vector3f forwardDir = rotMat.multVector33(EGG::Vector3f::ez);
    EGG::Vector3f distance_to_cannon = this->pos() - cannonPos;
    distance_to_cannon.y = 0.0f;

    EGG::Vector3f right = EGG::Vector3f::ey.cross(forwardDir);
    return std::pair(cannonPos + right * right.dot(distance_to_cannon), forwardDir);
}

/// @addr{0x80590DD0}
/// @brief Gets the kart's speed ratio from the kart's @ref KartMove subsystem
/// @return The kart's speed ratio
f32 KartObjectProxy::speedRatio() const {
    return move()->speedRatio();
}

/// @addr{0x80590DC0}
/// @brief Gets the kart's capped speed ratio from the kart's @ref KartMove subsystem
/// @return The kart's capped speed ratio
f32 KartObjectProxy::speedRatioCapped() const {
    return move()->speedRatioCapped();
}

/// @addr{0x805914F4}
/// @brief Checks if the kart is currently in a respawn state based on its @ref KartMove subsystem
/// @return True if the kart is in a respawn state, false otherwise
bool KartObjectProxy::isInRespawn() const {
    return move()->respawnTimer() > 0 || move()->respawnPostLandTimer() > 0;
}

/// @addr{0x805911A8}
/// @brief Gets the @ref Field::KCLTypeMask of the closest colliding wall
/// @return The @ref Field::KCLTypeMask of the closest colliding wall
Field::KCLTypeMask KartObjectProxy::wallKclType() const {
    return collisionData().closestWallFlags;
}

/// @addr{0x805911C0}
/// @brief Gets the KCL variant of the closest colliding wall
/// @return The KCL variant of the closest colliding wall
u32 KartObjectProxy::wallKclVariant() const {
    return collisionData().closestWallSettings;
}

/// @brief Gets a reference to the kart's @ref Status subsystem
/// @return A reference to the kart's @ref Status subsystem
Status &KartObjectProxy::status() {
    return state()->status();
}

/// @brief Gets a reference to the kart's @ref Status subsystem
/// @return A const reference to the kart's @ref Status subsystem
const Status &KartObjectProxy::status() const {
    return state()->status();
}

/// @addr{0x8059031C}
/// @brief Gets the position of the kart's wheel at the given index from the kart's @ref
/// WheelPhysics subsystem
/// @param idx The index of the wheel to get the position of
/// @return A const reference to the position of the kart's wheel at the given index
const EGG::Vector3f &KartObjectProxy::wheelPos(u16 idx) const {
    return tirePhysics(idx)->pos();
}

/// @addr{0x80590390}
/// @brief Gets the position of the edge of the kart's wheel at the given index from the kart's @ref
/// WheelPhysics subsystem
/// @param idx The index of the wheel to get the edge position of
/// @return A const reference to the position of the edge of the kart's wheel at the given index
const EGG::Vector3f &KartObjectProxy::wheelEdgePos(u16 idx) const {
    return tirePhysics(idx)->wheelEdgePos();
}

/// @addr{0x805909C8}
/// @brief Gets the camera Y-axis distance for the kart based on its parameters
/// @return The camera Y-axis distance for the kart
f32 KartObjectProxy::cameraDistY() const {
    if (isBike()) {
        return param()->bikeDisp().m_cameraDistY;
    } else {
        return param()->kartDisp().m_cameraDistY;
    }
}

/// @addr{0x805909F4}
/// @brief Gets the X-axis hop direction input from the kart's @ref KartMove subsystem
/// @return The X-axis hop direction input
s32 KartObjectProxy::hopStickX() const {
    return move()->hopStickX();
}

/// @addr{0x80590A10}
/// @brief Gets the @param KartParam::Stats::DriftType of the kart from its parameters
/// @return The @param KartParam::Stats::DriftType of the kart
KartParam::Stats::DriftType KartObjectProxy::vehicleType() const {
    return param()->stats().driftType;
}

/// @addr{0x805901D0}
/// @brief Copies over the @ref KartAccessor pointers from the kart object at the given index
/// @param idx The index of the kart object to copy the @ref KartAccessor pointers from
void KartObjectProxy::apply(size_t idx) {
    m_accessor = KartObjectManager::Instance()->object(idx)->accessor();
}

/// @addr{0x80590138}
/// @brief For all proxies in the static list, synchronizes all pointers to the KartAccessor.
/// @param pointers The pointer to synchronize all other proxies to.
void KartObjectProxy::ApplyAll(const KartAccessor *pointers) {
    for (auto iter = s_proxyList.begin(); iter != s_proxyList.end(); ++iter) {
        (*iter)->m_accessor = pointers;
    }
}

alloc_list<KartObjectProxy *> KartObjectProxy::s_proxyList; ///< @addr{0x809C1900}

} // namespace Kinoko::Kart
