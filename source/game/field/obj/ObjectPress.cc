#include "ObjectPress.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x80777564}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectPress::ObjectPress(const System::MapdataGeoObj &params)
    : ObjectCollidable(params),
      m_loweringVelocity(0.0f),
      m_initialRaisedDuration(static_cast<u32>(m_mapObj->setting(1))),
      m_raisedDuration(static_cast<u32>(m_mapObj->setting(2))) {}

/// @addr{0x807775FC}
/// @brief Default virtual destructor
ObjectPress::~ObjectPress() = default;

/// @addr{0x8077763C}
/// @copybrief ObjectBase::init()
/// @details Performs a floor collision check to find the lowered position and to laterally adjust
/// the stomper's position to avoid the stomper partially clipping into the floor.
void ObjectPress::init() {
    constexpr f32 FLOOR_CHECK_SPEED = 100.0f;
    constexpr f32 HEIGHT = 20.0f;
    constexpr EGG::Vector3f HITBOX_OFFSET = EGG::Vector3f(0.0f, HEIGHT, 0.0f);

    m_state = State::Raised;
    m_startingRise = false;
    initTimers();
    m_raisedHeight = pos().y;

    bool hasCol = false;
    CollisionInfo info;
    auto *colDir = CollisionDirector::Instance();

    while (!hasCol) {
        subPos(EGG::Vector3f(0.0f, FLOOR_CHECK_SPEED, 0.0f));

        hasCol = colDir->checkSphereFull(HEIGHT, pos() + HITBOX_OFFSET, EGG::Vector3f::inf,
                KCL_TYPE_FLOOR, &info, nullptr, 0);
    }

    EGG::Vector3f loweredPos = pos() + info.tangentOff;
    setPos(EGG::Vector3f(loweredPos.x, m_raisedHeight, loweredPos.z));
    m_startedLowered = false;
}

/// @addr{0x8077788C}
/// @copybrief ObjectBase::calc()
void ObjectPress::calc() {
    m_startedLowered = false;

    switch (m_state) {
    case State::Raised:
        calcRaised();
        break;
    case State::WindUp:
        calcWindUp();
        break;
    case State::Lowering:
        calcLowering();
        break;
    case State::Lowered:
        calcLowered();
        break;
    case State::Raising:
        calcRaising();
        break;
    }
}

/// @addr{0x8077840C}
/// @copybrief ObjectBase::createCollision()
/// @details Defines the convex hull geometry that represents the press's collision shape.
void ObjectPress::createCollision() {
    constexpr f32 POINT_SCALE = 10.0f;
    constexpr std::array<EGG::Vector3f, 16> POINTS = {{
            EGG::Vector3f(37.5f, 0.0f, 43.5f) * POINT_SCALE,
            EGG::Vector3f(43.0f, 0.0f, 38.0f) * POINT_SCALE,
            EGG::Vector3f(37.5f, 0.0f, -43.5f) * POINT_SCALE,
            EGG::Vector3f(43.0f, 0.0f, 38.0f) * POINT_SCALE,
            EGG::Vector3f(-37.5f, 0.0f, -43.5f) * POINT_SCALE,
            EGG::Vector3f(-43.0f, 0.0f, -38.0f) * POINT_SCALE,
            EGG::Vector3f(-37.5f, 0.0f, 43.5f) * POINT_SCALE,
            EGG::Vector3f(-43.0f, 0.0f, 38.0f) * POINT_SCALE,
            EGG::Vector3f(40.0f, 143.8f, 46.0f) * POINT_SCALE,
            EGG::Vector3f(45.5f, 143.8f, 40.5f) * POINT_SCALE,
            EGG::Vector3f(40.0f, 143.8f, -46.0f) * POINT_SCALE,
            EGG::Vector3f(45.5f, 143.8f, -40.5f) * POINT_SCALE,
            EGG::Vector3f(-40.0f, 143.8f, -46.0f) * POINT_SCALE,
            EGG::Vector3f(-45.5f, 143.8f, -40.5f) * POINT_SCALE,
            EGG::Vector3f(-40.0f, 143.8f, 46.0f) * POINT_SCALE,
            EGG::Vector3f(-45.5f, 143.8f, 40.5f) * POINT_SCALE,
    }};

    m_collision = EGG::egg_new<ObjectCollisionConvexHull>(POINTS);
}

/// @addr{0x807782D4}
/// @details Checks if the kart is within the "crush threshold". If so, and the press is lowering,
/// then the kart will be squished and items will be lost.
Kart::Reaction ObjectPress::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) {
    constexpr f32 CRUSH_THRESHOLD = 430.0f;

    EGG::Vector3f diff = kartObj->pos() - pos();
    bool close =
            EGG::Mathf::abs(diff.x) < CRUSH_THRESHOLD && EGG::Mathf::abs(diff.z) < CRUSH_THRESHOLD;

    // ObjectId::Press corresponds to a WallAllSpeed reaction, so when the stomper is coming down,
    // the base game instead uses @ref ObjectId::PressSoko to induce LongCrushLoseItem.
    if (close && (m_state == State::Lowering || m_startedLowered)) {
        const auto &hitTable = ObjectDirector::Instance()->hitTableKart();
        return hitTable.reaction(hitTable.slot(ObjectId::PressSoko));
    }

    return reactionOnKart;
}

/// @addr{0x80777BC0}
/// @brief Runs every frame that the press is in contact with the floor
/// @details Plays a press and unpress animation. When the press animatiosn finish, the press will
/// transition to the raising state.
void ObjectPress::calcLowered() {
    if (--m_anmTimer > 0) {
        return;
    }

    if (m_startingRise) {
        m_state = State::Raising;
    } else {
        enterRaising();
    }
}

/// @addr{0x80777CB0}
/// @brief Runs every frame that the press is rising back up to the raised position
/// @details When the press reaches the raised position, it will transition to the raised state.
void ObjectPress::calcRaising() {
    constexpr f32 SPEED = 10.0f;

    f32 height = pos().y + SPEED;

    if (height < m_raisedHeight) {
        setPos(EGG::Vector3f(pos().x, height, pos().z));
    } else {
        setPos(EGG::Vector3f(pos().x, m_raisedHeight, pos().z));
        m_state = State::Raised;
        m_raisedTimer = m_raisedDuration;
    }
}

/// @addr{0x80777D10}
/// @brief Runs while the press is lowering to check if it has hit the floor
void ObjectPress::checkCollisionLowering() {
    constexpr f32 RADIUS = 10.0f;
    constexpr EGG::Vector3f HITBOX_OFFSET = EGG::Vector3f(0.0f, RADIUS, 0.0f);

    CollisionInfo info;

    if (!CollisionDirector::Instance()->checkSphereFull(RADIUS, pos() + HITBOX_OFFSET,
                EGG::Vector3f::inf, KCL_TYPE_FLOOR, &info, nullptr, 0)) {
        return;
    }

    m_loweringVelocity = 0.0f;
    addPos(info.tangentOff);

    auto *anmMgr = m_drawMdl->anmMgr();
    anmMgr->playAnim(0.0f, ANM_RATE, 0);

    m_state = State::Lowered;
    m_startingRise = false;
    m_anmTimer = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount() / ANM_RATE;
    m_startedLowered = true;
}

/// @addr{0x8076E7AC}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectPressSenko::ObjectPressSenko(const System::MapdataGeoObj &params)
    : ObjectPress(params),
      m_startingWindup(false) {}

/// @addr{0x8076E818}
/// @brief Default virtual destructor
ObjectPressSenko::~ObjectPressSenko() = default;

} // namespace Kinoko::Field
