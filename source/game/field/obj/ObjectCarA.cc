#include "ObjectCarA.hh"

#include "game/field/ObjectCollisionCylinder.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartObject.hh"

namespace Field {

/// @addr{0x806B7710}
ObjectCarA::ObjectCarA(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), StateManager(this) {
    m_speed = static_cast<f32>(params.setting(0));
    m_accel = static_cast<f32>(params.setting(1)) / 10.0f;
    m_pauseTime = params.setting(2);
}

/// @addr{0x806B78CC}
ObjectCarA::~ObjectCarA() = default;

/// @addr{0x806B7CE0}
void ObjectCarA::init() {
    m_railInterpolator->init(0, 0);
    f32 accelTime = m_speed / m_accel;
    m_railFrameCount =
            (m_railInterpolator->railLength() - accelTime * m_accel * accelTime) / m_speed;

    FUN_808218B0(m_railInterpolator->curTangentDir());

    m_flags |= 1;
    m_pos = m_railInterpolator->curPos();
    m_currVel = 0.0f;
    m_state = 0;
    m_isChangingDir = false;
    m_currUp = EGG::Vector3f::ey;
    m_currTangent = m_railInterpolator->curTangentDir();
    m_nextStateId = 0;
}

/// @addr{0x806B82CC}
void ObjectCarA::calc() {
    if (m_nextStateId < 0) {
        ++m_currentFrame;
    } else {
        m_currentStateId = m_nextStateId;
        m_nextStateId = -1;
        m_currentFrame = 0;

        auto enterFunc = m_entries[m_entryIds[m_currentStateId]].onEnter;
        (this->*enterFunc)();
    }

    auto calcFunc = m_entries[m_entryIds[m_currentStateId]].onCalc;
    (this->*calcFunc)();

    m_railInterpolator->setCurrVel(m_currVel);

    auto status = m_railInterpolator->calc();
    m_isChangingDir = (status == RailInterpolator::Status::ChangingDirection);

    calcPos();
}

/// @addr{0x806B7B44}
void ObjectCarA::createCollision() {
    constexpr f32 RADIUS = 210.0f;
    constexpr f32 HEIGHT = 200.0f;

    m_collision = new ObjectCollisionCylinder(RADIUS, HEIGHT, collisionCenter());
}

/// @addr{0x806B7BC4}
void ObjectCarA::calcCollisionTransform() {
    ObjectCollisionBase *objCol = collision();
    if (!objCol) {
        return;
    }

    calcTransform();

    EGG::Matrix34f mat;
    mat.setRotTangentHorizontal(m_transform.base(2), EGG::Vector3f::ey);

    calcTransform();

    EGG::Vector3f scaledBase1 = 50.0f * m_transform.base(1);

    calcTransform();

    mat.setBase(3, m_transform.base(3) + scaledBase1);
    objCol->transform(mat, m_scale,
            -m_railInterpolator->curTangentDir() * m_railInterpolator->getCurrVel());
}

/// @addr{0x806B7E60}
Kart::Reaction ObjectCarA::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction reactionOnObj, EGG::Vector3f & /*hitDepth*/) {
    if (reactionOnObj == Kart::Reaction::None) {
        if (kartObj->speedRatioCapped() < 0.5f) {
            reactionOnKart = Kart::Reaction::WallAllSpeed;
        }
    }

    return reactionOnKart;
}

void ObjectCarA::enterStateStub() {}

/// @addr{0x806B84FC}
void ObjectCarA::enterState0() {
    m_currVel = 0.0f;
}

/// @addr{0x806B8838}
void ObjectCarA::enterState2() {
    m_currVel = m_speed;
}

/// @addr{0x806B8588}
void ObjectCarA::calcState0() {
    if (m_currentFrame > m_pauseTime) {
        m_state = 0;
        m_currentStateId = 1;
    }
}

/// @addr{0x806B86F0}
void ObjectCarA::calcState1() {
    if (m_state == 0) {
        m_currVel += m_accel;

        if (m_speed < m_currVel) {
            m_currVel = m_speed;
            m_state = 1;
            m_nextStateId = 2;
        }
    } else if (m_state == 2) {
        m_currVel = std::max(0.01f, m_currVel - m_accel);

        if (m_isChangingDir) {
            m_currVel = 0.0f;

            if (m_railInterpolator->isMovementDirectionForward()) {
                m_railInterpolator->init(0.0f, 0);
            }

            m_state = 1;
            m_nextStateId = 0;
        }
    }
}

/// @addr{0x806B8844}
void ObjectCarA::calcState2() {
    if (static_cast<f32>(m_currentFrame) > m_railFrameCount - 1.0f) {
        m_state = 2;
        m_nextStateId = 1;
    }
}

/// @addr{0x806B8D3C}
void ObjectCarA::calcPos() {
    m_currUp = interpolate(0.1f, m_currUp, EGG::Vector3f::ey);
    m_currUp.normalise2();

    setMatrixTangentTo(m_currUp, m_currTangent);

    m_pos = m_railInterpolator->curPos();
    m_flags |= 1;
}

const std::array<StateManagerEntry<ObjectCarA>, 3> StateManager<ObjectCarA>::STATE_ENTRIES = {{
        {0, &ObjectCarA::enterState0, &ObjectCarA::calcState0},
        {1, &ObjectCarA::enterStateStub, &ObjectCarA::calcState1},
        {2, &ObjectCarA::enterState2, &ObjectCarA::calcState2},
}};

StateManager<ObjectCarA>::StateManager(ObjectCarA *obj) {
    constexpr size_t ENTRY_COUNT = 3;

    m_obj = obj;
    m_entries = std::span{STATE_ENTRIES};
    m_entryIds = std::span(new u16[ENTRY_COUNT], ENTRY_COUNT);

    // The base game initializes all entries to 0xffff, possibly to avoid an uninitialized value
    for (auto &id : m_entryIds) {
        id = 0xffff;
    }

    for (size_t i = 0; i < m_entryIds.size(); ++i) {
        m_entryIds[STATE_ENTRIES[i].id] = i;
    }
}

StateManager<ObjectCarA>::~StateManager() {
    delete[] m_entryIds.data();
}

} // namespace Field
