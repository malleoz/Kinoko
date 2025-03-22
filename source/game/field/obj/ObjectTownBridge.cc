#include "ObjectTownBridge.hh"

#include "game/field/obj/ObjectKCL.hh"

#include "game/system/RaceManager.hh"
#include "game/system/ResourceManager.hh"

#include <algorithm>

namespace Field {

/// @addr{0x80809448}
ObjectTownBridge::ObjectTownBridge(const System::MapdataGeoObj &params) : ObjectKCL(params) {
    m_bridgeRot = m_rot;
    m_rotateUpwards = m_rot.y < 0.0f;
    m_angVel = static_cast<float>(params.setting(0));
    m_pivotFrames = params.setting(1);
    m_raisedFrames = params.setting(2);
    m_loweredFrames = params.setting(3);
    m_fullAnimFrames = m_pivotFrames * 2 + (m_loweredFrames + m_raisedFrames);
    m_state = State::Raising;
}

/// @addr{0x8080ACE0}
ObjectTownBridge::~ObjectTownBridge() = default;

/// @addr{0x80809774}
void ObjectTownBridge::calc() {
    u32 t = System::RaceManager::Instance()->timer();
    s32 animFrame = t % m_fullAnimFrames;

    State state = calcState(animFrame);

    f32 dVar9;

    switch (state) {
    case State::Raised: {
        s32 rot = m_rotateUpwards ? -1 : 1;
        dVar9 = m_angVel * static_cast<f32>(rot);
    } break;
    case State::Lowered:
        dVar9 = 0.0f;
        break;
    case State::Raising: {
        s32 rot = m_rotateUpwards ? -1 : 1;
        dVar9 = m_angVel * static_cast<f32>((animFrame * rot)) / static_cast<f32>(m_pivotFrames);
    } break;
    case State::Lowering: {
        s32 rot = m_rotateUpwards ? -1 : 1;
        dVar9 = m_angVel *
                static_cast<f32>(
                        (rot * (m_pivotFrames - (animFrame - (m_pivotFrames + m_raisedFrames))))) /
                static_cast<f32>(m_pivotFrames);
    } break;
    default:
        dVar9 = 0.0f;
    }

    // Set the object collision based off the angle of the bridge.
    // The thresholds are >30 degrees for "raised", >10 degrees for "middle", otherwise "flat".
    f32 angle = std::clamp(dVar9, -45.0f, 45.0f);
    f32 absAng = EGG::Mathf::abs(angle);

    if (absAng <= 10.0f) {
        m_objColMgr = m_flatColMgr;
    } else {
        m_objColMgr = absAng <= 30.0f ? m_midColMgr : m_raisedColMgr;
    }

    m_flags |= 2;
    m_rot.z = angle * F_PI / 180.0f;
    m_state = state;
}

/// @addr{0x808095B8}
void ObjectTownBridge::createCollision() {
    ObjectKCL::createCollision();

    const char *name = getKclName();
    char filepath2[128];
    char filepath3[128];
    snprintf(filepath2, sizeof(filepath2), "%s2.kcl", name);
    snprintf(filepath3, sizeof(filepath3), "%s3.kcl", name);

    auto *resMgr = System::ResourceManager::Instance();
    m_midColMgr = new ObjColMgr(resMgr->getFile(filepath2, nullptr, System::ArchiveId::Course));
    m_flatColMgr = new ObjColMgr(resMgr->getFile(filepath3, nullptr, System::ArchiveId::Course));
    m_raisedColMgr = m_objColMgr;
}

/// @brief Helper function which determines the current state of the bridge based on t.
ObjectTownBridge::State ObjectTownBridge::calcState(s32 t) const {
    if (t < m_pivotFrames) {
        return State::Raising;
    }

    if (t < m_pivotFrames + m_raisedFrames) {
        return State::Raised;
    }

    if (t < m_pivotFrames * 2 + m_raisedFrames) {
        return State::Lowering;
    }

    return State::Lowered;
}

} // namespace Field
