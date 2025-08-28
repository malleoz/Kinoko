#include "ObjectBird.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x8077BD80}
ObjectBird::ObjectBird(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
    m_subB0 = new ObjectBirdSubB0(params, this);
    m_subB0->load();

    u32 count = params.setting(1);
    if (count == 0) {
        count = 5;
    }

    m_subBC = std::span<ObjectBirdSubBC *>(new ObjectBirdSubBC *[count], count);

    for (u32 i = 0; i < count; ++i) {
        auto *bird = new ObjectBirdSubBC(params, this, i);
        m_subBC[i] = bird;
        bird->load();
    }
}

/// @addr{0x8077CDC8}
ObjectBird::~ObjectBird() {
    delete[] m_subBC.data();
}

/// @addr{0x8077BFC8}
void ObjectBird::calc() {
    for (u32 i = 0; i < m_subBC.size() - 1; ++i) {
        const auto &pos = m_subBC[i]->pos();

        for (u32 j = i + 1; j < m_subBC.size(); ++j) {
            const auto &jPos = m_subBC[j]->pos();
            EGG::Vector3f posDelta = pos - jPos;
            f32 len = posDelta.length();

            if (len >= 300.0f) {
                continue;
            }

            posDelta.normalise();
            posDelta = jPos - posDelta * (300.0f - len);
            m_subBC[j]->setPos(posDelta);
        }
    }
}

/// @addr{0x8077C2F4}
ObjectBirdSubB0::ObjectBirdSubB0(const System::MapdataGeoObj &params, ObjectBird *bird)
    : ObjectCollidable(params), m_bird(bird) {}

/// @addr{0x8077CE48}
ObjectBirdSubB0::~ObjectBirdSubB0() = default;

/// @addr{0x8077C384}
void ObjectBirdSubB0::init() {
    m_ac = EGG::Vector3f::ez;

    auto *anmMgr = m_drawMdl->anmMgr();
    anmMgr->playAnim(0.0f, 1.0f, 0);
    f32 frameCount = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();

    auto &rand = System::RaceManager::Instance()->random();
    f32 rate = rand.getF32(static_cast<f32>(frameCount));
    anmMgr->playAnim(rate, 1.0f, 0);

    m_railInterpolator->init(0.0f, 0);
    m_railInterpolator->calc();
    m_pos = m_railInterpolator->curPos();
    m_flags.setBit(eFlags::Position);
    m_railInterpolator->setCurrVel(static_cast<f32>(m_mapObj->setting(0)));
}

/// @addr{0x8077C504}
void ObjectBirdSubB0::calc() {
    m_railInterpolator->calc();
    m_pos = m_railInterpolator->curPos();
    m_flags.setBit(eFlags::Position);
}

/// @addr{0x8077CC78}
void ObjectBirdSubB0::loadAnims() {
    std::array<const char *, 1> names = {{
            "flying",
    }};

    std::array<Render::AnmType, 1> types = {{
            Render::AnmType::Chr,
    }};

    linkAnims(names, types);
}

/// @addr{0x8077C580}
ObjectBirdSubBC::ObjectBirdSubBC(const System::MapdataGeoObj &params, ObjectBird *bird, u32 idx)
    : ObjectBirdSubB0(params, bird), m_idx(idx) {}

/// @addr{0x8077CE88}
ObjectBirdSubBC::~ObjectBirdSubBC() = default;

/// @addr{0x8077C5E0}
void ObjectBirdSubBC::init() {
    m_ac = EGG::Vector3f::ez;

    auto *anmMgr = m_drawMdl->anmMgr();
    anmMgr->playAnim(0.0f, 1.0f, 0);
    f32 frameCount = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();

    auto &rand = System::RaceManager::Instance()->random();
    f32 rate = rand.getF32(static_cast<f32>(frameCount));
    anmMgr->playAnim(rate, 1.0f, 0);

    m_d0 = static_cast<f32>(m_mapObj->setting(0));
    m_c4 = m_bird->subB0()->ac() * m_d0;

    f32 z = rand.getF32(1000.0f) - 500.0f;
    f32 y = rand.getF32(1000.0f) - 500.0f;
    f32 x = rand.getF32(1000.0f) - 500.0f;
    EGG::Vector3f delta = EGG::Vector3f(x, y, z);

    m_pos = m_bird->subB0()->pos() + delta;
    m_flags.setBit(eFlags::Position);
}

/// @addr{0x8077C7F0}
void ObjectBirdSubBC::calc() {
    FUN_8077C8F4();

    EGG::Vector3f local_7c = EGG::Vector3f(m_c4.x, 0.0f, m_c4.z);
    local_7c.normalise();

    CollisionInfo info;

    if (CollisionDirector::Instance()->checkSphereFull(100.0f, m_pos, EGG::Vector3f::inf,
                KCL_TYPE_FLOOR, &info, nullptr, 0)) {
        m_pos += info.tangentOff;
        m_flags.setBit(eFlags::Position);
    }
}

/// @addr{0x8077C8F4}
void ObjectBirdSubBC::FUN_8077C8F4() {
    EGG::Vector3f pos = m_bird->subB0()->pos();

    const auto &subBC = m_bird->subBC();
    u32 count = 1;

    for (u32 i = 0; i < subBC.size(); ++i) {
        if (i == m_idx) {
            continue;
        }

        ++count;
        pos += subBC[i]->pos();
    }

    pos *= (1.0f / static_cast<f32>(count));

    EGG::Vector3f posDelta = pos - m_pos;
    posDelta.normalise();
    posDelta *= 0.5f;

    m_c4 += posDelta;

    if (m_c4.length() > m_d0 * 1.2f) {
        m_c4.normalise();
        m_c4 *= m_d0 * 1.2f;
    }

    m_pos += m_c4;
    m_flags.setBit(eFlags::Position);
}

} // namespace Field
