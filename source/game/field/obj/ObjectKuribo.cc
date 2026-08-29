#include "ObjectKuribo.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806DB184}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectKuribo::ObjectKuribo(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), StateManager(this, STATE_ENTRIES),
      m_accel(static_cast<f32>(params.setting(1)) / 100.0f),
      m_animRate(static_cast<f32>(params.setting(2)) / 100.0f) {}

/// @addr{0x806DB3A0}
/// @brief Default virtual destructor
ObjectKuribo::~ObjectKuribo() = default;

/// @addr{0x806DB40C}
void ObjectKuribo::init() {
    calcTransform();
    m_forward = transform().base(2);

    m_railInterpolator->init(0.0f, 0);
    m_railInterpolator->setCurrVel(0.0f);

    m_currSpeed = 0.0f;
    m_animTimer = 0.0f;
    m_currFrame = 0;

    auto *anmMgr = m_drawMdl->anmMgr();
    anmMgr->playAnim(0.0f, m_animRate, 0);
    m_animDuration = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();
    m_nextStateId = 1;
}

/// @addr{0x806dd278}
void ObjectKuribo::loadAnims() {
    std::array<const char *, 2> names = {{
            "walk_l",
            "walk_r",
    }};

    std::array<Render::AnmType, 2> types = {{
            Render::AnmType::Chr,
            Render::AnmType::Chr,
    }};

    linkAnims(names, types);
}

/// @addr{0x806DC220}
/// @brief Called when the Goomba is changing direction
void ObjectKuribo::calcReroute() {
    if (m_railInterpolator->curPoint().setting[0] < m_currentFrame) {
        m_nextStateId = 1;
    }

    checkSphereFull();
    calcRot();
    calcMatFromRotAndForward();
}

/// @addr{0x806DCDDC}
void ObjectKuribo::calcAnim() {
    bool shouldMove;

    if (m_railInterpolator->isMovementDirectionForward()) {
        shouldMove = m_animTimer > 15.0f && m_animTimer < 25.0f;
    } else {
        shouldMove = m_animTimer > 45.0f && m_animTimer < 55.0f;
    }

    if (shouldMove) {
        m_currSpeed = std::min(10.0f, m_currSpeed + m_accel);
    } else {
        m_currSpeed = std::max(0.0f, m_currSpeed - m_accel);
    }

    m_railInterpolator->setCurrVel(m_currSpeed);
    const auto &curPos = m_railInterpolator->curPos();
    setPos(EGG::Vector3f(curPos.x, pos().y, curPos.z));

    if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
        m_nextStateId = 0;
        m_currSpeed = 0.0f;
    }

    checkSphereFull();
    calcRot();
    calcMatFromRotAndForward();
}

/// @addr{0x806DCC9C}
/// @brief Smoothly interpolates the Goomba's up vector to match the floor normal beneath it
void ObjectKuribo::calcRot() {
    m_rot = Interpolate(0.1f, m_rot, m_floorNrm);

    if (m_rot.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_rot.normalise2();
    } else {
        m_rot = EGG::Vector3f::ey;
    }
}

/// @addr{0x806DCB58}
/// @brief Checks for floor collision beneath the Goomba
void ObjectKuribo::checkSphereFull() {
    constexpr f32 RADIUS = 50.0f;

    // Apply gravity if we're not changing direction
    if (m_currentStateId != 0) {
        subPos(EGG::Vector3f(0.0f, 2.0f, 0.0f));
    }

    CollisionInfo colInfo;
    EGG::Vector3f colPos = pos();
    colPos.y += RADIUS;

    bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, colPos, EGG::Vector3f::inf,
            KCL_TYPE_FLOOR, &colInfo, nullptr, 0);

    if (hasCol) {
        addPos(colInfo.tangentOff);

        if (colInfo.floorDist > -std::numeric_limits<f32>::min()) {
            m_floorNrm = colInfo.floorNrm;
        }
    }
}

} // namespace Kinoko::Field
