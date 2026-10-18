#include "KartObjectManager.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartObject.hh"
#include "game/kart/KartParamFileManager.hh"

#include "game/system/RaceConfig.hh"
#include "game/system/ResourceManager.hh"

#include <abstract/g3d/ResFile.hh>

namespace Kinoko::Kart {

/// @addr{0x8058FEE0}
/// @brief Initializes each object's collision and physics subsystems
/// @details Simply calls @ref KartObject::initCollision() and @ref KartObject::initPhysics()
/// for each object.
void KartObjectManager::init() {
    for (auto *&object : m_objects) {
        object->initCollision();
        object->initPhysics();
    }
}

/// @addr{0x8058FFE8}
/// @brief Performs two passes of calculations for each kart object
/// @details Resets @ref KartCollide::m_tangentOff and @ref KartCollide::m_movement for all objects.
/// Then iterates all objects and calls @ref KartObject::calcSub() and @ref KartObject::calc().
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

/// @addr{0x80590100}
/// @brief Returns the @ref KartObject for the given player index
/// @param i The player index
/// @return A pointer to the @ref KartObject for the given player index
KartObject *KartObjectManager::object(size_t i) const {
    return m_objects[i];
}

/// @brief Gets the shock unshrink animation's data
/// @return Pointer to the shock unshrink animation's data, @ref s_thunderScaleUpAnmChr
const Abstract::g3d::ResAnmChr *KartObjectManager::ThunderScaleUpAnmChr() {
    return s_thunderScaleUpAnmChr;
}

/// @brief Gets the shock shrink animation's data
/// @return Pointer to the shock shrink animation's data, @ref s_thunderScaleDownAnmChr
const Abstract::g3d::ResAnmChr *KartObjectManager::ThunderScaleDownAnmChr() {
    return s_thunderScaleDownAnmChr;
}

/// @brief Gets the stomper uncrush animation's data
/// @return Pointer to the stomper uncrash animation's data, @ref s_pressScaleUpAnmChr
const Abstract::g3d::ResAnmChr *KartObjectManager::PressScaleUpAnmChr() {
    return s_pressScaleUpAnmChr;
}

/// @addr{0x8058FAA8}
/// @brief Creates the singleton instance of the @ref KartObjectManager
/// @return A pointer to the newly created singleton instance of the @ref KartObjectManager
/// @pre The singleton instance of the @ref KartObjectManager must not already exist.
KartObjectManager *KartObjectManager::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<KartObjectManager>();
    return s_instance;
}

/// @addr{0x8058FAF8}
/// @brief Destroys the singleton instance of the @ref KartObjectManager
/// @pre The singleton instance of the @ref KartObjectManager must already exist.
void KartObjectManager::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @addr{0x809C18F8}
/// @brief Returns the singleton instance of the @ref KartObjectManager
/// @return The singleton instance of the @ref KartObjectManager
KartObjectManager *KartObjectManager::Instance() {
    return s_instance;
}

/// @addr{0x8058FB2C}
/// @brief Private constructor that loads animation data and creates a kart object for each player
/// @details Creates the @ref KartParamFileManager singleton instance. Parses crush and shrink
/// animation data from `driver.brres`. For each player, creates a @ref KartObject instance and
/// invokes @ref KartObject::createModel() on the new instance.
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
/// @note If the proxy list is not cleared when we're done with the @ref KartObjectManager, the
/// list's destructor calls delete on all of the links remaining in the list. Since the heaps are
/// gone by that point, this results in a segmentation fault. So, we clear the @ref KartObjectProxy
/// links here.
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

    KartObjectProxy::ProxyList().clear();
}

/// @addr{0x8056AB6C}
/// @brief Parses the scale animation data from `driver.brres` pertaining to shrinking and crushing
/// @pre Requires that `Common.szs` has already been loaded via the @ref System::ResourceManager.
void KartObjectManager::loadScaleAnimations() {
    auto *resMgr = System::ResourceManager::Instance();
    std::span<const u8> file = resMgr->getFile("driver.brres", System::ArchiveId::Core);
    ASSERT(!file.empty());

    // Copy construct onto the heap
    auto resAnmChr = Abstract::g3d::ResFile(file.data()).resAnmChr("thunder_scale_up");
    s_thunderScaleUpAnmChr = EGG::egg_new<Abstract::g3d::ResAnmChr>(resAnmChr);
    resAnmChr = Abstract::g3d::ResFile(file.data()).resAnmChr("thunder_scale_down");
    s_thunderScaleDownAnmChr = EGG::egg_new<Abstract::g3d::ResAnmChr>(resAnmChr);
    resAnmChr = Abstract::g3d::ResFile(file.data()).resAnmChr("press_scale_up");
    s_pressScaleUpAnmChr = EGG::egg_new<Abstract::g3d::ResAnmChr>(resAnmChr);
}

Abstract::g3d::ResAnmChr *KartObjectManager::s_thunderScaleUpAnmChr = nullptr;
Abstract::g3d::ResAnmChr *KartObjectManager::s_thunderScaleDownAnmChr = nullptr;
Abstract::g3d::ResAnmChr *KartObjectManager::s_pressScaleUpAnmChr = nullptr;
KartObjectManager *KartObjectManager::s_instance = nullptr;

} // namespace Kinoko::Kart
