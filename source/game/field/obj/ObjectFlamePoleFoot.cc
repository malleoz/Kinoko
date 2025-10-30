#include "ObjectFlamePoleFoot.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x8067E6F4}
ObjectFlamePoleFoot::ObjectFlamePoleFoot(const System::MapdataGeoObj &params)
    : ObjectKCL(params), StateManager(this) {
    m_160 = params.setting(0);
    m_initDelay = params.setting(1);
    m_168 = static_cast<f32>(params.setting(2));

    ++FLAMEPOLE_COUNT;

    if (m_168 == 0.0f) {
        m_168 = 3.0f + static_cast<f32>(FLAMEPOLE_COUNT % 3);
    }

    m_pole = new ObjectFlamePole(params, m_pos, m_rot, m_scale);
    m_pole->load();
}

/// @addr{0x8067EBE0}
ObjectFlamePoleFoot::~ObjectFlamePoleFoot() {
    FLAMEPOLE_COUNT = 0;
}

/// @addr{0x8067EC94}
void ObjectFlamePoleFoot::init() {
    m_nextStateId = 0;
    m_cycleFrame = 0;
    m_170 = static_cast<s32>(0.3f * (static_cast<f32>(DAT_808C0F58) * (7.0f - 1.0f)) / 7.0f);
    m_174 = static_cast<s32>(0.1f * (static_cast<f32>(DAT_808C0F58) * (7.0f - 1.0f)) / 7.0f);
    m_178 = static_cast<s32>(static_cast<f32>(DAT_808C0F58) * 1.0f / 7.0f);
    m_17c = static_cast<s32>(0.2f * (static_cast<f32>(DAT_808C0F58) * (7.0f - 1.0f)) / 7.0f);
    m_18c = m_170;
    m_190 = m_170 + m_174;
    m_194 = m_190 + m_178;
    m_180 = static_cast<s32>(0.4f * (static_cast<f32>(DAT_808C0F58) * (7.0f - 1.0f)) / 7.0f);
    m_198 = m_194 + m_17c;
    m_19c = m_198 + m_180;
    m_1a0 = (m_168 - 1.0f) / static_cast<f32>(m_18c);
    m_1a8 = ObjectFlamePole::HEIGHT * m_168;
    m_1b8 = (300.0f + m_1a8) / static_cast<f32>(m_17c);
    m_pole->setActive(false);
    m_pole->disableCollision();

    EGG::Vector3f polePos = m_pole->pos();
    polePos.y = m_pos.y - ObjectFlamePole::HEIGHT * m_168;
    m_pole->setPos(polePos);
}

/// @addr{0x8067EF70}
void ObjectFlamePoleFoot::calc() {
    if (System::RaceManager::Instance()->timer() < m_initDelay) {
        return;
    }

    calcStates();

    StateManager::calc();

    f32 scale = getScaleY(0);
    m_flags.setBit(eFlags::Scale);
    m_scale = EGG::Vector3f(scale, scale, scale);

    EGG::Vector3f polePos = m_pole->pos();
    m_pole->setPos(EGG::Vector3f(polePos.x, m_1b0 + (m_pos.y - m_1a8), polePos.z));
    m_pole->setScale(EGG::Vector3f(m_168, m_168, m_168));
}

/// @addr{0x8067F6B8}
void ObjectFlamePoleFoot::calcStates() {
    m_cycleFrame = static_cast<s32>(System::RaceManager::Instance()->timer() - m_initDelay);
    m_cycleFrame %= m_160 + DAT_808C0F58;

    // TODO: Make if-statement with timers in a std::array?
    if (m_cycleFrame >= m_19c) {
        if (m_currentStateId != 5) {
            m_nextStateId = 5;
        }
    } else if (m_cycleFrame >= m_198) {
        if (m_currentStateId != 4) {
            m_nextStateId = 4;
        }
    } else if (m_cycleFrame >= m_194) {
        if (m_currentStateId != 3) {
            m_nextStateId = 3;
        }
    } else if (m_cycleFrame >= m_190) {
        if (m_currentStateId != 2) {
            m_nextStateId = 2;
        }
    } else if (m_cycleFrame >= m_18c) {
        if (m_currentStateId != 1) {
            m_nextStateId = 1;
        }
    } else if (m_cycleFrame >= 0) {
        if (m_currentStateId != 0) {
            m_nextStateId = 0;
        }
    }
}

/// @addr{0x8067FBB8}
const EGG::Matrix34f &ObjectFlamePoleFoot::getUpdatedMatrix(u32 /*timeOffset*/) {
    calcTransform();
    m_workMatrix = m_transform;
    return m_transform;
}

f32 ObjectFlamePoleFoot::getScaleY(u32 timeOffset) const {
    u32 frame = System::RaceManager::Instance()->timer() - timeOffset;
    if (frame < m_initDelay) {
        return 1.0f;
    }

    s32 cycleFrame = frame - m_initDelay;
    cycleFrame %= m_160 + DAT_808C0F58;

    if (cycleFrame >= m_19c) {
        return 1.0f;
    }

    if (cycleFrame >= m_198) {
        return std::max(1.0f,
                m_168 - m_1a0 * static_cast<f32>(m_17c / 2) -
                        m_1a0 * static_cast<f32>(cycleFrame - m_198));
    }

    if (cycleFrame >= m_194) {
        s32 framesSince194 = cycleFrame - m_194;
        s32 half17c = m_17c / 2;
        if (framesSince194 > half17c) {
            return m_168 - m_1a0 * static_cast<f32>(framesSince194 - half17c);
        } else {
            return m_168;
        }
    }

    if (cycleFrame >= m_190 || cycleFrame >= m_18c) {
        return m_168;
    }

    if (cycleFrame >= 0) {
        return std::min(m_168, m_1a0 * static_cast<f32>(cycleFrame) + 1.0f);
    }

    return 1.0f;
}

/// @addr{0x8067FE88}
bool ObjectFlamePoleFoot::checkCollision(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);
    f32 scale = getScaleY(timeOffset);

    if (!m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut)) {
        return false;
    }

    if (2.0f < scale) {
        CollisionDirector::Instance()->setCurrentCollisionTrickable(true);
    }

    return true;
}

/// @addr{0x80680218}
bool ObjectFlamePoleFoot::checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    update(timeOffset);
    calcScale(timeOffset);
    f32 scale = getScaleY(timeOffset);

    if (!m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut)) {
        return false;
    }

    if (2.0f < scale) {
        CollisionDirector::Instance()->setCurrentCollisionTrickable(true);
    }

    return true;
}

/// @addr{0x8067F2F4}
void ObjectFlamePoleFoot::enterState1() {
    m_pole->setActive(true);
    m_pole->enableCollision();
    m_1b0 = 0.0f;

    f32 root0 = -1.0f;
    f32 root1 = -1.0f;
    f32 fVar1 = m_1a8;
    EGG::Mathf::FUN_800867C0(static_cast<f32>(m_174 * m_174),
            -4.0f * m_1a8 * static_cast<f32>(m_174), 4.0f * m_1a8 * m_1a8, root0, root1);

    if (root0 <= 0.0f) {
        root0 = -1.0f;
    }

    if (root1 <= 0.0f) {
        root1 = -1.0f;
        fVar1 = root0;
    }

    m_1c0 = fVar1;
    m_1bc = fVar1 * fVar1 / (2.0f * m_1a8);
}

/// @addr{0x8067F484}
void ObjectFlamePoleFoot::calcState1() {
    f32 frame = static_cast<f32>(m_cycleFrame - static_cast<u32>(m_18c));
    m_1b0 = std::min(m_1a8, m_1c0 * frame - frame * 0.5f * m_1bc * frame);
}

/// @addr{0x8067F544}
void ObjectFlamePoleFoot::calcState2() {
    m_1b0 = m_1b4 +
            50.0f *
                    EGG::Mathf::SinFIdx(
                            DEG2FIDX * (360.0f * static_cast<f32>(m_cycleFrame - m_190) / 30.0f));
}

u32 ObjectFlamePoleFoot::FLAMEPOLE_COUNT = 0;

} // namespace Field
