#include "KartObjectManager.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartParamFileManager.hh"

#include "game/system/RaceConfig.hh"

#include <abstract/g3d/ResFile.hh>

namespace Kinoko::Kart {

/// @addr{0x8058FFE8}
/// @brief Performs two passes of calculations for each kart object
void KartObjectManager::calc() {
    for (auto *&object : m_objects) {
        object->collide()->setTangentOff(EGG::Vector3f::zero);
        object->collide()->setMovement(EGG::Vector3f::zero);
    }

    for (auto *&object : m_objects) {
        object->calcSub();
        object->calc();
    }
}

/// @addr{0x8058FB2C}
/// @brief Private constructor that constructs the manager and kart objects for each player
/// @details Creates the @ref KartParamFileManager singleton instance and @ref KartObject instances
/// for each player. Also parses crush and shrink animation data from @p driver.brres.
KartObjectManager::KartObjectManager() {
    const auto &raceScenario = System::RaceConfig::Instance()->raceScenario();
    u8 count = raceScenario.playerCount;
    m_objects.reserve(count);
    KartParamFileManager::CreateInstance();

    loadScaleAnimations();

    for (size_t i = 0; i < count; ++i) {
        const auto &player = raceScenario.players[i];
        KartObject *object = KartObject::Create(player.character, player.vehicle, i);
        object->createModel();
        m_objects.push_back(object);
    }
}

/// @addr{0x8058FDD4}
/// @brief Private virtual destructor which destroys all @ref KartObject objects, the @ref
/// KartParamFileManager singleton instance, and the scale animation data
KartObjectManager::~KartObjectManager() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KartObjectManager instance not explicitly handled!");
    }

    KartParamFileManager::DestroyInstance();

    for (auto *&object : m_objects) {
        EGG::egg_delete(object);
    }

    EGG::egg_delete(s_thunderScaleUpAnmChr);
    EGG::egg_delete(s_thunderScaleDownAnmChr);
    EGG::egg_delete(s_pressScaleUpAnmChr);

    // If the proxy list is not cleared when we're done with the KartObjectManager, the list's
    // destructor calls delete on all of the links remaining in the list. Since the heaps are
    // gone by that point, this results in a segmentation fault. So, we clear the links here.
    KartObjectProxy::proxyList().clear();
}

/// @addr{0x8056AB6C}
/// @brief Parses the scale animation data from @p driver.brres pertaining to shrinking and crushing
void KartObjectManager::loadScaleAnimations() {
    auto *resMgr = System::ResourceManager::Instance();
    const void *file = resMgr->getFile("driver.brres", nullptr, System::ArchiveId::Core);
    ASSERT(file);

    // Copy construct onto the heap
    auto resAnmChr = Abstract::g3d::ResFile(file).resAnmChr("thunder_scale_up");
    s_thunderScaleUpAnmChr = EGG::egg_new<Abstract::g3d::ResAnmChr>(resAnmChr);
    resAnmChr = Abstract::g3d::ResFile(file).resAnmChr("thunder_scale_down");
    s_thunderScaleDownAnmChr = EGG::egg_new<Abstract::g3d::ResAnmChr>(resAnmChr);
    resAnmChr = Abstract::g3d::ResFile(file).resAnmChr("press_scale_up");
    s_pressScaleUpAnmChr = EGG::egg_new<Abstract::g3d::ResAnmChr>(resAnmChr);
}

Abstract::g3d::ResAnmChr *KartObjectManager::s_thunderScaleUpAnmChr = nullptr;
Abstract::g3d::ResAnmChr *KartObjectManager::s_thunderScaleDownAnmChr = nullptr;
Abstract::g3d::ResAnmChr *KartObjectManager::s_pressScaleUpAnmChr = nullptr;
KartObjectManager *KartObjectManager::s_instance = nullptr;

} // namespace Kinoko::Kart
