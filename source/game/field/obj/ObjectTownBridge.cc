#include "ObjectTownBridge.hh"

#include "game/field/obj/ObjectKCL.hh"

namespace Field {

/// @addr{0x80809448}
ObjectTownBridge::ObjectTownBridge(const System::MapdataGeoObj &params) : ObjectKCL(params) {
    m_bridgeRot = m_rot;
    m_rotateUpwards = m_rot.y < 0.0f;
    m_angVel = static_cast<float>(params.setting(0));
    m_pivotFrames = params.setting(1);
    m_raisedFrames = params.setting(2);
    m_120 = params.setting(3);
    m_fullAnimFrames = m_pivotFrames * 2 + (m_120 + m_raisedFrames);
    m_state = 0;
}

/// @addr{0x8080ACE0}
ObjectTownBridge::~ObjectTownBridge() = default;

/// @addr{0x80809774}
void ObjectTownBridge::calc() {}

/// @addr{0x808095B8}
void ObjectTownBridge::createCollision() {
    ObjectKCL::createCollision();

    const char *name = getKclName();

    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s2.kcl", name);

    auto *resMgr = System::ResourceManager::Instance();
    m_midColMgr = new ObjColMgr(resMgr->getFile(filepath, nullptr, System::ArchiveId::Course));

    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s3.kcl", name);

    auto *resMgr = System::ResourceManager::Instance();
    m_flatColMgr = new ObjColMgr(resMgr->getFile(filepath, nullptr, System::ArchiveId::Course));

    m_raisedColMgr = m_objColMgr;
}

} // namespace Field
