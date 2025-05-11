#include "ObjectKoopaFireball.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/KColData.hh"

#include "game/kart/KartCollide.hh"

namespace Field {

/// @addr{0x80770384}
ObjectKoopaFireball::ObjectKoopaFireball(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), m_collisionTranslation(EGG::Vector3f::zero) {}

/// @addr{0x80771F70}
ObjectKoopaFireball::~ObjectKoopaFireball() = default;

/// @addr{0x807703D0}
void ObjectKoopaFireball::init() {
    constexpr u32 START_COOLDOWN = 221;

    m_state = State::Exploding;
    m_cooldownTimer = START_COOLDOWN;

    m_railInterpolator->init(0.0f, 0);
    m_railInterpolator->setCurrVel(INITIAL_VELOCITY);
    m_railInterpolator->calc();

    m_startYpos = m_railInterpolator->curPos().y;
    m_flags |= 1;
    m_pos.y = m_startYpos;

    EGG::Vector3f curTanDirNorm = m_railInterpolator->curTangentDir();
    curTanDirNorm.normalise();
    setMatrixLookAt(curTanDirNorm);
    calcTransform();
    m_curTransform = m_transform;

    m_angleRad = 0.0f;
    m_angSpeed = INITIAL_ANGULAR_SPEED;
    m_collisionTranslation.y = -30.0f;

    std::array<const char *, 1> names = {{
            "bombCore",
    }};

    std::array<Render::AnmType, 1> types = {{
            Render::AnmType::Chr,
    }};

    linkAnims(names, types);
    auto *anmMgr = m_drawMdl->anmMgr();
    anmMgr->playAnim(0.0f, 1.0f, 0);
    m_animFramecount = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();

    EGG::Matrix34f mat;
    mat.makeR(m_rot);
    m_curRot = mat.base(2);
    m_curRot.normalise();

    vf88(870.0f * 2.0f, 0.0f);
}

/// @addr{0x80770ADC}
void ObjectKoopaFireball::calc() {
    switch (m_state) {
    case State::Tangible:
        calcTangible();
        break;
    case State::Intangible:
        calcIntangible();
        break;
    case State::Exploding:
        calcExploding();
        break;
    default:
        break;
    }

    --m_cooldownTimer;

    EGG::Vector3f scaledTangent =
            m_railInterpolator->curTangentDir() * m_railInterpolator->getCurrVel();
    m_collisionTranslation.x = scaledTangent.x;
    m_collisionTranslation.z = scaledTangent.z;
}

/// @addr{0x80771BF4}
Kart::Reaction ObjectKoopaFireball::onCollision(Kart::KartObject * /*kartObj*/,
        Kart::Reaction reactionOnKart, Kart::Reaction /*reactionOnObj*/,
        EGG::Vector3f & /*hitDepth*/) {
    if (m_explodeTimer > 0) {
        return Kart::Reaction::ExplosionLoseItem;
    }

    return reactionOnKart;
}

/// @addr{0x80770F4C}
void ObjectKoopaFireball::calcTangible() {
    auto railStatus = m_railInterpolator->calc();

    switch (railStatus) {
    case RailInterpolator::Status::SegmentEnd:
        m_collisionTranslation.y = 0.0f;
        m_railInterpolator->setCurrVel(400.0f / 3.0f);
        break;
    case RailInterpolator::Status::ChangingDirection: {
        m_state = State::Intangible;
        m_flags |= 8;
        m_curScale = SCALE_INITIAL;
        m_scale = EGG::Vector3f(SCALE_INITIAL, SCALE_INITIAL, SCALE_INITIAL);
        m_explodeTimer = m_animFramecount;
    } break;
    default:
        break;
    }

    m_flags |= 1;
    m_pos = m_railInterpolator->curPos();
    m_collisionTranslation.y = m_collisionTranslation.y - 2.0f;
    m_pos.y = m_collisionTranslation.y + m_pos.y;

    checkSphereFull();

    m_angleRad += -m_angSpeed * DEG2RAD;

    EGG::Matrix34f mat = EGG::Matrix34f::ident;
    EGG::Vector3f vRot = EGG::Vector3f(m_angleRad, 0.0f, 0.0f);
    mat.makeR(vRot);
    m_flags |= 4;
    m_transform = m_transform.multiplyTo(mat);
    m_transform.setBase(3, m_pos);
}

/// @addr{0x80771324}
void ObjectKoopaFireball::calcExploding() {
    constexpr u32 EXPLODE_COLLISION_DURATION = 20;

    if (m_explodeTimer > 30) {
        m_flags |= 8;
        m_curScale += SCALE_DELTA;
        m_scale = EGG::Vector3f(m_curScale, m_curScale, m_curScale);
    }

    if (m_explodeTimer == EXPLODE_COLLISION_DURATION) {
        disableCollision();
    }

    if (--m_explodeTimer == 0) {
        m_curScale = 1.01f;
        m_flags |= 8;
        m_scale = EGG::Vector3f(1.01f, 1.01f, 1.01f);
        m_railInterpolator->init(0.0f, 0);
        m_pos = m_railInterpolator->curPos();
        m_flags |= 1;
        enableCollision();
        m_state = State::Intangible;
    }
}

/// @addr{0x80771248}
void ObjectKoopaFireball::calcIntangible() {
    if (m_cooldownTimer < 0) {
        m_state = State::Tangible;
        m_railInterpolator->setCurrVel(400.0f);
        m_angSpeed = 3.0f;
        m_collisionTranslation.y = -30.0f;
        m_cooldownTimer = 210;
    }

    --m_cooldownTimer;

    EGG::Vector3f scaledTanDir =
            m_railInterpolator->curTangentDir() * m_railInterpolator->getCurrVel();

    m_collisionTranslation.x = scaledTanDir.x;
    m_collisionTranslation.z = scaledTanDir.z;
}

/// @addr{0x80771624}
void ObjectKoopaFireball::checkSphereFull() {
    CollisionInfo info;
    info.bbox.setZero();

    EGG::Vector3f pos = m_pos;
    pos.y += -900.0f;

    if (CollisionDirector::Instance()->checkSphereFull(100.0f, pos, EGG::Vector3f::inf,
                KCL_TYPE_FLOOR, &info, nullptr, 0)) {
        m_collisionTranslation.y *= -0.4f;
        m_angSpeed = std::max(10.0f, m_angSpeed);
        m_pos += info.tangentOff;
        m_flags |= 1;
    }
}

} // namespace Field
