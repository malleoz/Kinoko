#include "ObjectTruckWagon.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/RailManager.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x806E2624}
/// @copybrief ObjectBase::calcCollisionTransform()
/// @details Applies a vertical offset to the minecart's collision transform based on whether it is
/// rolling on the floor or is suspended mid-air.
void ObjectTruckWagonCart::calcCollisionTransform() {
    constexpr f32 OFFSET_SUSPENDED = 500.0f;
    constexpr f32 OFFSET_ROLLING = 100.0f;

    auto *col = collision();
    if (!col || !m_active) {
        return;
    }

    f32 yOffset = m_currentStateId == 1 ? OFFSET_SUSPENDED : OFFSET_ROLLING;
    EGG::Matrix34f mat = EGG::Matrix34f::zero;
    mat.makeT(EGG::Vector3f(0.0f, yOffset, 0.0f));

    calcTransform();
    col->transform(transform().multiplyTo(mat), scale(), m_vel);
}

/// @addr{0x806E0650}
/// @copybrief ObjectCollidable::onCollision()
/// @param kartObj The kart object involved in the collision
/// @param reactionOnKart The reaction that should be applied to the kart upon collision
/// @return @ref Kart::Reaction::Wall if the kart's speed is below 50%, otherwise @ref
/// Kart::Reaction::Sideways.
Kart::Reaction ObjectTruckWagonCart::onCollision(Kart::KartObject *kartObj,
        Kart::Reaction reactionOnKart, Kart::Reaction /*reactionOnObj*/,
        EGG::Vector3f & /*hitDepth*/) {
    return kartObj->speedRatioCapped() < 0.5f ? Kart::Reaction::Wall : reactionOnKart;
}

/// @addr{0x806E0DF0}
/// @brief Runs every frame the minecart is rolling along the floor
/// @details This function's behavior diverges based on whether the minecart is between rail nodes 3
/// and 4.
///
/// If the minecart is between rail nodes 3 and 4, then the minecart will fall until it collides
/// with the floor. This special logic exists to avoid unnatural-looking movement in Wario's Gold
/// Mine when the minecart falls shortly after the spawner.  When falling, the minecart's motion has
/// an initial downward velocity of `15.0f`. Every frame the minecart is not colliding with the
/// floor, an additional downward gravitational force of `2.0f` is applied. If a collision did
/// occur, the Y-component of the minecart's @ref m_vel is reset to zero, the minecart is
/// repositioned to rest on the floor, and the minecart's orientation vectors (@ref m_up and @ref
/// m_tangent) are interpolated towards the floor's orientation and the rail interpolator's tangent
/// direction. Finally, the minecart's transformation matrix is updated to reflect its new position
/// and orientation.
///
/// If the minecart is between any other nodes, its position is directly set based on the rail
/// interpolator's current position. The minecart's orientation vectors (@ref m_up and @ref
/// m_tangent) are then interpolated towards the rail's pre-computed floor normal and the rail
/// interpolator's tangent direction, and the minecart's transformation matrix is updated
/// accordingly.
/// @warning It is not clear why @ref ObjectTruckWagon::ObjectTruckWagon() conditionally calls @ref
/// Rail::checkSphereFull() only if `rail->pointCount() > 40`, but as a result, minecart spawners
/// having a rail with fewer than 41 nodes will cause the game to crash on course load. @ref
/// Rail::checkSphereFull() allocates an array of floor normals for each node of the rail, which
/// this function will then index into. If this array is not allocated, then this will result in a
/// nullptr dereference.
void ObjectTruckWagonCart::calcRolling() {
    constexpr f32 RADIUS = 50.0f;
    constexpr f32 GRAVITY = 2.0f;
    constexpr f32 INITIAL_FALL_VEL = 15.0f;
    constexpr f32 INTERP_RATE = 0.1f;

    const EGG::Vector3f &railPos = m_railInterpolator->curPos();
    if (m_railInterpolator->curPointIdx() != 3) {
        setPos(railPos);

        m_up = Interpolate(INTERP_RATE, m_up,
                m_railInterpolator->floorNrm(m_railInterpolator->nextPointIdx()));
        m_tangent = Interpolate(INTERP_RATE, m_tangent, m_railInterpolator->curTangentDir());
        m_up.normalise2();
        m_tangent.normalise2();

        setMatrixTangentTo(m_up, m_tangent);

        return;
    }

    setPos(EGG::Vector3f(railPos.x, pos().y - INITIAL_FALL_VEL, railPos.z));

    CollisionInfo info;
    EGG::Vector3f colPos = pos() + EGG::Vector3f(0.0f, RADIUS, 0.0f);

    EGG::Vector3f floorNrm = m_up;
    EGG::Vector3f tangent = m_tangent;

    bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, colPos, EGG::Vector3f::inf,
            KCL_TYPE_FLOOR, &info, nullptr, 0);

    if (hasCol) {
        m_vel.y = 0.0f;
        addPos(info.tangentOff);

        if (info.floorDist > -std::numeric_limits<f32>::min()) {
            floorNrm = info.floorNrm;
        }

        tangent = m_railInterpolator->curTangentDir();
    } else {
        m_vel.y -= GRAVITY;
        setPos(EGG::Vector3f(pos().x, m_vel.y + pos().y, pos().z));
    }

    m_up = Interpolate(INTERP_RATE, m_up, floorNrm);
    m_tangent = Interpolate(INTERP_RATE, m_tangent, tangent);
    m_up.normalise2();
    m_tangent.normalise2();

    setMatrixTangentTo(m_up, m_tangent);
}

/// @addr{0x806E1370}
/// @brief Runs every frame that the minecart is suspended mid-air
/// @details Updates the minecart's @ref m_angVel based on its current pitch, the change in XZ-plane
/// velocity this frame, a spring stiffness factor, a pitch inertia factor, and a damping factor.
/// Increments @ref m_pitch based on the updated angular velocity. Interpolates @ref m_up towards
/// the up vector rotated based on the current pitch, and @ref m_tangent towards the rail's tangent
/// direction. Finally, updates the minecart's transformation matrix based on the interpolated
/// orientation vectors, updates the minecart's position based on the interpolated orientation
/// vectors with a vertical offset of `710.0f`, and caches the current @ref m_vel to @ref m_lastVel.
void ObjectTruckWagonCart::calcSuspended() {
    constexpr f32 INTERP_RATE = 0.1f;
    constexpr EGG::Vector3f INITIAL_OFFSET = EGG::Vector3f(0.0f, 710.0f, 0.0f);

    // Controls how strongly the cart tries to restore to neutral (damped harmonic oscillator)
    constexpr f32 SPRING_STIFFNESS = 3.9f;
    constexpr f32 PITCH_INERTIA = 1300.0f;
    constexpr f32 ANG_VEL_DECAY = 0.998f;

    EGG::Vector2f lastVelXZ = EGG::Vector2f(m_lastVel.x, m_lastVel.z);
    EGG::Vector2f velXZ = EGG::Vector2f(m_vel.x, m_vel.z);
    f32 velMagDiff = EGG::Mathf::sqrt((velXZ - lastVelXZ).dot());
    velMagDiff = velXZ.cross(lastVelXZ) > 0.0f ? -velMagDiff : velMagDiff;
    f32 cos = velMagDiff * EGG::Mathf::CosFIdx(RAD2FIDX * m_pitch);
    f32 sin = EGG::Mathf::SinFIdx(RAD2FIDX * m_pitch);
    m_angVel = (m_angVel + (cos + -SPRING_STIFFNESS * sin) / PITCH_INERTIA) * ANG_VEL_DECAY;
    m_pitch += m_angVel;

    EGG::Vector3f tanXZ = m_railInterpolator->curTangentDir();
    tanXZ.y = 0.0f;
    tanXZ.normalise2();
    EGG::Matrix34f mat;
    mat.setAxisRotation(m_pitch, tanXZ);
    mat.setBase(3, EGG::Vector3f::zero);

    m_up = Interpolate(INTERP_RATE, m_up, mat.ps_multVector(EGG::Vector3f::ey));
    m_tangent = Interpolate(INTERP_RATE, m_tangent, tanXZ);
    m_tangent.y = 0.0f;

    m_up.normalise2();
    m_tangent.normalise2();
    EGG::Vector3f cross = m_up.cross(m_tangent);
    cross.normalise2();

    mat.setBase(0, cross);
    mat.setBase(1, m_up);
    mat.setBase(2, m_tangent);
    mat.setBase(3, EGG::Vector3f::zero);

    EGG::Matrix34f transMat = mat;
    transMat.setBase(3, pos());
    setTransform(transMat);

    setPos(m_railInterpolator->curPos() + INITIAL_OFFSET - mat.ps_multVector(INITIAL_OFFSET));
    m_lastVel = m_vel;
}

/// @addr{0x806E01C0}
/// @brief Resets the minecart to its initial state along the rail at the given rail point index
/// @param idx The index of the rail point to reset the minecart to
/// @details Initializes the minecart's rail interpolator to the specified rail point index and sets
/// @ref m_speed and @ref m_vel based on the rail's tangent direction at that point. Resets @ref
/// m_lastVel to the zero vector. If the minecart is not in the suspended state and the current rail
/// point's second setting is in the range `[0, 2]`, sets @ref m_nextStateId to that setting so that
/// the minecart transitions to the rolling, suspended, or "state2" state respectively. Finally, the
/// orientation vectors (@ref m_up and @ref m_tangent) are initialized to their default values, and
/// @ref m_pitch and @ref m_angVel are reset to zero.
void ObjectTruckWagonCart::reset(u32 idx) {
    m_railInterpolator->init(0.0f, idx);
    m_railInterpolator->setPerPointVelocities(true);

    setPos(m_railInterpolator->curPos());
    m_speed = m_railInterpolator->speed();
    m_vel = m_railInterpolator->curTangentDir() * m_speed;
    m_lastVel.setZero();

    if (m_currentStateId != 1) {
        u16 setting = m_railInterpolator->curPoint().setting[1];
        if (setting <= 2) {
            m_nextStateId = setting;
        }
    }

    m_up = EGG::Vector3f::ey;
    m_tangent = EGG::Vector3f::ez;
    m_pitch = 0.0f;
    m_angVel = 0.0f;
}

/// @addr{0x806E206C}
/// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
/// @param params The parameters used to initialize the object
/// @details Sets @ref m_spawn2Frame based on param setting 2 and @ref m_spawn1Frame based on
/// param setting 3. Minecart spawners having param setting 4 set to a non-zero value spawn low LOD
/// minecarts in the background which do not have any tangible collision, thus we can ignore them in
/// Kinoko. Spawns 12 minecart objects and loads them. If the spawner's rail path contains 40 or
/// more segments, then calls @ref Rail::checkSphereFull() to cache floor normals at each node of
/// the rail.
/// @warning It is not clear why the `rail->pointCount() > 40` check is used, but as a result,
/// minecart spawners having a rail with fewer than 41 nodes will cause the game to crash on course
/// load. @ref Rail::checkSphereFull() allocates an array of floor normals for each node of the
/// rail, which @ref ObjectTruckWagonCart::calcRolling() will then index into. If this array is not
/// allocated, then this will result in a nullptr dereference.
ObjectTruckWagon::ObjectTruckWagon(const System::MapdataGeoObj &params)
    : ObjectCollidable(params),
      m_spawn2Frame(static_cast<s32>(params.setting(1))),
      m_spawn1Frame(static_cast<s32>(params.setting(2))) {
    constexpr u32 CART_COUNT = 12;

    // For now, we don't care about low LOD minecarts since they don't have collision.
    if (params.setting(3) > 0) {
        return;
    }

    m_carts = owning_span<ObjectTruckWagonCart *>(CART_COUNT);

    for (auto *&cart : m_carts) {
        cart = EGG::egg_new<ObjectTruckWagonCart>(params);
        cart->load();
    }

    auto *rail = RailManager::Instance()->rail(params.pathId());
    ASSERT(rail);

    if (rail->pointCount() > 40) {
        rail->checkSphereFull();
    }
}

/// @addr{0x806E222C}
/// @copybrief ObjectBase::init()
/// @details Since we do not implement low LOD minecarts in Kinoko, we early return if the @ref
/// m_carts array is empty. For each minecart, ensures that the cart is deactivated before the
/// spawner's cycle begins. The second half of the minecarts are then activated and positioned along
/// the rail equidistantly. Finally, @ref m_cycleFrame and @ref m_curCartIdx are reset to zero.
void ObjectTruckWagon::init() {
    // If this spawner is for low LOD carts, then we need to skip init since the span is empty
    if (m_carts.empty()) {
        return;
    }

    for (auto *&cart : m_carts) {
        if (cart->isActive()) {
            cart->deactivate();
        }
    }

    u16 ptCount = m_carts[0]->railInterpolator()->pointCount();
    u32 cartCount = m_carts.size();
    u32 halfCount = m_carts.size() / 2;

    for (u32 i = halfCount; i < cartCount; ++i) {
        auto *&cart = m_carts[i];
        cart->reset(ptCount / halfCount * (cartCount - i));
        cart->setActive(true);
        cart->loadAABB(0.0f);
    }

    m_cycleFrame = 0;
    m_curCartIdx = 0;
}

/// @addr{0x806E23A4}
/// @copybrief ObjectBase::calc()
/// @details Checks to see if the current frame in the spawner's cycle corresponds to a kart
/// spawning. A kart spawns at the start of the spawner's cycle and at @ref m_spawn2Frame frames
/// after the start of the cycle. The minecart corresponding to @ref m_curCartIdx is spawned via a
/// call to @ref ObjectTruckWagonCart::activate(). @ref m_curCartIdx is incremented modulo the total
/// number of carts. Regardless of whether a minecart spawned, @ref m_cycleFrame increments,
/// wrapping around once it exceeds the sum of @ref m_spawn2Frame and @ref m_spawn1Frame.
void ObjectTruckWagon::calc() {
    if (m_carts.empty()) {
        return;
    }

    if (m_cycleFrame == m_spawn2Frame || m_cycleFrame == m_spawn2Frame + m_spawn1Frame) {
        m_carts[m_curCartIdx]->activate();
        m_curCartIdx = (m_curCartIdx + 1) % m_carts.size();
    }

    m_cycleFrame = m_cycleFrame % (m_spawn2Frame + m_spawn1Frame) + 1;
}

} // namespace Kinoko::Field
