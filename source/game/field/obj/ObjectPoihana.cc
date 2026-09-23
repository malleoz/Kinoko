#include "ObjectPoihana.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x80747248}
/// @copybrief ObjectBase::init()
/// @details Initializes @ref m_workMat to the identity matrix. Sets @ref m_accel, @ref m_extVel,
/// and @ref m_vel to the zero vector. Initializes @ref m_up and @ref m_floorNrm to the default up
/// vector and @ref m_forward to the default forward vector. Initializes @ref m_walkState to @ref
/// WalkState::NeedTarget so that the Cataquack will fetch its first walking target. Finally,
/// initializes the position component of @ref m_workMat to the Cataquack's initial position.
void ObjectPoihanaBase::init() {
    m_workMat = EGG::Matrix34f::ident;
    m_accel.setZero();
    m_extVel.setZero();
    m_vel.setZero();
    m_up = EGG::Vector3f::ey;
    m_forward = EGG::Vector3f::ez;
    m_floorNrm = EGG::Vector3f::ey;
    m_walkState = WalkState::NeedTarget;
    m_workMat.setBase(3, pos());
}

/// @addr{0x80748958}
/// @copybrief ObjectBase::init()
/// @details Calls the base class initialization. Caches the Cataquack's initial position to @ref
/// m_initPos. Sets the initial @ref m_dir to the default forward vector. Clears @ref m_targetVel.
/// Initializes @ref m_accel with a downward gravitational force of `1.2f`. Sets the Cataquack's
/// @ref m_radius to `100.0f` and its @ref m_heightOffset to `-160.0f. Finally, transitions the
/// Cataquack to the walk state.
void ObjectPoihana::init() {
    constexpr f32 GRAVITY = -1.2f;

    ObjectPoihanaBase::init();

    m_initPos = curPos();
    m_dir = EGG::Vector3f::ez;
    m_targetVel.setZero();
    m_accel = EGG::Vector3f(0.0f, GRAVITY, 0.0f);
    m_radius = 100.0f;
    m_heightOffset = -160.0f;
    m_nextStateId = 0;
}

/// @addr{0x807475DC}
/// @brief Re-orientates the Cataquack so that its side and forward vectors are orthogonal to its up
/// vector
/// @details Forms an orthonormal basis from the up and forward vectors, and updates the current
/// transformation matrix accordingly
void ObjectPoihana::calcOrthonormalBasis() {
    EGG::Vector3f side = m_up.cross(m_forward);
    side.normalise2();

    if (side.squaredLength() <= std::numeric_limits<f32>::epsilon()) {
        side = EGG::Vector3f::ex;
    }

    EGG::Vector3f forward = side.cross(m_up);
    forward.normalise2();

    m_workMat.setBase(0, side);
    m_workMat.setBase(1, m_up);
    m_workMat.setBase(2, forward);
}

/// @addr{0x80747788}
/// @brief Checks for floor and wall collision and updates the transformation matrix and velocity
/// accordingly
/// @details If a floor or wall collision occurs, offsets the Cataquack's position outside of the
/// floor or wall. If the collision was with a wall, applies an additional force to @ref m_extVel to
/// push the Cataquack further away from the wall. If the collision was with a floor, clears @ref
/// m_extVel and caches the floor normal to @ref m_floorNrm so that the Cataquack can re-orient
/// itself.
void ObjectPoihana::calcCollision() {
    CollisionInfo info;
    EGG::Vector3f pos = collisionPos();

    bool hasCol = CollisionDirector::Instance()->checkSphereFull(m_radius, pos, EGG::Vector3f::inf,
            KCL_TYPE_64EBDFFF, &info, nullptr, 0);

    if (!hasCol) {
        return;
    }

    m_workMat.setBase(3, m_workMat.base(3) + info.tangentOff);

    if (info.wallDist > -std::numeric_limits<f32>::min()) {
        m_extVel += Project(m_extVel, info.wallNrm) * -2.0f;
    }

    if (info.floorDist > -std::numeric_limits<f32>::min()) {
        m_floorNrm = info.floorNrm;
        m_extVel.setZero();
    }
}

/** @addr{0x80748D98}
 * @brief Manages the cataquack's walking behavior, including selecting a target position and
 * updating its velocity
 * @details When the cataquack needs a new target position, it generates a pseudo-random position
 * as follows:
 * 1. Build a position-based seed. \n
 *    \f$ s = \left|x_{\mathrm{pos}} + z_{\mathrm{pos}}\right| \f$ \n
 *    This collapses world position into one non-negative scalar.
 * 2. Quantize the seed at two different scales. \n
 *    \f$ t_1 = \left\lfloor 0.1\,s \right\rfloor,\quad t_2 = \left\lfloor 100\,s \right\rfloor \f$
 * 3. Fold both quantized values into a small periodic range. \n
 *    \f$ r = (t_1 \bmod 100) + (t_2 \bmod 100) \f$ \n
 *    This gives a bounded integer used as the main pseudo-random driver.
 * 4. Derive normalized X/Z components in [0, 1). \n
 *    \f$ x_u = \frac{r \bmod 100}{100},\quad z_u = \frac{(7r) \bmod 100}{100} \f$ \n
 *    The factor 7 decorrelates the Z component from X.
 * 5. Convert normalized values into a unit direction on the XZ plane. \n
 *    \f$ \mathbf{d} = \mathrm{normalize}(x_u - 0.5,\ 0,\ z_u - 0.5) \f$ \n
 *    Subtracting 0.5 recenters around the origin before normalization.
 * 6. Compute a travel radius in [1000, 2000). \n
 *    \f$ \mathrm{dist} = 1000 + 1000\cdot\frac{(3r)\bmod 100}{100} \f$ \n
 *    The factor 3 provides another decorrelated component for distance.
 * 7. Build the final target position around the initial spawn. \n
 *    \f$ \mathbf{target} = \mathbf{initPos} + \mathbf{d}\cdot\mathrm{dist} \f$
 *
 * If the Cataquack has a target position, it walks towards the target with a speed of `7.0f`. Once
 * it walks within `200.0f` units of the target position, it will fetch a new target position next
 * frame.
 *
 * Finally, updates the Cataquack's up and forward vectors, and interpolates its @ref m_vel towards
 * @ref m_targetVel.
 **/
void ObjectPoihana::calcStep() {
    constexpr f32 COARSE_SCALAR = 0.1f;
    constexpr f32 FINE_SCALAR = 100.0f;
    constexpr f32 CLAMP = 0.5f;
    constexpr s32 MOD = 100;
    constexpr f32 INV_MOD = 1.0f / static_cast<f32>(MOD);
    constexpr f32 SPEED = 7.0f;
    constexpr f32 RAND_POS_MAX = 2000.0f;
    constexpr f32 RAND_POS_MIN = 1000.0f;
    constexpr f32 RAND_POS_RANGE = RAND_POS_MAX - RAND_POS_MIN;
    constexpr f32 TARGET_DIST_THRESHOLD = 200.0f;
    constexpr f32 UP_INTERP_RATE = 0.1f;
    constexpr f32 VEL_INTERP_RATE = 0.05f;

    if (m_walkState == WalkState::NeedTarget) {
        // psuedo-random number generator based on current position of the Poihana
        EGG::Vector3f pos = curPos();
        f32 sum = EGG::Mathf::abs(pos.x + pos.z);
        s32 tmp1 = static_cast<s32>(COARSE_SCALAR * sum);
        s32 tmp2 = static_cast<s32>(FINE_SCALAR * sum);
        s32 remSum = (tmp1 % MOD) + (tmp2 % MOD);
        f32 x = INV_MOD * static_cast<f32>(remSum % MOD);
        f32 z = INV_MOD * static_cast<f32>(remSum * 7 % MOD);

        EGG::Vector3f dir = EGG::Vector3f(x - CLAMP, 0.0f, z - CLAMP);
        dir.normalise2();

        f32 scalar = RAND_POS_RANGE * (INV_MOD * static_cast<f32>(remSum * 3 % MOD)) + RAND_POS_MIN;
        m_targetPos = m_initPos + dir * scalar;

        calcDir();

        m_walkState = WalkState::HasTarget;
    }

    if (m_walkState == WalkState::HasTarget) {
        EGG::Vector3f dir = m_targetPos - curPos();
        dir.y = 0.0f;

        f32 len = 0.0f;
        if (dir.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            len = dir.normalise();
        }

        m_targetVel = dir * SPEED;

        if (len < TARGET_DIST_THRESHOLD) {
            m_walkState = WalkState::NeedTarget;
        }
    }

    calcUp(UP_INTERP_RATE);
    calcForward();

    m_vel = Interpolate(VEL_INTERP_RATE, m_vel, m_targetVel);
}

} // namespace Kinoko::Field
