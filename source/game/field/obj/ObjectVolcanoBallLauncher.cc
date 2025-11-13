#include "ObjectVolcanoBallLauncher.hh"

#include "game/field/RailManager.hh"
#include "game/field/obj/ObjectVolcanoBall.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x806E3458}
ObjectVolcanoBallLauncher::ObjectVolcanoBallLauncher(const System::MapdataGeoObj &params)
    : ObjectCollidable(params) {
    m_initDelay = static_cast<f32>(static_cast<s16>(params.setting(1)));
    m_cycleDuration = static_cast<f32>(static_cast<s16>(params.setting(2)));

    const auto *rail = RailManager::Instance()->rail(params.pathId());
    ASSERT(rail);
    const auto &points = rail->points();

    f32 pos = 0.0f;

    for (size_t i = 0; i < points.size(); ++i) {
        const auto &point = points[i];
        if (point.setting[0] == 1) {
            pos = point.pos.y;
            break;
        }
    }

    EGG::Vector3f delta = points[1].pos - points[0].pos;
    delta.normalise2();
    f32 scale = static_cast<f32>(static_cast<s16>(params.setting(0)));
    f32 dVar9 = delta.y * scale;
    m_c8 = dVar9 * dVar9 / (2.0f * (pos - points[0].pos.y));

    f32 lastPosY = points.back().pos.y;
    f32 root1 = -1.0f;
    f32 root2 = -1.0f;
    EGG::Mathf::FUN_800867C0(0.5f * m_c8, -dVar9, lastPosY - points[0].pos.y, root1, root2);

    if (root2 > 0.0f) {
        m_cc = root2;
    } else if (root1 > 0.0f) {
        m_cc = root1;
    } else {
        m_cc = -1.0f;
    }

    m_d0 = 2.0f * m_c8 * (pos - lastPosY);

    u32 ballCount =
            static_cast<u32>((m_cc + static_cast<f32>(static_cast<s16>(params.setting(3)))) /
                    m_cycleDuration) +
            2;

    m_balls = std::span<ObjectVolcanoBall *>(new ObjectVolcanoBall *[ballCount], ballCount);

    for (auto *&ball : m_balls) {
        ball = new ObjectVolcanoBall(m_c8, m_d0, lastPosY, params, delta * scale);
        ball->load();
    }

    m_d4 = false;
}

/// @addr{0x806E384C}
ObjectVolcanoBallLauncher::~ObjectVolcanoBallLauncher() {
    delete[] m_balls.data();
}

/// @addr{0x806E388C}
void ObjectVolcanoBallLauncher::init() {
    for (auto *&ball : m_balls) {
        ball->init();
        ball->m_nextStateId = 0;
    }

    m_c0 = 0;
    m_currBallIdx = 0;
}

/// @addr{0x806E3920}
void ObjectVolcanoBallLauncher::calc() {
    u32 timer = System::RaceManager::Instance()->timer();

    if (m_d4) {
        m_c0 = (timer - static_cast<s32>(m_initDelay)) % static_cast<s32>(m_cycleDuration);

        if (static_cast<f32>(m_c0) == m_cycleDuration - 1.0f) {
            m_balls[m_currBallIdx++]->m_nextStateId = 1;
        }
    } else {
        m_c0 = timer;

        if (static_cast<f32>(timer) == m_initDelay) {
            m_balls[m_currBallIdx++]->m_nextStateId = 1;
            m_d4 = true;
        }
    }

    if (m_currBallIdx == m_balls.size()) {
        m_currBallIdx = 0;
    }
}

} // namespace Field
