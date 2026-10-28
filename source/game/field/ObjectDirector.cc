#include "ObjectDirector.hh"

#include "game/field/BoxColManager.hh"
#include "game/field/ObjectCollisionConvexHull.hh"
#include "game/field/ObjectDrivableDirector.hh"
#include "game/field/obj/ObjectRegistry.hh"

#include "game/system/CourseMap.hh"
#include "game/system/RaceConfig.hh"

#include <algorithm>

namespace Kinoko::Field {

/// @addr{0x8082A2B4}
/// @brief Initializes all objects, including those in @ref ObjectDrivableDirector
/// @details Kinoko-specific: seagulls (@ref ObjectBird) only affect the simulation through their
/// floor collision checks, which can update the transforms of moving drivable objects. If there
/// are no drivable objects requiring per-frame calculations, the seagulls are removed from @ref
/// m_calcObjects. They are still initialized, since initialization consumes random numbers.
void ObjectDirector::init() {
    for (auto *&obj : m_objects) {
        obj->init();
        obj->calcModel();
    }

    ObjectDrivableDirector::Instance()->init();

    if (!m_calcObjects.empty() && !ObjectDrivableDirector::Instance()->hasCalcObjects()) {
        auto *newEnd = std::remove_if(m_calcObjects.begin(), m_calcObjects.end(),
                [](const ObjectBase *obj) { return obj->id() == ObjectId::Seagull; });

        while (m_calcObjects.end() != newEnd) {
            m_calcObjects.pop_back();
        }
    }
}

/// @addr{0x8082A8F4}
/// @brief Runs per-frame calculations for all objects, including those in @ref
/// ObjectDrivableDirector
void ObjectDirector::calc() {
    for (auto *&obj : m_calcObjects) {
        obj->calc();
    }

    for (auto *&obj : m_calcObjects) {
        obj->calcModel();
    }

    ObjectDrivableDirector::Instance()->calc();
}

/// @addr{0x8082B0E8}
/// @brief Registers an object to the director, and adds it to the appropriate lists based on its
/// load flags and collision mode
/// @param obj The @ref ObjectCollidable to add to the director
/// @details If the object's load flags have @ref ObjectBase::eLoadFlags::Calc set, then adds `obj`
/// to @ref m_calcObjects. If the object has a valid collision mode, then adds `obj` to @ref
/// m_collisionObjects. Finally, adds `obj` to @ref m_objects.
/// @bug In the base game, @ref ObjectHighwayManager will erroneously access slot -1 in the flow
/// table; the arbitrary read in the base game produces a `mode` of zero, so the object is not added
/// to @ref m_collisionObjects. To match the base game behavior here, we add a `nullptr` check
/// instead.
void ObjectDirector::addObject(ObjectCollidable *obj) {
    if (obj->loadFlags().onBit(ObjectBase::eLoadFlags::Calc)) {
        m_calcObjects.push_back(obj);
    }

    const auto *set = m_flowTable.set(m_flowTable.slot(obj->id()));
    if (set && set->mode != 0) {
        if (obj->collision()) {
            m_collisionObjects.push_back(obj);
        }
    }

    m_objects.push_back(obj);
}

// @brief Adds an object to the director without adding it to the list of objects that require
/// per-frame calculations or collision checks
/// @param obj The object to add to the director without adding it to @ref calcObjects
void ObjectDirector::addObjectNoImpl(ObjectBase *obj) {
    m_objects.push_back(obj);
}

/// @addr{0x806C4ED4}
/// @brief Adds an object to the list of objects that are meant to be accessed directly by other
/// @param obj The @ref ObjectCollidable to add to the list of managed objects
void ObjectDirector::addManagedObject(ObjectCollidable *obj) {
    m_managedObjects.push_back(obj);
}

/// @addr{0x8082AB04}
/// @brief Performs collision checks between a kart and all objects having a tangible @ref
/// BoxColUnit
/// @param kartObj The kart to check for collisions against
/// @param convexHull The convex hull representing the kart's collision shape
/// @return The number of collisions detected
/// @pre Requires that @ref BoxColManager::search() has been called prior to invoking this function.
/// @details Iterates the list of nearby objects via @ref BoxColManager::getNextObject(). Skips
/// objects which do not have a GJK collision object (@ref ObjectCollisionBase). Updates the
/// object's GJK collision transform and dispatches to @ref ObjectCollidable::checkCollision() to
/// perform the GJK collision check.
///
/// When a collision occurs, fetches the reaction to apply on the kart and processes it. Depending
/// on the behavior of the colliding object, the reaction may be modified. Caches the modified
/// reaction so it can be fetched later.
size_t ObjectDirector::checkKartObjectCollision(Kart::KartObject *kartObj,
        ObjectCollisionConvexHull *convexHull) {
    size_t count = 0;

    while (ObjectCollidable *obj = BoxColManager::Instance()->getNextObject()) {
        auto *objCollision = obj->collision();
        if (!objCollision) {
            continue;
        }

        obj->calcCollisionTransform();
        if (!obj->checkCollision(convexHull, m_hitDepths[count])) {
            continue;
        }

        // We have a collision, process it
        // Assume that we are not in a star, mega, or bullet
        Kart::Reaction reactionOnKart = m_hitTableKart.reaction(m_hitTableKart.slot(obj->id()));
        Kart::Reaction reactionOnObj =
                m_hitTableKartObject.reaction(m_hitTableKartObject.slot(obj->id()));

        // The object might change the reaction states
        obj->processKartReactions(kartObj, reactionOnKart, reactionOnObj);

        Kart::Reaction reaction =
                obj->onCollision(kartObj, reactionOnKart, reactionOnObj, m_hitDepths[count]);
        m_reactions[count] = reaction;

        // We omit the call to @ref ObjectCollidable::onWallCollision() and @ref
        // ObjectCollidable::onObjectCollision() since they are unused in Kinoko

        m_collidingObjects[count] = obj;
        if (m_hitDepths[count].y < 0.0f) {
            m_hitDepths[count].y = 0.0f;
        }

        ++count;
    }

    return count;
}

/// @brief Gets the @ref ObjectFlowTable for the objects
/// @return Const ref to the @ref ObjectFlowTable for the objects, @ref m_flowTable
[[nodiscard]] const ObjectFlowTable &ObjectDirector::flowTable() const {
    return m_flowTable;
}

/// @brief Gets the @ref ObjectHitTable for the kart objects
/// @return Const ref to the @ref ObjectHitTable for the kart objects, @ref m_hitTableKart
[[nodiscard]] const ObjectHitTable &ObjectDirector::hitTableKart() const {
    return m_hitTableKart;
}

/// @brief Gets the object at the specified collision index
/// @param idx The index of the collision to retrieve the object for
/// @return Const pointer to the @ref ObjectCollidable at the specified collision index in @ref
/// m_collidingObjects
/// @pre Assumes that @ref checkKartObjectCollision() has been called and returned a number of
/// collisions greater than the specified index
[[nodiscard]] const ObjectCollidable *ObjectDirector::collidingObject(size_t idx) const {
    ASSERT(idx < m_collidingObjects.size());
    return m_collidingObjects[idx];
}

/// @brief Gets the reaction at the specified collision index
/// @param idx The index of the collision to retrieve the reaction for
/// @return The @ref Kart::Reaction at the specified collision index in @ref m_reactions
/// @pre Assumes that @ref checkKartObjectCollision() has been called and returned a number of
/// collisions greater than the specified index
[[nodiscard]] Kart::Reaction ObjectDirector::reaction(size_t idx) const {
    ASSERT(idx < m_reactions.size());
    return m_reactions[idx];
}

/// @brief Gets the hit depth at the specified collision index
/// @param idx The index of the collision to retrieve the hit depth for
/// @return Const reference to the hit depth vector at the specified collision index in @ref
/// m_hitDepths
/// @pre Assumes that @ref checkKartObjectCollision() has been called and returned a number of
/// collisions greater than the specified index
[[nodiscard]] const EGG::Vector3f &ObjectDirector::hitDepth(size_t idx) const {
    ASSERT(idx < m_hitDepths.size());
    return m_hitDepths[idx];
}

/// @brief Exposes a span of the managed objects' pointers
/// @return A span of the managed objects' pointers from @ref m_managedObjects
[[nodiscard]] std::span<ObjectCollidable *const> ObjectDirector::managedObjects() {
    return std::span<ObjectCollidable *>(m_managedObjects.begin(), m_managedObjects.size());
}

/// @brief Exposes the rising water object, so that @ref Kart::KartMove::calcRisingWater can
/// know if it should run or not
/// @return A pointer to the rising water object, @ref m_psea
ObjectPsea *ObjectDirector::psea() const {
    return m_psea;
}

/// @brief Sets the @ref ObjectPsea instance to expose in the director
/// @param psea A pointer to the @ref ObjectPsea instance to set
void ObjectDirector::setPsea(ObjectPsea *psea) {
    m_psea = psea;
}

/// @addr{0x8082B3EC}
/// @brief Gets the vertical distance between the kart and the rising water object
/// @return The vertical distance between the kart and the @ref ObjectPsea
/// @pre Requires that @ref m_psea was previously set via @ref setPsea().
f32 ObjectDirector::distAboveRisingWater(f32 offset) const {
    ASSERT(m_psea);
    return offset - m_psea->pos().y;
}

/// @addr{0x8082B400}
/// @brief Returns the height of the kill plane for the rising water object, if present
/// @return The height of the kill plane for @ref ObjectPsea
/// @pre Requires that @ref m_psea was previously set via @ref setPsea().
/// @details The kill plane height is a fixed `260.0f` units below the height of the @ref
/// ObjectPsea object.
f32 ObjectDirector::risingWaterKillPlaneHeight() const {
    ASSERT(m_psea);
    return m_psea->pos().y - 260.0f;
}

///< @addr{0x808C70E8}
/// @brief Gets the maximum pitch angle of @ref ObjectWanwan based on the current stage
/// @return The maximum pitch angle of @ref ObjectWanwan
/// @see s_wanwanMaxPitch
f32 ObjectDirector::WanwanMaxPitch() {
    return s_wanwanMaxPitch;
}

/// @addr{0x8082A784}
/// @brief Creates the singleton instances of @ref ObjectDirector and @ref
/// ObjectDrivableDirector. Also creates all objects in the course.
/// @return A pointer to the newly created singleton instance of @ref ObjectDirector
ObjectDirector *ObjectDirector::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<ObjectDirector>();
    ObjectDrivableDirector::CreateInstance();
    s_instance->createObjects();
    return s_instance;
}

/// @addr{0x8082A824}
/// @brief Destroys the singleton instances of @ref ObjectDirector and @ref
/// ObjectDrivableDirector
void ObjectDirector::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
    ObjectDrivableDirector::DestroyInstance();
}

/// @brief Returns the singleton instance of the @ref ObjectDirector
/// @return A pointer to the singleton instance of the @ref ObjectDirector
ObjectDirector *ObjectDirector::Instance() {
    return s_instance;
}

/// @addr{0x8082A38C}
/// @brief Private constructor
/// @details Constructs @ref m_flowTable from `ObjFlow.bin`, @ref m_hitTableKart from
/// `GeoHitTableKart.bin`, and @ref m_hitTableKartObject from `GeoHitTableKartObj.bin`. Initializes
/// other member variables to their default states. Initializes @ref m_objects, @ref m_calcObjects,
/// and @ref m_collisionObjects to have a maximum size of @ref MAX_UNIT_COUNT. Initializes @ref
/// m_managedObjects to have a maximum size of @ref MAX_MANAGED_OBJECTS. Finally, initializes @ref
/// m_psea to `nullptr`.
ObjectDirector::ObjectDirector()
    : m_flowTable("ObjFlow.bin"),
      m_hitTableKart("GeoHitTableKart.bin"),
      m_hitTableKartObject("GeoHitTableKartObj.bin"),
      m_objects(MAX_UNIT_COUNT),
      m_calcObjects(MAX_UNIT_COUNT),
      m_collisionObjects(MAX_UNIT_COUNT),
      m_psea(nullptr),
      m_managedObjects(MAX_MANAGED_OBJECTS) {}

/// @addr{0x8082A694}
/// @brief Private virtual destructor that deletes all objects in the director
ObjectDirector::~ObjectDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("ObjectDirector instance not explicitly handled!");
    }

    for (auto *&obj : m_objects) {
        EGG::egg_delete(obj);
    }
}

/// @addr{0x80826E8C}
/// @brief Constructs all objects defined in `course.kmp`
/// @details After construction, calls @ref ObjectBase::load() on each object, which will load
/// resources, create collision, and register the object to the appropriate director. Also
/// constructs special manager classes depending on what course and objects are present.
void ObjectDirector::createObjects() {
    const auto *courseMap = System::CourseMap::Instance();
    size_t objectCount = courseMap->getGeoObjCount();

    auto *objDrivableDir = ObjectDrivableDirector::Instance();
    auto course = System::RaceConfig::Instance()->raceScenario().course;
    bool rGV2 = course == Course::SNES_Ghost_Valley_2;
    bool sun = false;

    // The max pitch of the Chain Chomp only needs to be set once,
    // so we set it here instead of in the Chain Chomp constructor.
    s_wanwanMaxPitch = course == Course::GCN_Mario_Circuit ? -20.0f : -30.0f;

    for (size_t i = 0; i < objectCount; ++i) {
        const auto *pObj = courseMap->getGeoObj(i);
        ASSERT(pObj);

        // Assume one player - if the presence flag isn't set, don't construct it
        if (!(pObj->presenceFlag() & 1)) {
            continue;
        }

        // Prevent construction of objects with disabled or no collision
        if (IsObjectBlacklisted(pObj->id())) {
            continue;
        }

        // rGV2's blocks are created outside of the factory function
        if (rGV2) {
            switch (static_cast<ObjectId>(pObj->id())) {
            case ObjectId::ObakeBlockSFCc:
            case ObjectId::ObakeBlock2SFCc:
            case ObjectId::ObakeBlock3SFCc: {
                // Create the manager if this is the first block.
                auto *obakeManager = objDrivableDir->obakeManager();
                if (!obakeManager) {
                    objDrivableDir->createObakeManager(*pObj);
                } else {
                    obakeManager->addBlock(*pObj);
                }
            } break;
            default:
                break;
            }
        } else {
            ObjectBase *object = createObject(*pObj);
            object->load();
        }

        if (static_cast<ObjectId>(pObj->id()) == ObjectId::SunDS) {
            sun = true;
        }
    }

    if (course == Course::Moonview_Highway) {
        auto *highwayMgr = EGG::egg_new<ObjectHighwayManager>();
        highwayMgr->load();
    }

    if (sun) {
        auto *sunMgr = EGG::egg_new<ObjectSunManager>();
        sunMgr->load();
    }

    if (course == Course::GBA_Shy_Guy_Beach) {
        auto *shipMgr = EGG::egg_new<ObjectHeyhoShipManager>();
        shipMgr->load();
    }
}

/// @addr{0x80821E14}
/// @brief Factory function that constructs an object based on its ID
/// @param params The parameters used to initialize the object
/// @return A pointer to the newly constructed object
/// @details Some objects are primitive types that do not require a subclass, and some are not
/// implemented as they do not affect time trial physics.
ObjectBase *ObjectDirector::createObject(const System::MapdataGeoObj &params) {
    ObjectId id = static_cast<ObjectId>(params.id());
    switch (id) {
    case ObjectId::Psea:
        return EGG::egg_new<ObjectPsea>(params);
    case ObjectId::Woodbox:
        return EGG::egg_new<ObjectWoodbox>(params);
    case ObjectId::WLWallGC:
        return EGG::egg_new<ObjectWLWallGC>(params);
    case ObjectId::CarA1:
    case ObjectId::CarA2:
    case ObjectId::CarA3:
        return EGG::egg_new<ObjectCarA>(params);
    case ObjectId::Basabasa:
        return EGG::egg_new<ObjectBasabasa>(params);
    case ObjectId::HeyhoShipGBA:
        return EGG::egg_new<ObjectHeyhoShip>(params);
    case ObjectId::KartTruck:
    case ObjectId::CarBody:
        return EGG::egg_new<ObjectCarTGE>(params);
    case ObjectId::KoopaBall:
        return EGG::egg_new<ObjectKoopaBall>(params);
    case ObjectId::W_Woodbox:
        return EGG::egg_new<ObjectWoodboxW>(params);
    case ObjectId::SunDS:
        return EGG::egg_new<ObjectSunDS>(params);
    case ObjectId::ItemboxLine:
        return EGG::egg_new<ObjectItemboxLine>(params);
    case ObjectId::VolcanoBall:
        return EGG::egg_new<ObjectVolcanoBallLauncher>(params);
    case ObjectId::PenguinS:
        return EGG::egg_new<ObjectPenguinS>(params);
    case ObjectId::PenguinM:
        return EGG::egg_new<ObjectPenguin>(params);
    case ObjectId::Dossunc:
        return EGG::egg_new<ObjectDossunc>(params);
    case ObjectId::Boble:
        return EGG::egg_new<ObjectBoble>(params);
    case ObjectId::Hanachan:
        return EGG::egg_new<ObjectHanachan>(params);
    case ObjectId::Seagull:
        return EGG::egg_new<ObjectBird>(params);
    case ObjectId::Crab:
        return EGG::egg_new<ObjectCrab>(params);
    case ObjectId::Hwanwan:
        return EGG::egg_new<ObjectHwanwanManager>(params);
    case ObjectId::HeyhoBallGBA:
        return EGG::egg_new<ObjectHeyhoBall>(params);
    case ObjectId::DokanSFC:
        return EGG::egg_new<ObjectDokan>(params);
    case ObjectId::Pylon:
        return EGG::egg_new<ObjectPylon>(params);
    case ObjectId::OilSFC:
        return EGG::egg_new<ObjectOilSFC>(params);
    case ObjectId::ParasolR:
        return EGG::egg_new<ObjectParasolR>(params);
    case ObjectId::KoopaFigure64:
        return EGG::egg_new<ObjectKoopaFigure64>(params);
    case ObjectId::Kuribo:
        return EGG::egg_new<ObjectKuribo>(params);
    case ObjectId::Choropu:
    case ObjectId::Choropu2:
        return EGG::egg_new<ObjectChoropu>(params);
    case ObjectId::Cow:
        return EGG::egg_new<ObjectCowHerd>(params);
    case ObjectId::PakkunF:
        return EGG::egg_new<ObjectPakkunF>(params);
    case ObjectId::WLFirebarGC:
    case ObjectId::KoopaFirebar:
        return EGG::egg_new<ObjectFirebar>(params);
    case ObjectId::Wanwan:
        return EGG::egg_new<ObjectWanwan>(params);
    case ObjectId::Poihana:
        return EGG::egg_new<ObjectPoihana>(params);
    case ObjectId::Propeller:
        return EGG::egg_new<ObjectPropeller>(params);
    case ObjectId::DKRockGC:
        return EGG::egg_new<ObjectRock>(params);
    case ObjectId::Sanbo:
        return EGG::egg_new<ObjectSanbo>(params);
    case ObjectId::TruckWagon:
        return EGG::egg_new<ObjectTruckWagon>(params);
    case ObjectId::Heyho:
        return EGG::egg_new<ObjectHeyho>(params);
    case ObjectId::Press:
        return EGG::egg_new<ObjectPress>(params);
    case ObjectId::WLFireRingGC:
        return EGG::egg_new<ObjectFireRing>(params);
    case ObjectId::FireSnake:
        return EGG::egg_new<ObjectFireSnake>(params);
    case ObjectId::FireSnakeV:
        return EGG::egg_new<ObjectFireSnakeV>(params);
    case ObjectId::PuchiPakkun:
        return EGG::egg_new<ObjectPuchiPakkun>(params);
    case ObjectId::KinokoUd:
        return EGG::egg_new<ObjectKinokoUd>(params);
    case ObjectId::KinokoBend:
        return EGG::egg_new<ObjectKinokoBend>(params);
    case ObjectId::VolcanoRock:
        return EGG::egg_new<ObjectVolcanoRock>(params);
    case ObjectId::BulldozerL:
    case ObjectId::BulldozerR:
        return EGG::egg_new<ObjectBulldozer>(params);
    case ObjectId::KinokoNm:
        return EGG::egg_new<ObjectKinokoNm>(params);
    case ObjectId::Crane:
        return EGG::egg_new<ObjectCrane>(params);
    case ObjectId::VolcanoPiece:
        return EGG::egg_new<ObjectVolcanoPiece>(params);
    case ObjectId::FlamePole:
        return EGG::egg_new<ObjectFlamePoleFoot>(params);
    case ObjectId::TwistedWay:
        return EGG::egg_new<ObjectTwistedWay>(params);
    case ObjectId::TownBridge:
        return EGG::egg_new<ObjectTownBridge>(params);
    case ObjectId::DKShip64:
        return EGG::egg_new<ObjectShip64>(params);
    case ObjectId::Turibashi:
        return EGG::egg_new<ObjectTuribashi>(params);
    case ObjectId::Aurora:
        return EGG::egg_new<ObjectAurora>(params);
    case ObjectId::DCPillar:
        return EGG::egg_new<ObjectPillar>(params);
    case ObjectId::Sandcone:
        return EGG::egg_new<ObjectSandcone>(params);
    case ObjectId::FlamePoleV:
    case ObjectId::FlamePoleVBig:
        return EGG::egg_new<ObjectFlamePoleV>(params);
    case ObjectId::Ami:
        return EGG::egg_new<ObjectAmi>(params);
    case ObjectId::BeltEasy:
        return EGG::egg_new<ObjectBeltEasy>(params);
    case ObjectId::BeltCrossing:
        return EGG::egg_new<ObjectBeltCrossing>(params);
    case ObjectId::BeltCurveA:
        return EGG::egg_new<ObjectBeltCurveA>(params);
    case ObjectId::Escalator:
        return EGG::egg_new<ObjectEscalator>(params);
    case ObjectId::EscalatorGroup:
        return EGG::egg_new<ObjectEscalatorGroup>(params);

    // Non-specified objects are stock collidable objects by default
    // However, we need to specify an impl, so we don't use default
    case ObjectId::DummyPole:
    case ObjectId::CastleTree1c:
    case ObjectId::MarioTreeGCc:
    case ObjectId::PeachTreeGCc:
    case ObjectId::MarioGo64c:
    case ObjectId::KinokoT1:
    case ObjectId::PalmTree:
    case ObjectId::Parasol:
    case ObjectId::HeyhoTreeGBAc:
    case ObjectId::GardenTreeDSc:
    case ObjectId::DKtreeA64c:
    case ObjectId::DKTreeB64c:
    case ObjectId::TownTreeDsc:
    case ObjectId::PakkunDokan:
        return EGG::egg_new<ObjectCollidable>(params);
    case ObjectId::WLDokanGC:
    case ObjectId::Mdush:
        return EGG::egg_new<ObjectKCL>(params);
    default:
        return EGG::egg_new<ObjectNoImpl>(params);
    }
}

f32 ObjectDirector::s_wanwanMaxPitch;

ObjectDirector *ObjectDirector::s_instance = nullptr;

} // namespace Kinoko::Field
