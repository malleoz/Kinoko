#include "ObjectPakkunF.hh"

namespace Kinoko::Field {

/// @addr{0x807743A4}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectPakkunF::ObjectPakkunF(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), m_attackFrames(0), m_currAttackFrame(0),
      m_waitDuration(static_cast<s32>(m_mapObj->setting(0))) {}

/// @addr{0x807754FC}
/// @brief Default virtual destructor
ObjectPakkunF::~ObjectPakkunF() = default;

/// @addr{0x80774754}
/// @copybrief ObjectBase::calc()
void ObjectPakkunF::calc() {
    switch (m_state) {
    case State::Wait:
        calcWait();
        break;
    case State::Attack:
        calcAttack();
        break;
    default:
        break;
    }

    calcCollisionTransform();
}

/// @addr{0x80775464}
/// @copybrief ObjectBase::loadAnims()
void ObjectPakkunF::loadAnims() {
    std::array<const char *, 3> names = {{
            "attack",
            "damage",
            "wait",
    }};

    std::array<Render::AnmType, 3> types = {{
            Render::AnmType::Chr,
            Render::AnmType::Chr,
            Render::AnmType::Chr,
    }};

    linkAnims(names, types);
}

/// @addr{0x80775108}
/// @copybrief ObjectBase::calcCollisionTransform()
/// @details Offsets the collision position when the piranha is attacking. For the first 10 frames,
/// it remains at an initial position. For the next 10 frames, it linearly interpolates from the
/// initial position to a "final" position. For the next 10 frames, it linearly interpolates back to
/// the initial position where it remains until the cycle is reset.
void ObjectPakkunF::calcCollisionTransform() {
    constexpr EGG::Vector3f INIT_POS = EGG::Vector3f(0.0f, 620.0f, 70.0f);
    constexpr EGG::Vector3f FINAL_POS = EGG::Vector3f(0.0f, 160.0f, 550.0f);

    if (!m_collision) {
        return;
    }

    EGG::Vector3f posOffset;

    if (m_state == State::Attack) {
        if (m_currAttackFrame <= 10) {
            posOffset = INIT_POS * scale().x;
        } else if (m_currAttackFrame <= 20) {
            f32 fVar1 = 0.1f * static_cast<f32>(m_currAttackFrame - 10);
            posOffset = FINAL_POS * scale().x * fVar1 + INIT_POS * scale().x * (1.0f - fVar1);
        } else if (m_currAttackFrame <= 30) {
            f32 fVar1 = 0.1f * static_cast<f32>(m_currAttackFrame - 20);
            posOffset = INIT_POS * scale().x * fVar1 + FINAL_POS * scale().x * (1.0f - fVar1);
        } else {
            posOffset = INIT_POS * scale().x;
        }
    } else {
        posOffset = INIT_POS * scale().x;
    }

    EGG::Matrix34f rotMat;
    rotMat.makeR(rot());

    // Probably always evaluates to the identity matrix in time trials
    EGG::Matrix34f damageRotMat = EGG::Matrix34f::ident;
    damageRotMat.setAxisRotation(0.0f, EGG::Vector3f::ey);

    EGG::Matrix34f transformMat;
    transformMat.makeT(pos() + rotMat.multiplyTo(damageRotMat).ps_multVector(posOffset));

    m_collision->transform(transformMat, scale());
}

/// @addr{0x80774CB0}
/// @brief Runs once when the piranha stops idling and is about to start attacking
void ObjectPakkunF::enterAttack() {
    constexpr s32 LINGERING_FRAMES = 60; ///< Additional frames before switching back to idle state

    m_state = State::Attack;
    auto *anmMgr = m_drawMdl->anmMgr();
    anmMgr->playAnim(0.0f, 1.0f, 0);
    m_currAttackFrame = 0;
    auto *attackAnm = anmMgr->activeAnim(Render::AnmType::Chr);
    m_attackFrames = static_cast<s32>(attackAnm->frameCount()) + LINGERING_FRAMES;
}

} // namespace Kinoko::Field
