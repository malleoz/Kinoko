#include "ObjectCarA.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x806B7710}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectCarA::ObjectCarA(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), StateManager(this, STATE_ENTRIES),
      m_finalVel(static_cast<f32>(params.setting(0))),
      m_accel(static_cast<f32>(params.setting(1)) / 10.0f),
      m_stopTime(static_cast<u32>(params.setting(2))) {}

/// @addr{0x806B78CC}
/// @brief Default virtual destructor
ObjectCarA::~ObjectCarA() = default;

/// @addr{0x806B7CE0}
void ObjectCarA::init() {
    constexpr f32 RADIUS = 400.0f;

    m_railInterpolator->init(0, 0);

    // Time to reach final velocity
    f32 finalTime = m_finalVel / m_accel;
    m_cruiseTime =
            (m_railInterpolator->railLength() - finalTime * m_accel * finalTime) / m_finalVel;

    setMatrixFromOrthonormalBasisAndPos(m_railInterpolator->curTangentDir());

    setPos(m_railInterpolator->curPos());
    m_currVel = 0.0f;
    m_motionState = MotionState::Accelerating;
    m_changingDir = false;
    m_currUp = EGG::Vector3f::ey;
    m_currTangent = m_railInterpolator->curTangentDir();
    resize(RADIUS, 0.0f);
    m_nextStateId = 0;
}

/// @addr{0x806B7BC4}
void ObjectCarA::calcCollisionTransform() {
    ObjectCollisionBase *objCol = collision();
    if (!objCol) {
        return;
    }

    calcTransform();

    EGG::Matrix34f mat;
    SetRotTangentHorizontal(mat, transform().base(2), EGG::Vector3f::ey);
    mat.setBase(3, transform().base(3) + 50.0f * transform().base(1));

    objCol->transform(mat, scale(),
            -m_railInterpolator->curTangentDir() * m_railInterpolator->getCurrVel());
}

/// @addr{0x806B7E60}
Kart::Reaction ObjectCarA::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) {
    return kartObj->speedRatioCapped() < 0.5f ? Kart::Reaction::Wall : reactionOnKart;
}

/// @addr{0x806B86F0}
/// @brief Runs once per frame when the car is in the accelerating or decelerating state
void ObjectCarA::calcAccel() {
    if (m_motionState == MotionState::Accelerating) {
        m_currVel += m_accel;

        if (m_currVel > m_finalVel) {
            m_currVel = m_finalVel;
            m_motionState = MotionState::Cruising;
            m_nextStateId = 2;
        }
    } else if (m_motionState == MotionState::Decelerating) {
        // The cruising time might've had decimals and needed to end early.
        // If we undershot it, keep a velocity of 0.01f so we can reach the end.
        m_currVel = std::max(0.01f, m_currVel - m_accel);

        if (m_changingDir) {
            m_currVel = 0.0f;

            if (m_railInterpolator->isMovementDirectionForward()) {
                m_railInterpolator->init(0.0f, 0);
            }

            m_motionState = MotionState::Cruising;
            m_nextStateId = 0;
        }
    }
}

} // namespace Kinoko::Field
