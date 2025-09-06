#include "ObjectWanwan.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/obj/ObjectPile.hh"

#include "game/kart/KartObject.hh"

#include "game/system/RaceConfig.hh"
#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x806E4224}
ObjectWanwan::ObjectWanwan(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), StateManager(this), m_110(0.0f), m_4fc(0.0f), m_508(0.0f),
      m_518(false) {
    constexpr EGG::Vector3f MARIO_CIRCUIT_INIT_POS = EGG::Vector3f(14500.0f, 1300.0f, 44850.0f);
    constexpr EGG::Vector3f GCN_MARIO_CIRCUIT_INIT_POS = EGG::Vector3f(8012.0f, 1668.0f, -30150.0f);
    constexpr EGG::Vector3f UNK_1 = EGG::Vector3f(0.0f, 20.0f, 0.0f);
    constexpr EGG::Vector3f CHN_SCALE = EGG::Vector3f(2.0f, 2.0f, 2.0f);

    m_lurchDistance = static_cast<f32>(params.setting(0));
    m_43c = static_cast<f32>(params.setting(2)) + 4800.0f;
    m_angle1 = 10.0f * static_cast<f32>(params.setting(3));
    m_angle2 = 10.0f * static_cast<f32>(params.setting(4));
    m_idleFrames = static_cast<u32>(params.setting(5));
    m_4e4 = static_cast<f32>(params.setting(6));

    if (m_idleFrames == 0) {
        m_idleFrames = 300.0f;
    }

    if (m_4e4 == 0.0f) {
        m_4e4 = 30.0f;
    }

    m_pile = new ObjectPile(m_pos, m_rot, m_scale);
    m_pile->load();

    m_474 = m_pos + UNK_1;

    m_scale = EGG::Vector3f(2.0f, 2.0f, 2.0f);
    m_flags.setBit(eFlags::Scale);

    f32 dVar18 = m_lurchDistance * m_lurchDistance + 300.0f * (300.0f * m_scale.y) * m_scale.y;

    f32 fVar1 = 0.0f;
    if (dVar18 > 0.0f) {
        fVar1 = dVar18 * EGG::Mathf::frsqrt(dVar18);
    }

    u32 chainCount = static_cast<u32>(fVar1 / (135.0f * m_scale.y));
    if (chainCount % 2 == 1) {
        --chainCount;
    }

    m_chains = std::span<ObjectWanwanChain *>(new ObjectWanwanChain *[chainCount], chainCount);

    for (auto *&chain : m_chains) {
        chain = new ObjectWanwanChain(params);
        chain->m_flags.setBit(eFlags::Scale);
        chain->m_scale = CHN_SCALE;
        chain->load();
    }

    m_468 = m_474 + EGG::Vector3f(0.5f * -m_lurchDistance, 600.0f, 0.5f * -m_lurchDistance);

    switch (System::RaceConfig::Instance()->raceScenario().course) {
    case Course::Mario_Circuit: {
        EGG::Vector3f pos = MARIO_CIRCUIT_INIT_POS - m_474;
        pos.y = 0.0f;
        pos.normalise2();
        m_initPos = pos * m_lurchDistance + m_474;
    } break;
    case Course::GCN_Mario_Circuit: {
        EGG::Vector3f pos = GCN_MARIO_CIRCUIT_INIT_POS - m_474;
        pos.y = 0.0f;
        pos.normalise2();
        m_initPos = pos * m_lurchDistance + m_474;
    } break;
    default: {
        m_initPos = m_474;
        m_initPos.z += m_lurchDistance;
    }
    }
}

/// @addr{0x806E4AEC}
ObjectWanwan::~ObjectWanwan() {
    delete m_chains.data();
}

/// @addr{0x806E4B9C}
void ObjectWanwan::init() {
    m_pos = m_468;
    m_flags.setBit(eFlags::Position);
    m_450 = m_468;
    m_velocity = EGG::Vector3f::zero;
    m_accel = EGG::Vector3f::zero;
    m_10c = 0.0f;
    m_tangent = EGG::Vector3f::ex;
    m_120 = EGG::Vector3f::ey;
    m_floorNrm = EGG::Vector3f::ey;
    m_bTouchingFloor = false;
    m_494 = false;
    m_49c = 135.0f * m_chains[0]->m_scale.y;
    m_4a0 = EGG::Vector3f::zero;
    m_4ac = EGG::Vector3f::ez;
    m_4b8 = false;
    m_4bc = EGG::Vector3f::ez;
    m_4c8 = 0.0f;
    m_4cc = 0.0f;
    m_4d4 = 0;
    m_4d8 = false;
    m_4f8 = -1;
    m_4ec = EGG::Vector3f::ex;
    m_nextStateId = 0;
}

/// @addr{0x806E4F2C}
void ObjectWanwan::calc() {
    if (m_nextStateId >= 0) {
        m_currentStateId = m_nextStateId;
        m_nextStateId = -1;
        m_currentFrame = 0;

        auto enterFunc = m_entries[m_entryIds[m_currentStateId]].onEnter;
        (this->*enterFunc)();
    } else {
        ++m_currentFrame;
    }

    auto calcFunc = m_entries[m_entryIds[m_currentStateId]].onCalc;
    (this->*calcFunc)();

    m_velocity += m_accel - EGG::Vector3f(0.0f, 2.5f, 0.0f);
    m_pos += m_velocity;
    m_flags.setBit(eFlags::Position); // m_pos wrong t=51
    m_accel.setZero();

    calcLurch();
    checkCollision();

    EGG::Matrix34f mat;

    if (m_currentStateId == 5) {
        SetRotTangentHorizontal(mat, EGG::Vector3f::ey, m_tangent);
    } else {
        SetRotTangentHorizontal(mat, m_120, m_tangent);
    }

    mat.setBase(3, EGG::Vector3f::zero);

    EGG::Vector3f rot = EGG::Vector3f(m_110 * DEG2RAD, 0.0f, 0.0f);
    EGG::Matrix34f mat48;
    mat48.makeRT(rot, EGG::Vector3f::zero);

    if (m_4f8 == -1) {
        mat = mat.multiplyTo(mat48);
    }

    mat.setBase(3, m_pos);
    m_flags.setBit(eFlags::Matrix);
    m_transform = mat;
    m_pos = mat.base(3);

    if (m_4f8 != -1) {
        FUN_806E7638();
    }

    calcChain();

    if (m_pos.y < m_474.y - 1000.0f) {
        m_flags.setBit(eFlags::Position);
        m_pos.y = m_474.y + 1000.0f;
        m_velocity.y = 0.0f;
    }
}

/// @addr{0x806E526C}
Kart::Reaction ObjectWanwan::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) {
    // Assume reactionObObj is None
    if (kartObj->speedRatioCapped() < 0.5f && (m_currentStateId != 5 || m_4d8)) {
        return Kart::Reaction::WallAllSpeed;
    }

    return reactionOnKart;
}

/// @addr{0x806E9084}
void ObjectWanwan::calcLurch() {
    if (m_currentStateId != 5) {
        return;
    }

    EGG::Matrix34f mat;
    SetRotTangentHorizontal(mat, EGG::Vector3f::ey, m_tangent);
    mat.setBase(3, EGG::Vector3f::zero);
    EGG::Vector3f local_1a8 = EGG::Vector3f::zero;
    local_1a8.x = m_110 * DEG2RAD;

    EGG::Matrix34f mStack_88;
    mStack_88.makeRT(local_1a8, EGG::Vector3f::zero);

    if (m_4f8 == -1) {
        mat = mat.multiplyTo(mStack_88);
    }

    mat.setBase(3, m_pos);

    m_flags.setBit(eFlags::Matrix);
    m_transform = mat;
    m_pos = mat.base(3);

    calcTransform();

    mat = m_transform;
    mat.setBase(3, mat.base(3) - mat.base(2) * 250.0f * m_scale.x);

    EGG::Vector3f vStack_118 = mat.base(3) - mat.base(2) * 140.0f * m_scale.x;
    EGG::Vector3f vStack_148 = mat.base(1) * 20.0f * m_scale.x;
    m_450 = vStack_118 - vStack_148;

    EGG::Vector3f vStack_c4 = m_450 - m_474;
    m_4e8 = vStack_c4.normalise();
    f32 norm = m_4e8 - m_49c * static_cast<f32>(m_chains.size());

    if (norm > 0.0f || m_4d8) {
        m_494 = true;
        m_pos = m_pos - vStack_c4 * norm + vStack_c4 * 35.0f;
        m_flags.setBit(eFlags::Position);
    }
}

/// @addr{0x806E7BA4}
void ObjectWanwan::calcChain() {
    ;
}

/// @addr{0x806E5A8C}
void ObjectWanwan::checkCollision() {
    constexpr f32 RADIUS = 30.0f;
    constexpr EGG::Vector3f POS_OFFSET = EGG::Vector3f(0.0f, RADIUS - 600.0f, 0.0f);

    m_bTouchingFloor = false;

    CollisionInfo info;
    KCLTypeMask maskOut;

    auto *colDir = CollisionDirector::Instance();

    if (!colDir->checkSphereFullPush(RADIUS, m_pos + POS_OFFSET, EGG::Vector3f::inf, KCL_TYPE_FLOOR,
                &info, &maskOut, 0)) {
        return;
    }

    m_bTouchingFloor = true;

    f32 scale = info.tangentOff.normalise();
    m_pos += EGG::Vector3f::ey * scale;
    m_flags.setBit(eFlags::Position);

    if (info.floorDist > -std::numeric_limits<f32>::min()) {
        m_floorNrm = info.floorNrm;
    }

    m_accel += EGG::Vector3f(0.0f, 2.5f, 0.0f);
}

/// @addr{0x806E7638}
void ObjectWanwan::FUN_806E7638() {
    constexpr f32 RADIUS = 30.0f;
    constexpr EGG::Vector3f POS_OFFSET = EGG::Vector3f(0.0f, RADIUS - 600.0f, 0.0f);

    if (m_518) {
        m_4fc = FUN_806B59A8(0.6f * 80.0f, 2.5f, static_cast<u32>(m_4f8) - m_508);
    } else {
        m_4fc = FUN_806B59A8(80.0f, 2.5f, static_cast<u32>(m_4f8));
    }

    EGG::Vector3f pos = m_pos + EGG::Vector3f::ey * m_4fc + POS_OFFSET;

    bool hasCol = CollisionDirector::Instance()->checkSpherePartial(50.0f, pos, EGG::Vector3f::inf,
            KCL_TYPE_64EBDFFF, nullptr, nullptr, 0);

    if (hasCol && m_4f8 > 5 && !m_518) {
        m_518 = true;
        m_4fc = 0.0f;
        m_508 = static_cast<f32>(m_4f8);
    } else if (hasCol && static_cast<f32>(m_4f8) > m_508 + 1.0f && m_518) {
        m_4f8 = -1;
        m_4fc = 0.0f;
        enableCollision();
        calcTransform();
        m_120 = m_transform.base(1);

        return;
    }

    ++m_4f8;
}

/// @addr{0x806E79E4}
void ObjectWanwan::FUN_806E79E4() {
    if (m_4d4 >= m_idleFrames) {
        m_nextStateId = 5;
    }

    ++m_4d4;
}

/// @addr{0x806E8520}
void ObjectWanwan::FUN_806E8520() {
    FUN_806E87C8();
}

/// @addr{0x806E87C8}
void ObjectWanwan::FUN_806E87C8() {
    f32 fVar4 = System::RaceManager::Instance()->random().getF32(2.0f * m_4e4);
    EGG::Vector3f local_3c = EGG::Vector3f(m_angle1, 0.0f, m_angle2);
    EGG::Vector3f vStack_48 = local_3c - m_474;
    vStack_48.y = 0.0f;
    vStack_48.normalise2();

    m_4a0 = m_474 + FUN_806B3900((fVar4 - m_4e4) * DEG2RAD, vStack_48) * m_43c;
    m_4ac = m_4a0 - m_pos;
    m_4ac.y = 0.0f;
    m_4ac.normalise2();
}

void ObjectWanwan::enterStateStub() {
    ;
}

/// @addr{0x806E6208}
void ObjectWanwan::enterState0() {
    m_4b8 = false;
    m_velocity.x = 0.0f;
    m_velocity.z = 0.0f;
    m_accel.x = 0.0f;
    m_accel.z = 0.0f;
    m_10c = 0.0f;

    f32 rand = System::RaceManager::Instance()->random().getF32(0.33f * 60.0f);
    f32 dVar1 = 0.66f * 60.0f + rand;

    EGG::Vector3f vStack_58 = m_pos + m_tangent;
    EGG::Vector3f vStack_64 = m_pos + EGG::Vector3f::zero;

    if (FUN_806B38A8(vStack_58, vStack_64, m_474) >= 0.0f) {
        dVar1 *= -1.0f;
    }

    EGG::Vector3f local_34 = EGG::Vector3f(m_pos.x, m_474.y, m_pos.z);
    EGG::Vector3f local_40 = m_474 - local_34;
    local_40.normalise();
    EGG::Vector3f vStack_4c = FUN_806B3900(dVar1 * DEG2RAD, local_40);
    f32 fVar2 = m_lurchDistance < 3000.0f ? 0.5f : 0.7f;
    m_4a0 = vStack_4c * m_lurchDistance * fVar2 + m_474;
}

/// @addr{0x806E6730}
void ObjectWanwan::enterState1() {
    m_velocity.x = 0.0f;
    m_velocity.z = 0.0f;
    m_accel.x = 0.0f;
    m_accel.z = 0.0f;
    m_10c = 0.0f;
    m_4cc = 0.0f;

    auto &random = System::RaceManager::Instance()->random();
    f32 rand = random.getF32(0.33f * 120.0f);
    m_4c8 = 0.66f * 120.0f + rand;

    rand = random.getF32(1.0f);

    if (rand >= 0.5f) {
        m_4c8 *= -1.0f;
    }

    m_4bc = m_4a0 - m_474;
}

/// @addr{0x806E6B58}
void ObjectWanwan::enterState2() {
    m_velocity.x = 0.0f;
    m_velocity.z = 0.0f;
    m_accel.x = 0.0f;
    m_accel.z = 0.0f;
    m_10c = 0.0f;
    m_110 = 0.0f;
}

/// @addr{0x806E6EB0}
void ObjectWanwan::enterState5() {
    m_velocity.x = 0.0f;
    m_velocity.z = 0.0f;
    m_accel.x = 0.0f;
    m_accel.z = 0.0f;
    m_10c = 0.0f;
    m_110 = 0.0f;
    m_4d4 = 0;
    m_4d8 = false;
    m_494 = false;
    m_474.y += 30.0f;

    FUN_806E8520();
}

/// @addr{0x806E73C4}
void ObjectWanwan::enterState6() {
    m_velocity.x = 0.0f;
    m_velocity.z = 0.0f;
    m_accel.x = 0.0f;
    m_accel.z = 0.0f;
    m_10c = 15.0f;
    m_110 = -(-30.0f);
    m_4ec = m_474 - m_pos;
    m_4ec.normalise2();
    m_4d8 = false;
    m_494 = false;
    m_474.y -= 30.0f;
}

void ObjectWanwan::calcStateStub() {
    ;
}

/// @addr{0x806E63A4}
void ObjectWanwan::calcState0() {
    EGG::Vector3f local_44 = m_4a0 - m_pos;
    local_44.y = 0.0f;

    // In the base game, r31 is never set if we fail the epsilon check.
    // In this scenario, the register will end up holding Pi.
    f32 in_f31;
    if (local_44.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        in_f31 = local_44.normalise();
    } else {
        in_f31 = F_PI;
    }

    if (!m_4b8 && in_f31 < 0.55f * m_lurchDistance) {
        EGG::Vector3f vStack_50 = m_4a0 - m_474;
        f32 rand = System::RaceManager::Instance()->random().getF32(0.33f * (0.5f * 60.0f));
        f32 dVar2 = 0.66f * (0.5f * 60.0f) + rand;

        if (FUN_806B38A8(m_4a0, m_initPos, m_474) >= 0.0f) {
            dVar2 *= -1.0f;
        }

        EGG::Vector3f vStack_5c = FUN_806B3900(dVar2 * DEG2RAD, vStack_50);
        m_4a0 = vStack_5c + m_474;
        m_4b8 = true;
    }

    if (in_f31 < 300.0f) {
        m_nextStateId = 2;
    }

    m_4ac = local_44;
    m_tangent = Interpolate(0.04f, m_tangent, m_4ac);

    if (m_tangent.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_tangent.normalise2();
    } else {
        m_tangent = m_4ac;
    }

    m_120 = Interpolate(0.1f, m_120, m_floorNrm);

    if (m_120.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_120.normalise2();
    } else {
        m_120 = EGG::Vector3f::ey;
    }

    if (m_10c < 8.0f) {
        m_10c += 0.5f;
        EGG::Vector3f scaledTan = m_tangent * 0.5f;
        m_accel.x = scaledTan.x;
        m_accel.z = scaledTan.z;
    } else {
        m_10c = 8.0f;
        m_accel.x = 0.0f;
        m_accel.z = 0.0f;
        m_velocity.x = m_tangent.x * 8.0f;
        m_velocity.z = m_tangent.z * 8.0f;
    }

    if (m_bTouchingFloor) {
        m_velocity.y = 0.0f;
        m_accel += EGG::Vector3f::ey * 12.0f;
    } else {
        m_accel.y = 0.0f;
    }

    FUN_806E79E4();
}

/// @addr{0x806E6838}
void ObjectWanwan::calcState1() {
    if (EGG::Mathf::abs(m_4cc) < EGG::Mathf::abs(m_4c8)) {
        if (m_4c8 < 0.0f) {
            m_4cc -= 0.3f;
        } else {
            m_4cc += 0.3f;
        }
    }

    m_4a0 = FUN_806B3900(m_4cc * DEG2RAD, m_4bc) + m_474;
    m_4ac = m_4a0 - m_pos;
    m_4ac.y = 0.0f;
    f32 norm = m_4ac.normalise();

    m_4bc *= 0.999f;

    if (EGG::Mathf::abs(m_4c8) < EGG::Mathf::abs(m_4cc) && norm < 400.0f) {
        m_nextStateId = 0;
    }

    m_tangent = Interpolate(0.04f, m_tangent, m_4ac);

    if (m_tangent.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_tangent.normalise2();
    } else {
        m_tangent = m_4ac;
    }

    m_120 = Interpolate(0.1f, m_120, m_floorNrm);

    if (m_120.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_120.normalise2();
    } else {
        m_120 = EGG::Vector3f::ey;
    }

    if (m_10c < 8.0f) {
        m_10c += 0.5f;
        EGG::Vector3f scaledTan = m_tangent * 0.5f;
        m_accel.x = scaledTan.x;
        m_accel.z = scaledTan.z;
    } else {
        m_10c = 8.0f;
        m_accel.x = 0.0f;
        m_accel.z = 0.0f;
        m_velocity.x = m_tangent.x * 8.0f;
        m_velocity.z = m_tangent.z * 8.0f;
    }

    if (m_bTouchingFloor) {
        m_velocity.y = 0.0f;
        m_accel += EGG::Vector3f::ey * 12.0f;
    } else {
        m_accel.y = 0.0f;
    }

    FUN_806E79E4();
}

/// @addr{0x806E6B7C}
void ObjectWanwan::calcState2() {
    constexpr f32 UNK_60 = -60.0f;

    m_110 += UNK_60 / 25.0f;

    if (EGG::Mathf::abs(m_110) > EGG::Mathf::abs(UNK_60)) {
        m_nextStateId = 3;
    }

    if (m_bTouchingFloor) {
        m_velocity.y = 0.0f;
        m_accel += EGG::Vector3f::ey * 12.0f;
    } else {
        m_accel.y = 0.0f;
    }

    FUN_806E79E4();
}

/// @addr{0x806E6C98}
void ObjectWanwan::calcState3() {
    f32 sin = EGG::Mathf::SinFIdx(
            DEG2FIDX * ((360.0f * (0.25f * 35.0f + static_cast<f32>(m_currentFrame))) / 35.0f));
    m_110 = -60.0f * sin;

    if (m_bTouchingFloor) {
        m_velocity.y = 0.0f;
        m_accel += EGG::Vector3f::ey * 12.0f;
    } else {
        m_accel.y = 0.0f;
    }

    FUN_806E79E4();

    if (static_cast<f32>(m_currentFrame) > 0.5f * 35.0f) {
        m_nextStateId = 4;
    }
}

/// @addr{0x806E6DD8}
void ObjectWanwan::calcState4() {
    m_110 += -60.0f / 15.0f;

    if (EGG::Mathf::abs(m_110) < 2.0f) {
        m_nextStateId = 1;
    }

    if (m_bTouchingFloor) {
        m_velocity.y = 0.0f;
        m_accel += EGG::Vector3f::ey * 12.0f;
    } else {
        m_accel.y = 0.0f;
    }

    FUN_806E79E4();
}

/// @addr{0x806E6F6C}
void ObjectWanwan::calcState5() {
    if (m_currentFrame > 120) {
        m_nextStateId = 6;
    }

    if (EGG::Mathf::abs(m_110) < EGG::Mathf::abs(-30.0f)) {
        m_110 -= -30.0f / 25.0f;
    }

    m_tangent = Interpolate(10.0f * 0.04f, m_tangent, m_4ac);

    if (m_tangent.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_tangent.normalise2();
    } else {
        m_tangent = m_4ac;
    }

    m_120 = Interpolate(0.1f, m_120, m_floorNrm);

    if (m_120.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_120.normalise2();
    } else {
        m_120 = EGG::Vector3f::ey;
    }

    if (!m_494 && !m_4d8) { // m_494 wrong
        m_velocity.x = m_tangent.x * 120.0f;
        m_velocity.z = m_tangent.z * 120.0f;

        return;
    }

    m_4a0 = m_474 + (m_450 - m_474) * 2.0f;
    m_4ac = m_4a0 - m_pos;
    m_4ac.normalise2();

    EGG::Vector3f local_44 = m_pos - m_450;
    local_44.y = 0.0f;
    f32 fVar6 = local_44.length();
    EGG::Vector3f local_50 = m_450 - m_474; // m_450 wrong
    local_50.y = 0.0f;
    local_50.normalise2();
    m_flags.setBit(eFlags::Position);
    m_pos.x = m_450.x + local_50.x * fVar6; // local_50 wrong
    m_pos.z = m_450.z + local_50.z * fVar6;
    EGG::Vector3f local_5c = m_tangent + local_50;
    local_5c.y = 0.0f;
    if (local_5c.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        local_5c.normalise2();
    }

    m_tangent = local_5c;
    m_4d8 = true;
    m_velocity.y = 0.0f;
    m_velocity.x *= -0.85f;
    m_velocity.y = -0.0f;
    m_velocity.z *= -0.85f;
}

/// @addr{0x806E7494}
void ObjectWanwan::calcState6() {
    m_velocity.x = 1.5f * m_4ec.x * m_10c;
    m_velocity.z = 1.5f * m_4ec.z * m_10c;

    if (m_bTouchingFloor) {
        m_velocity.y = 0.0f;
        m_accel += EGG::Vector3f::ey * 12.0f;
    } else {
        m_accel.y = 0.0f;
    }

    if (m_currentFrame > 90) {
        m_nextStateId = 0;
    }
}

const std::array<StateManagerEntry<ObjectWanwan>, 7> StateManager<ObjectWanwan>::STATE_ENTRIES = {{
        {0, &ObjectWanwan::enterState0, &ObjectWanwan::calcState0},
        {1, &ObjectWanwan::enterState1, &ObjectWanwan::calcState1},
        {2, &ObjectWanwan::enterState2, &ObjectWanwan::calcState2},
        {3, &ObjectWanwan::enterStateStub, &ObjectWanwan::calcState3},
        {4, &ObjectWanwan::enterStateStub, &ObjectWanwan::calcState4},
        {5, &ObjectWanwan::enterState5, &ObjectWanwan::calcState5},
        {6, &ObjectWanwan::enterState6, &ObjectWanwan::calcState6},

}};

StateManager<ObjectWanwan>::StateManager(ObjectWanwan *obj) {
    constexpr size_t ENTRY_COUNT = 7;

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

StateManager<ObjectWanwan>::~StateManager() {
    delete[] m_entryIds.data();
}

} // namespace Field
