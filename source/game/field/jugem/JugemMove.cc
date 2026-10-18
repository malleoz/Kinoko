#include "JugemMove.hh"

#include "game/kart/KartObject.hh"

namespace Kinoko::Field {

/// @addr{0x8071E9B4}
/// @brief Constructor
/// @param kartObj Pointer to the kart object associated with this instance
JugemMove::JugemMove(const Kart::KartObject *kartObj) : m_kartObj(kartObj) {}

/// @addr{0x8071EA80}
/// @brief Default destructor
JugemMove::~JugemMove() = default;

/// @addr{0x8071EB6C}
/// @brief Initializes the internal state, including all matrices, vectors, and interpolation rates
void JugemMove::init() {
    m_leanMat = EGG::Matrix34f::ident;
    m_rtMat = EGG::Matrix34f::ident;
    m_transform = EGG::Matrix34f::ident;
    m_phaseX = 0.0f;
    m_phaseY = 0.0f;
    m_pos.setZero();
    m_transPos.setZero();
    m_lastKartObjPos = m_kartObj->pos();
    m_vel.setZero();
    m_riseVel.setZero();
    m_smoothedVel.setZero();
    m_anchorPos.setZero();
    m_currForward.setZero();
    m_targetForward = EGG::Vector3f::ez;
    m_isAwayOrDescending = false;
    m_velInterpRate = 1.0f;
    m_forwardInterpRate = 0.08f;
    m_isDescending = true;
    m_isRising = false;
}

/// @addr{0x8071F404}
/// @brief Calculates Lakitu's position, orientation, and oscillation based on the kart's position
/// @details If the Lakitu is descending from the sky, it will accelerate towards the anchor point
/// with a speed cap of `25.0f`. Otherwise, the Lakitu's velocity will change based on how far the
/// Lakitu is from the current @ref m_anchorPos.
void JugemMove::calc() {
    constexpr EGG::Vector3f POS_OFFSET = EGG::Vector3f(0.0f, 40.0f, 0.0f);

    const EGG::Vector3f &pos = m_kartObj->pos();
    f32 heightGapT = (EGG::Mathf::abs(m_anchorPos.y - m_pos.y) - 200.0f) / 100.0f;
    heightGapT = std::clamp(heightGapT, 0.0f, 1.0f);

    f32 yDeltaWeight = 0.4f + heightGapT * (0.8f - 0.4f);
    yDeltaWeight = std::clamp(yDeltaWeight, 0.0f, 1.0f);

    EGG::Vector3f kartObjPosDelta = pos - m_lastKartObjPos;
    kartObjPosDelta.y = yDeltaWeight * (pos.y - m_lastKartObjPos.y);
    EGG::Vector3f toAnchor = m_anchorPos - kartObjPosDelta - m_pos;
    EGG::Vector3f approachStep = toAnchor;

    f32 anchorDist = approachStep.normalise();
    f32 approachScale = anchorDist > 200.0f ? 0.75f : 0.5f;
    approachStep *= approachScale;

    f32 speedCap;

    if (!m_isAwayOrDescending) {
        m_velInterpRate = 0.0f;

        if (anchorDist > 100.0f) {
            m_vel += approachStep;
        } else {
            if (m_vel.x * approachStep.x < 0.0f) {
                m_vel.x += approachStep.x;
            }
            if (m_vel.y * approachStep.y < 0.0f) {
                m_vel.y += approachStep.y;
            }
            if (m_vel.z * approachStep.z < 0.0f) {
                m_vel.z += approachStep.z;
            }
        }

        speedCap = 8.0f;
    } else {
        m_velInterpRate = std::min(1.0f, m_velInterpRate + 0.02f);
        m_vel = approachStep.interpolate(m_velInterpRate, toAnchor);
        speedCap = 25.0f;
    }

    f32 speed = m_vel.normalise();

    if (m_isDescending) {
        speed = std::min(speedCap, speed);
    }

    m_vel *= speed;

    if (!m_isRising) {
        m_pos += kartObjPosDelta;
    }

    m_rtMat = calcOrthonormalBasis();
    EGG::Vector3f oscPosOffset = calcOscillation(m_rtMat);

    if (m_isRising) {
        m_pos = m_pos + m_riseVel + EGG::Vector3f(kartObjPosDelta.x, 0.0f, kartObjPosDelta.z);
        m_isRising = false;
    } else {
        m_pos += m_vel;
    }

    m_leanMat = calcLeanBasis();

    m_rtMat.setBase(3, POS_OFFSET);
    m_leanMat.setBase(3, m_pos + oscPosOffset);

    EGG::Matrix34f combinedMat = EGG::Matrix34f::ident;
    combinedMat = combinedMat.multiplyTo(m_leanMat);
    combinedMat = combinedMat.multiplyTo(m_rtMat);
    combinedMat = combinedMat.multiplyTo(EGG::Matrix34f::ident);

    m_transform = combinedMat;
    m_transPos = m_transform.base(3);
    m_lastKartObjPos = pos;
}

/// @addr{0x8071F0CC}
/// @brief Updates the Lakitu's forward direction based on the kart's position delta
/// @param setCurr If true, snaps the current facing direction to the target facing direction
/// @details Recalculates Lakitu's target facing direction and interpolation rate based on the
/// kart's position delta and optionally snaps the Lakitu's current facing direction to the target.
void JugemMove::calcForwardFromKartObjPosDelta(bool setCurr) {
    constexpr f32 FAST_INTERP_DIST = 250.0f;
    constexpr f32 SLOW_INTERP_RATE = 0.03f;
    constexpr f32 FAST_INTERP_RATE = 0.08f;

    EGG::Vector3f forward = m_kartObj->pos() - m_pos;
    forward.y = 0.0f;
    f32 dist = forward.normalise();

    if (forward.squaredLength() <= std::numeric_limits<f32>::epsilon()) {
        forward = EGG::Vector3f::ez;
    }

    if (setCurr) {
        m_currForward = forward;
    }

    m_targetForward = forward;
    m_forwardInterpRate = dist < FAST_INTERP_DIST ? SLOW_INTERP_RATE : FAST_INTERP_RATE;
}

/// @addr{0x8071F204}
/// @brief Updates the Lakitu's forward direction based on the kart's main rotation
/// @param setCurr If true snaps the current facing direction to the target facing direction
/// @details Recalculates Lakitu's target facing direction based on the kart's main rotation and
/// optionally snaps the Lakitu's current facing direction to the target.
void JugemMove::calcForwardFromKartObjMainRot(bool setCurr) {
    EGG::Vector3f forward = m_kartObj->mainRot().rotateVector(EGG::Vector3f::ez);
    forward.y = 0.0f;
    forward *= -1.0f;
    forward.normalise2();

    if (forward.squaredLength() <= std::numeric_limits<f32>::epsilon()) {
        forward = EGG::Vector3f::ez;
    }

    if (setCurr) {
        m_currForward = forward;
    }

    m_targetForward = forward;
}

/// @addr{0x8071EFA8}
/// @brief Sets Lakitu's position and optionally sets the transformed position
/// @param pos The new position to set
/// @param transPos If `true`, also sets the transformed position to the same value
void JugemMove::setPos(const EGG::Vector3f &pos, bool transPos) {
    m_pos = pos;

    if (transPos) {
        m_transPos = pos;
    }
}

/// @addr{0x8071EFD8}
/// @brief Sets the anchor position that Lakitu is trying to move towards
/// @param v The new anchor position to set
void JugemMove::setAnchorPos(const EGG::Vector3f &v) {
    m_anchorPos = v;
}

/// @brief Sets Lakitu's velocity when rising back up
/// @param v The rising velocity to set
void JugemMove::setRiseVel(const EGG::Vector3f &v) {
    m_riseVel = v;
}

/// @brief Sets whether Lakitu is either away or descending
/// @param isSet `true` if Lakitu is away or descending, `false` otherwise
void JugemMove::setAwayOrDescending(bool isSet) {
    m_isAwayOrDescending = isSet;
}

/// @brief Sets whether Lakitu is descending from the sky
/// @param isSet `true` if Lakitu is descending, `false` otherwise
void JugemMove::setDescending(bool isSet) {
    m_isDescending = isSet;
}

/// @brief Sets whether Lakitu is rising up into the sky
/// @param isSet `true` if Lakitu is rising, `false` otherwise
void JugemMove::setRising(bool isSet) {
    m_isRising = isSet;
}

/// @brief Returns Lakitu's transformed position
/// @return The transformed position of Lakitu in world space, @ref m_transPos
const EGG::Vector3f &JugemMove::transPos() const {
    return m_transPos;
}

/// @brief Returns Lakitu's transformation matrix
/// @return The combined lean and rotation matrices representing Lakitu's orientation, @ref
/// m_transform
const EGG::Matrix34f &JugemMove::transform() const {
    return m_transform;
}

/// @addr{0x80720024}
/// @brief Calculates an orthonormal basis matrix based on the Lakitu's current facing direction
/// @return The orthonormal basis matrix representing Lakitu's orientation
/// @details Also interpolates the Lakitu's current facing direction towards the target facing
/// direction.
EGG::Matrix34f JugemMove::calcOrthonormalBasis() {
    m_currForward = m_currForward.interpolate(m_forwardInterpRate, m_targetForward);
    m_currForward.normalise();

    const EGG::Vector3f &up = EGG::Vector3f::ey;
    EGG::Vector3f forward = m_currForward;

    if (forward.squaredLength() <= std::numeric_limits<f32>::epsilon()) {
        forward = EGG::Vector3f::ez;
    }

    EGG::Vector3f right = up.cross(forward);
    right.normalise();

    forward = right.cross(up);
    forward.normalise();

    EGG::Matrix34f mat = EGG::Matrix34f::ident;
    mat.setBase(0, right);
    mat.setBase(1, up);
    mat.setBase(2, forward);

    return mat;
}

/// @addr{0x807201B0}
/// @brief Advances a step in the oscillation cycle and returns the resulting position offset
/// @param mat The orthonormal basis matrix to apply to the oscillation offset
/// @return The position offset resulting from the Lakitu's current oscillation
EGG::Vector3f JugemMove::calcOscillation(const EGG::Matrix34f &mat) {
    constexpr f32 PHASE_X_STEP = 0.04f;
    constexpr f32 PHASE_Y_STEP = 0.08f;
    constexpr f32 AMPLITUDE_X = 80.0f;
    constexpr f32 AMPLITUDE_Y = 30.0f;

    m_phaseX += PHASE_X_STEP;
    m_phaseY += PHASE_Y_STEP;

    if (m_phaseX > F_TAU) {
        m_phaseX -= F_TAU;
    }

    if (m_phaseY > F_TAU) {
        m_phaseY -= F_TAU;
    }

    f32 cosX = EGG::Mathf::CosFIdx(RAD2FIDX * m_phaseX);
    f32 sinY = EGG::Mathf::SinFIdx(RAD2FIDX * m_phaseY);
    EGG::Vector3f v = EGG::Vector3f(AMPLITUDE_X * cosX, AMPLITUDE_Y * sinY, 0.0f);

    return mat.ps_multVector(v);
}

/// @addr{0x807202BC}
/// @brief Calculates a rotation basis matrix to tilt the Lakitu when the player accelerates or
/// turns sharply
/// @return The rotation basis matrix representing the tilt of the Lakitu
EGG::Matrix34f JugemMove::calcLeanBasis() {
    constexpr f32 VEL_INTERP_RATE = 0.1f;
    constexpr f32 VEL_MAX = 20.0f;
    constexpr EGG::Vector3f LEAN_UP_BASE = EGG::Vector3f(0.0f, 100.0f, 0.0f);

    m_smoothedVel = m_smoothedVel.interpolate(VEL_INTERP_RATE, m_vel);
    m_smoothedVel *= std::min(VEL_MAX, m_smoothedVel.normalise());

    EGG::Vector3f up = LEAN_UP_BASE - m_smoothedVel * 1.5f;
    f32 lengthRatio = up.length() / LEAN_UP_BASE.length();
    up.normalise();

    up *= lengthRatio;
    if (up.squaredLength() <= std::numeric_limits<f32>::epsilon()) {
        up = EGG::Vector3f::ey;
    }

    EGG::Matrix34f mat = EGG::Matrix34f::ident;
    mat.setBase(0, EGG::Vector3f::ex);
    mat.setBase(1, up);
    mat.setBase(2, EGG::Vector3f::ez);

    return mat;
}

} // namespace Kinoko::Field
