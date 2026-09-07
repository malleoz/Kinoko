#include "ObjectKCL.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x8081A980}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectKCL::ObjectKCL(const System::MapdataGeoObj &params)
    : ObjectDrivable(params), m_lastMtxUpdateFrame(-2000), m_lastScaleUpdateFrame(-2000) {}

/// @addr{0x8067EAFC}
/// @brief Default virtual destructor that destroys the associated collision manager
ObjectKCL::~ObjectKCL() {
    EGG::egg_delete(m_objColMgr);
}

/// @addr{0x8081AA58}
/// @copybrief ObjectBase::createCollision()
/// @details Loads the KCL file for the object and creates an @ref ObjColMgr to interface with it
void ObjectKCL::createCollision() {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s.kcl", getKclName());

    auto *resMgr = System::ResourceManager::Instance();
    m_objColMgr =
            EGG::egg_new<ObjColMgr>(resMgr->getFile(filepath, System::ArchiveId::Course).data());
}

/// @addr{0x8081AB4C}
/// @details Saves the initial transformation matrix to the @ref ObjColMgr and computes the KCL's
/// bounding box midpoint and half-width.
void ObjectKCL::initCollision() {
    const EGG::Matrix34f &mat = getUpdatedMatrix(0);
    EGG::Matrix34f matInv;
    bool inverted = mat.ps_inverse(matInv);
    ASSERT(inverted);

    m_objColMgr->setMtx(mat);
    m_objColMgr->setInvMtx(matInv);
    m_objColMgr->setScale(getScaleY(0));

    EGG::Vector3f high = m_objColMgr->kclHighWorld();
    EGG::Vector3f low = m_objColMgr->kclLowWorld();

    m_kclMidpoint = (high + low).multInv(2.0f);

    EGG::Vector3f highLowDiffAbs = (high - low).abs();
    f32 maxDiff = std::max(highLowDiffAbs.x, highLowDiffAbs.z);
    m_bboxHalfSideLength = maxDiff * 0.5f;
}

/// @brief Advances the collision manager's transform to reflect the current frame
/// @addr{0x8081AD6C}
void ObjectKCL::update(u32 timeOffset) {
    u32 time = System::RaceManager::Instance()->timer() - timeOffset;
    if (m_lastMtxUpdateFrame == static_cast<s32>(time)) {
        return;
    }

    EGG::Matrix34f mat;

    if (timeOffset == 0) {
        calcTransform();
        mat = transform();
    } else {
        mat = getUpdatedMatrix(timeOffset);
    }

    EGG::Matrix34f matInv;
    mat.ps_inverse(matInv);
    m_objColMgr->setMtx(mat);
    m_objColMgr->setInvMtx(matInv);

    m_lastMtxUpdateFrame = time;
}

/// @brief Updates the collision manager's scale to reflect the current frame
/// @addr{0x8081AF28}
void ObjectKCL::calcScale(u32 timeOffset) {
    u32 time = System::RaceManager::Instance()->timer() - timeOffset;
    if (m_lastScaleUpdateFrame == static_cast<s32>(time)) {
        return;
    }

    if (time == 0) {
        m_objColMgr->setScale(scale().y);
    } else {
        m_objColMgr->setScale(getScaleY(timeOffset));
    }

    m_lastScaleUpdateFrame = time;
}

} // namespace Kinoko::Field
