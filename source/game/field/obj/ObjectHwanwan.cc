#include "ObjectHwanwan.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/KColData.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartObject.hh"

#include "game/system/RaceConfig.hh"

namespace Field {

/// @addr{0x806E95B0}
ObjectHwanwanSubB0::ObjectHwanwanSubB0(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), StateManager(this, STATE_ENTRIES), m_initPos(m_pos), m_3f8(0.0f) {}

/// @addr{0x806EC6E0}
ObjectHwanwanSubB0::~ObjectHwanwanSubB0() = default;

/// @addr{0x806E9724}
void ObjectHwanwanSubB0::init() {
    m_3ac = m_initPos + EGG::Vector3f::ey * 400.0f;
    m_3b8.setZero();
    m_3c4.setZero();
    m_3f4 = 0.0f;
    m_tangent = EGG::Vector3f::ez;
    m_3dc = EGG::Vector3f::ey;
    m_3e8 = EGG::Vector3f::ey;
    m_3fc = 0;
    m_400 = false;
    m_408 = m_initPos.y;
    m_410 = 0.0f;
    m_498 = m_3ac.y;
    m_49c = 0;

    calcTransform();

    m_438 = m_transform;
    m_438.setBase(3, m_438.base(3) - m_438.base(2) * 250.0f * m_scale.y);
    m_468 = m_438;
    m_420 = m_438.base(3) - m_438.base(2) * 140.0f * m_scale.y + m_438.base(1) * 20.0f * m_scale.y;
    m_42c = m_420;
    m_nextStateId = 0;
}

/// @addr{0x806E9A78}
void ObjectHwanwanSubB0::calc() {
    m_40c = false;

    StateManager::calc();

    m_3b8 += m_3c4 - EGG::Vector3f(0.0f, 2.5f, 0.0f);
    m_3ac += m_3b8;
    m_3c4.setZero();

    checkFloorCollision();

    SetRotTangentHorizontal(m_transform, m_3dc, m_tangent);
    m_transform.setBase(3, m_3ac);
    m_pos = m_3ac;
}

/// @addr{0x806E9CAC}
Kart::Reaction ObjectHwanwanSubB0::onCollision(Kart::KartObject *kartObj,
        Kart::Reaction reactionOnKart, Kart::Reaction /*reactionOnObj*/,
        EGG::Vector3f & /*hitDepth*/) {
    return kartObj->speedRatioCapped() < 0.5f ? Kart::Reaction::WallAllSpeed : reactionOnKart;
}

/// @addr{0x806EA288}
void ObjectHwanwanSubB0::enterState2() {
    m_404 = -1;
    m_3f8 = 0.0f;
    m_410 = 150.0f;
    m_414 = EGG::Vector3f(30.0f, 0.4f, 1.0f);

    auto course = System::RaceConfig::Instance()->raceScenario().course;
    if (course == Course::Mario_Circuit && m_49c == 1) {
        m_410 = 50.0f;
        m_414.setZero();
    }
}

/// @addr{0x806E9E10}
void ObjectHwanwanSubB0::calcState0() {
    if (m_400) {
        m_3c4 += EGG::Vector3f::ey * 12.0f;
    }

    if (EGG::Mathf::abs(m_3f4) > 360.0f) {
        m_3f4 -= std::fmodf(m_3f4, 360.0f);
    }

    if (EGG::Mathf::abs(m_3f4) < 10.0f) {
        m_3f4 = 0.0f;
    } else {
        m_3f4 += 3.0f;
    }

    calc3F8();
    FUN_806EB184();
}

/// @addr{0x806E9FE4}
void ObjectHwanwanSubB0::calcState1() {
    m_3f4 += 6.0f;

    calc3F8();

    m_3fc = 9;
}

/// @addr{0x806EA2FC}
void ObjectHwanwanSubB0::calcState2() {
    if (m_404 != -1 && m_currentFrame > static_cast<u32>(m_404) + 10 && m_400) {
        m_nextStateId = 0;
        m_40c = true;
        FUN_806EB184();

        return;
    }

    if (m_currentFrame >= 10 && m_400) {
        m_404 = static_cast<s32>(m_currentFrame);
    }

    if (m_404 == -1) {
        m_3ac.y += m_410 - 2.5f * static_cast<f32>(m_currentFrame);

        m_3f8 += m_414.z;
        m_3f4 -= m_414.x - m_414.y * static_cast<f32>(m_currentFrame);

        m_3fc = 7;
    } else {
        m_3ac.y += 0.6f * m_410 - 2.5f * static_cast<f32>(m_currentFrame - static_cast<u32>(m_404));

        if (EGG::Mathf::abs(m_3f4) > 360.0f) {
            m_3f4 -= std::fmodf(m_3f4, 360.0f);
        }

        m_3f4 -= 1.5f;
        m_3f8 -= 2.5f * m_414.z;

        if (m_currentFrame <= static_cast<u32>(m_404) + 16) {
            m_3fc = 9;
        }
    }
}

void ObjectHwanwanSubB0::calc3F8() {
    if (EGG::Mathf::abs(m_3f8) > 360.0f) {
        m_3f8 -= std::fmodf(m_3f8, 360.0f);
    }

    if (EGG::Mathf::abs(m_3f8) < 2.0f) {
        m_3f8 = 0.0f;
    }

    if (m_3f8 > 0.0f) {
        m_3f8 -= 0.5f;
    } else {
        m_3f8 += 0.5f;
    }
}

/// @addr{0x806EA784}
void ObjectHwanwanSubB0::checkFloorCollision() {
    constexpr f32 RADIUS = 200.0f;

    m_400 = false;

    CollisionInfo colInfo;
    KCLTypeMask mask;
    EGG::Vector3f pos = m_3ac + EGG::Vector3f(0.0f, -400.0f, 0.0f);

    auto *colDir = CollisionDirector::Instance();
    bool hasCol = CollisionDirector::Instance()->checkSphereFullPush(RADIUS, pos,
            EGG::Vector3f::inf, KCL_TYPE_FLOOR, &colInfo, &mask, 0);

    if (!hasCol || pos.y - m_498 >= 300.0f) {
        return;
    }

    m_400 = true;

    f32 len = colInfo.tangentOff.length();
    m_3ac += EGG::Vector3f::ey * len;

    if (colInfo.floorDist > -std::numeric_limits<f32>::min()) {
        m_3e8 = colInfo.floorNrm;
    }
    m_3b8.y = 0.0f;
    m_408 = m_3ac.y;

    if (mask & KCL_TYPE_FLOOR) {
        colDir->findClosestCollisionEntry(&mask, KCL_ANY);
        m_49c = colDir->closestCollisionEntry()->variant();
    }
}

/// @addr{0x806EAAE8}
void ObjectHwanwanSubB0::FUN_806EAAE8() {
    m_3dc = Interpolate(0.1f, m_3dc, m_3e8);
    if (m_3dc.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_3dc.normalise2();
    } else {
        m_3dc = EGG::Vector3f::ey;
    }
}

/// @addr{0x806C5354}
ObjectHwanwan::ObjectHwanwan(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
    m_subB0 = new ObjectHwanwanSubB0(params);
    m_subB0->setScale(EGG::Vector3f(2.0f, 2.0f, 2.0f));
    m_subB0->load();
}

/// @addr{0x806C56DC}
ObjectHwanwan::~ObjectHwanwan() = default;

/// @addr{0x806C571C}
void ObjectHwanwan::init() {
    m_railInterpolator->init(0.0f, 0);
    m_subB0->m_tangent = m_railInterpolator->curTangentDir();
    const auto &curPos = m_railInterpolator->curPos();
    m_subB0->m_3ac.x = curPos.x;
    m_subB0->m_3ac.z = curPos.z;
    m_subB0->m_498 = curPos.y;

    m_subB0->calc();
    m_subB0->calc();

    ASSERT(m_mapObj);
    m_railInterpolator->setCurrVel(static_cast<f32>(m_mapObj->setting(0)));
}

/// @addr{0x806C5AC4}
void ObjectHwanwan::calc() {
    calcState();

    const auto &curPos = m_railInterpolator->curPos();
    m_subB0->m_3ac.x = curPos.x;
    m_subB0->m_3ac.z = curPos.z;
    m_subB0->m_498 = curPos.y;
    m_subB0->m_tangent = m_railInterpolator->curTangentDir();
}

/// @addr{0x806C5DE0}
void ObjectHwanwan::calcState() {
    if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd &&
            m_railInterpolator->curPoint().setting[1] == 1 && m_subB0->m_currentStateId != 2) {
        m_subB0->m_nextStateId = 1;
    }

    if (m_subB0->m_currentStateId == 1 && m_subB0->m_currentFrame >= 60) {
        m_subB0->m_nextStateId = 0;
    }
}

} // namespace Field
