#include "ObjectPoihana.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x80747198}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectPoihanaBase::ObjectPoihanaBase(const System::MapdataGeoObj &params)
    : StateManager(this, {}), ObjectCollidable(params), m_walkState(WalkState::NeedTarget),
      m_heightOffset(0.0f), m_radius(150.0f), m_targetPos(EGG::Vector3f::zero) {}

/// @addr{0x80747208}
/// @brief Default virtual destructor
ObjectPoihanaBase::~ObjectPoihanaBase() = default;

/// @addr{0x80747248}
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

/// @addr{0x8074816C}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectPoihana::ObjectPoihana(const System::MapdataGeoObj &params)
    : StateManager(this, STATE_ENTRIES), ObjectPoihanaBase(params) {}

/// @addr{0x807488BC}
/// @brief Default virtual destructor
ObjectPoihana::~ObjectPoihana() = default;

/// @addr{0x80748958}
void ObjectPoihana::init() {
    ObjectPoihanaBase::init();

    m_initPos = curPos();
    m_dir = EGG::Vector3f::ez;
    m_targetVel.setZero();
    m_accel = EGG::Vector3f(0.0f, -1.2f, 0.0f);
    m_radius = 100.0f;
    m_heightOffset = -160.0f;
    m_nextStateId = 0;
}

/// @addr{0x80747530}
/// @brief Interpolates the up vector between itself and the floor normal using the provided
/// interpolation factor
void ObjectPoihana::calcUp(f32 t) {
    m_up = Interpolate(t, m_up, m_floorNrm);
    if (m_up.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_up.normalise2();
    } else {
        m_up = EGG::Vector3f::ey;
    }
}

/// @addr{0x807475DC}
/// @brief Forms an orthonormal basis from the up and forward vectors, and updates the current
/// transformation matrix accordingly
void ObjectPoihana::calcOrthonormalBasis() {
    EGG::Vector3f side = m_up.cross(m_forward);
    side.normalise2();

    if (side.squaredLength() <= std::numeric_limits<f32>::epsilon()) {
        side = EGG::Vector3f::ex;
    }

    EGG::Vector3f up = side.cross(m_up);
    up.normalise2();

    m_workMat.setBase(0, side);
    m_workMat.setBase(1, m_up);
    m_workMat.setBase(2, up);
}

/// @addr{0x80747788}
/// @brief Checks for floor collision and updates the transformation matrix and velocity accordingly
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
        EGG::Vector3f delta = m_targetPos - curPos();
        delta.y = 0.0f;

        f32 len = 0.0f;
        if (delta.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            len = delta.normalise();
        }

        m_targetVel = delta * SPEED;

        if (len < TARGET_DIST_THRESHOLD) {
            m_walkState = WalkState::NeedTarget;
        }
    }

    calcUp(0.1f);
    calcForward();

    m_vel = Interpolate(0.05f, m_vel, m_targetVel);
}

/// @addr{0x807496C4}
/// @brief Calculates the smoothed forward vector based on @ref m_dir and the previous forward
void ObjectPoihana::calcForward() {
    EGG::Vector3f forward = Interpolate(0.1f, m_workMat.base(2), m_dir);
    if (forward.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        forward.normalise2();
    } else {
        forward = EGG::Vector3f::ez;
    }

    m_forward = forward;
}

} // namespace Kinoko::Field
