#include "ObjectFireSnakeV.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x806C2B70}
ObjectFireSnakeV::ObjectFireSnakeV(const System::MapdataGeoObj &params)
    : ObjectFireSnake(params), StateManager<ObjectFireSnakeV>(this),
      m_cycleDuration(params.setting(1)), m_distFromPipe(static_cast<f32>(params.setting(2))) {
    m_delayFrame = params.setting(0);
    m_sunPos = m_pos;

    calcTransform();

    m_initRot = m_transform.base(0);
    m_initialPos = m_sunPos + m_initRot * m_distFromPipe;
}

/// @addr{0x806C3548}
ObjectFireSnakeV::~ObjectFireSnakeV() = default;

/// @addr{0x806C2CBC}
void ObjectFireSnakeV::init() {
    StateManager<ObjectFireSnakeV>::m_nextStateId = 1;
    ObjectFireSnake::enterDespawned();

    m_visualPos = m_sunPos;
    m_bounceDir = m_initRot;
}

/// @addr{0x806C2D54}
void ObjectFireSnakeV::calc() {
    if (System::RaceManager::Instance()->timer() < m_delayFrame) {
        return;
    }

    FUN_806C2DA4();
}

/// @brief Helper function since high and regular bounce functions only vary by initial velocity.
/// @todo THIS IS A DUPLICATE OF OBJECTFIRESNAKE TO AVOID USING THE WRONG STATEMANAGER INHERITANCE.
void ObjectFireSnakeV::calcBounce(f32 initialVel) {
    // Collision checks only occur after 10 frames in the bounce state.
    constexpr u32 BOUNCE_COL_CHECK_DELAY = 10;
    constexpr f32 RADIUS = 130.0f;
    constexpr f32 BOUNCE_SPEED = 20.0f;

    m_visualPos.x += m_bounceDir.x * BOUNCE_SPEED;
    m_visualPos.z += m_bounceDir.z * BOUNCE_SPEED;
    m_visualPos.y +=
            initialVel - GRAVITY * static_cast<f32>(StateManager<ObjectFireSnakeV>::m_currentFrame);

    CollisionInfo info;

    if (StateManager<ObjectFireSnakeV>::m_currentFrame > BOUNCE_COL_CHECK_DELAY) {
        bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, m_visualPos,
                EGG::Vector3f::inf, KCL_TYPE_64EBDFFF, &info, nullptr, 0);

        if (hasCol) {
            m_visualPos += info.tangentOff;
            StateManager<ObjectFireSnakeV>::m_nextStateId = 3;
        }
    }

    m_pos = m_visualPos;
    m_flags.setBit(eFlags::Position);
}

/// @addr{0x806C2530}
/// @todo THIS IS A DUPLICATE OF OBJECTFIRESNAKV TO AVOID USING THE WRONG STATEMANAGER INHERITANCE.
void ObjectFireSnakeV::calcChildren() {
    // Shift all matrices up by one index
    for (size_t i = m_prevTransforms.size() - 1; i > 0; --i) {
        m_prevTransforms[i] = m_prevTransforms[i - 1];
    }

    calcTransform();
    m_prevTransforms[0] = m_transform;
    m_prevTransforms[0].setBase(3, m_pos);

    u32 idx = 10;
    for (auto *&kid : m_kids) {
        if (StateManager<ObjectFireSnakeV>::m_currentStateId == 1 &&
                idx == StateManager<ObjectFireSnakeV>::m_currentFrame) {
            if (!kid->getUnit()) {
                kid->loadAABB(0.0f);
            }
        } else if (StateManager<ObjectFireSnakeV>::m_currentStateId == 0 &&
                StateManager<ObjectFireSnakeV>::m_currentFrame == 0) {
            if (kid->getUnit()) {
                kid->unregisterCollision();
            }
        }

        if (StateManager<ObjectFireSnakeV>::m_currentStateId == 1 &&
                StateManager<ObjectFireSnakeV>::m_currentFrame < idx) {
            calcTransform();
            kid->setTransform(m_transform);
        } else {
            kid->setTransform(m_prevTransforms[idx]);
        }

        idx += 10;
    }
}

/// @addr{0x806C30F8}
void ObjectFireSnakeV::enterFalling() {
    constexpr f32 FALL_DURATION = 140.0f;

    ObjectFireSnake::enterFalling();

    if (getUnit()) {
        unregisterCollision();
    }

    m_age = 0;
    m_visualPos = m_sunPos;
    m_bounceDir = m_initRot;
    m_initialPos = m_sunPos + m_initRot * m_distFromPipe;
    m_fallSpeed = m_distFromPipe / FALL_DURATION;
}

/// @addr{0x806C31F0}
void ObjectFireSnakeV::calcFalling() {
    constexpr f32 INITIAL_Y_VELOCITY = 120.0f;

    if (StateManager<ObjectFireSnakeV>::m_currentFrame > 5 && !getUnit()) {
        loadAABB(0.0f);
    }

    m_visualPos.x += m_bounceDir.x * m_fallSpeed;
    m_visualPos.z += m_bounceDir.z * m_fallSpeed;
    m_visualPos.y += INITIAL_Y_VELOCITY -
            GRAVITY * static_cast<f32>(StateManager<ObjectFireSnakeV>::m_currentFrame);

    if (StateManager<ObjectFireSnakeV>::m_currentFrame > 10) {
        CollisionInfo colInfo;

        bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, m_visualPos,
                EGG::Vector3f::inf, KCL_TYPE_FLOOR, &colInfo, nullptr, 0);

        if (hasCol) {
            m_visualPos += colInfo.tangentOff;
            StateManager<ObjectFireSnakeV>::m_nextStateId = 2;
        }
    }

    setMatrixTangentTo(EGG::Vector3f::ey, m_bounceDir);
    m_flags.setBit(eFlags::Position);
    m_pos = m_visualPos;
}

/// @addr{0x806C33C4}
void ObjectFireSnakeV::calcHighBounce() {
    constexpr f32 INITIAL_Y_VELOCITY = 90.0f;

    m_visualPos.z += m_bounceDir.z * m_fallSpeed;
    m_visualPos.x += m_bounceDir.x * m_fallSpeed;
    m_visualPos.y += INITIAL_Y_VELOCITY -
            GRAVITY * static_cast<f32>(StateManager<ObjectFireSnakeV>::m_currentFrame);

    if (StateManager<ObjectFireSnakeV>::m_currentFrame > 10) {
        CollisionInfo colInfo;

        bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, m_visualPos,
                EGG::Vector3f::inf, KCL_TYPE_FLOOR, &colInfo, nullptr, 0);

        if (hasCol) {
            m_visualPos += colInfo.tangentOff;
            StateManager<ObjectFireSnakeV>::m_nextStateId = 3;
        }
    }

    setMatrixTangentTo(EGG::Vector3f::ey, m_bounceDir);
    m_flags.setBit(eFlags::Position);
    m_pos = m_visualPos;
}

/// @addr{0x806C2138}
/// @todo THIS IS A DUPLICATE OF OBJECTFIRESNAKE. IT MUST BE REDECLARED HERE TO AVOID USING THE
/// WRONG STATEMANAGER INHERITANCE.
void ObjectFireSnakeV::calcRest() {
    if (StateManager<ObjectFireSnakeV>::m_currentFrame >= 30) {
        StateManager<ObjectFireSnakeV>::m_nextStateId = 4;
    }

    calcTransform();

    EGG::Vector3f tangent = Interpolate(0.3f, m_transform.base(2), m_bounceDir);
    tangent.normalise2();

    setMatrixTangentTo(EGG::Vector3f::ey, tangent);
}

/// @addr{0x806C2DA4}
void ObjectFireSnakeV::FUN_806C2DA4() {
    StateManager<ObjectFireSnakeV>::calc();

    u32 frame = System::RaceManager::Instance()->timer() - m_delayFrame;
    if (frame % m_cycleDuration == 0) {
        StateManager<ObjectFireSnakeV>::m_nextStateId = 1;
    }

    if (StateManager<ObjectFireSnakeV>::m_currentStateId == 1 ||
            StateManager<ObjectFireSnakeV>::m_currentStateId == 2 ||
            StateManager<ObjectFireSnakeV>::m_currentStateId == 3 ||
            StateManager<ObjectFireSnakeV>::m_currentStateId == 4) {
        ++m_age;
        if (m_age > 600 && StateManager<ObjectFireSnakeV>::m_currentStateId == 3) {
            StateManager<ObjectFireSnakeV>::m_nextStateId = 0;
        }
    }

    calcChildren();
}

} // namespace Field
