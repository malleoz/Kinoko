#include "KartObject.hh"

#include "game/kart/KartMove.hh"
#include "game/kart/KartSub.hh"
#include "game/kart/KartSuspension.hh"
#include "game/kart/KartTire.hh"

#include "game/field/ObjectCollisionKart.hh"

#include "game/render/KartModel.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Kart {

/// @addr{0x8058DDBC}
/// @brief Constructs a KartObject with the provided KartParam pointer
/// @param param Pointer to the kart's parameter structure containing its configuration and stats
KartObject::KartObject(KartParam *param) {
    m_pointers.param = param;
}

/// @addr{0x8058DEF0}
/// @brief Virtual destructor that destroys the KartObject and all of its subsystems
KartObject::~KartObject() {
    EGG::egg_delete(m_pointers.param);
    EGG::egg_delete(m_pointers.body);
    EGG::egg_delete(m_pointers.sub);
    EGG::egg_delete(m_pointers.model);
    EGG::egg_delete(m_pointers.objectCollisionKart);

    for (auto *susp : m_pointers.suspensions) {
        EGG::egg_delete(susp);
    }

    for (auto *tire : m_pointers.tires) {
        EGG::egg_delete(tire);
    }
}

/// @addr{0x8058EA0C}
/// @details Creates 4 tire objects, except for @enum Vehicle::Blue_Falcon which only has 3 tires.
/// Adds the tire and suspension objects to the shared @ref KartAccessor. Finally, initializes the
/// tire suspensions.
void KartObject::createTires() {
    constexpr u16 BSP_WHEEL_INDICES[8] = {0, 0, 1, 1, 2, 2, 3, 3};
    constexpr KartSuspensionPhysics::TireType X_MIRRORED_TIRE[8] = {
            KartSuspensionPhysics::TireType::Kart,
            KartSuspensionPhysics::TireType::KartReflected,
            KartSuspensionPhysics::TireType::Kart,
            KartSuspensionPhysics::TireType::KartReflected,
            KartSuspensionPhysics::TireType::Kart,
            KartSuspensionPhysics::TireType::KartReflected,
            KartSuspensionPhysics::TireType::Kart,
            KartSuspensionPhysics::TireType::KartReflected,
    };

    auto bodyType = m_pointers.param->stats().body;
    u32 tireCount = m_pointers.param->tireCount();

    if (bodyType == KartParam::Stats::Body::Three_Wheel_Kart) {
        tireCount = 4;
    }

    for (u16 wheelIdx = 0, i = 0; i < tireCount; ++i) {
        if (bodyType == KartParam::Stats::Body::Three_Wheel_Kart && i == 0) {
            continue;
        }

        u16 bspWheelIdx = BSP_WHEEL_INDICES[i];
        KartSuspensionPhysics::TireType tireType = X_MIRRORED_TIRE[i];

        KartSuspension *sus = EGG::egg_new<KartSuspension>();
        KartTire *tire = (bspWheelIdx == 0) ? EGG::egg_new<KartTireFront>(tireType, bspWheelIdx) :
                                              EGG::egg_new<KartTire>(tireType, bspWheelIdx);

        m_pointers.suspensions.push_back(sus);
        m_pointers.tires.push_back(tire);

        sus->init(wheelIdx++, tireType, bspWheelIdx);
    }
}

/// @addr{0x8058E22C}
/// @brief Creates and initializes the kart's subsystems
/// @details Creates the associated @ref KartSub, @ref KartPhysics, and @ref KartBody objects.
/// Creates and initializes the tires and their suspensions. Finally, creates the associated @ref
/// Field::ObjectCollisionKart.
void KartObject::init() {
    prepareTiresAndSuspensions();
    createSub();
    auto *physics = KartPhysics::Create(*m_pointers.param);
    auto *body = createBody(physics);
    m_pointers.body = body;
    createTires();
    for (u16 tireIdx = 0; tireIdx < m_pointers.param->tireCount(); ++tireIdx) {
        m_pointers.tires[tireIdx]->init(tireIdx);
    }
    m_pointers.objectCollisionKart = EGG::egg_new<Field::ObjectCollisionKart>();
}

/// @addr{0x8058E188}
/// @brief Initializes the kart's collision data and bounding box
void KartObject::initCollision() {
    sub()->initAABB(m_pointers, this);
    sub()->init();
    objectCollisionKart()->init(param()->playerIdx());
}

/// @addr{0x8058EE48}
/// @brief Sets the initial position and rotation of the kart based off the current track.
void KartObject::initPhysics() {
    EGG::Vector3f euler_angles_deg, position;

    System::RaceManager::Instance()->findKartStartPoint(position, euler_angles_deg);
    move()->setInitialPhysicsValues(position, euler_angles_deg);
}

/// @addr{0x8058E804}
/// @brief Computes the number of wheels based off the BSP
void KartObject::prepareTiresAndSuspensions() {
    constexpr u16 LOCAL_20[4] = {2, 1, 1, 1};
    constexpr u16 LOCAL_28[4] = {2, 1, 1, 2};

    const BSP &rBsp = m_pointers.param->bsp();
    const KartParam::Stats::Body bodyWheels = m_pointers.param->stats().body;
    u16 wheelCount = 0;

    if (rBsp.wheels[0].enable != 0) {
        wheelCount += LOCAL_20[static_cast<u16>(bodyWheels)];
    }
    if (rBsp.wheels[1].enable != 0) {
        wheelCount += LOCAL_28[static_cast<u16>(bodyWheels)];
    }
    if (rBsp.wheels[2].enable != 0) {
        wheelCount += LOCAL_20[static_cast<u16>(bodyWheels)];
    }
    if (rBsp.wheels[3].enable != 0) {
        wheelCount += LOCAL_28[static_cast<u16>(bodyWheels)];
    }

    m_pointers.param->setTireCount(wheelCount);
    m_pointers.param->setSuspCount(wheelCount);

    m_pointers.tires.reserve(wheelCount);
    m_pointers.suspensions.reserve(wheelCount);
}

/// @addr{0x8058E724}
/// @brief Creates the @ref KartSub object and initializes its subsystems
void KartObject::createSub() {
    m_pointers.sub = EGG::egg_new<KartSub>();
    m_pointers.sub->createSubsystems(m_pointers.param->isBike(), m_pointers.param->stats());
}

/// @addr{0x8058F820}
/// @brief Creates the @ref Render::KartModel object
/// @details Since the static @ref KartObjectProxy pointer list is cleared before creating the
/// model, we have to call @ref KartObjectProxy::ApplyAll to share this object's pointers with the
/// model.
void KartObject::createModel() {
    s_proxyList.clear();

    if (isBike()) {
        m_pointers.model = EGG::egg_new<Render::KartModelBike>();
    } else {
        m_pointers.model = EGG::egg_new<Render::KartModelKart>();
    }

    ApplyAll(&m_pointers);

    m_pointers.model->init();
}

/// @addr{0x8058EEB4}
/// @brief Calls the first pass of the kart's subsystem calculations
void KartObject::calcSub() {
    sub()->calcPass0();
}

/// @addr{0x8058EEBC}
/// @brief Calls the second pass of the kart's subsystem calculations and the model's calculations
void KartObject::calc() {
    sub()->calcPass1();
    model()->calc();
}

/// @addr{0x8058F5B4}
/// @brief Creates a @ref KartObject based on the provided character and vehicle enums
/// @param character The character to create the kart for
/// @param vehicle The vehicle to create the kart for
/// @param playerIdx The player index to assign to the kart (always 0 in Kinoko)
/// @return A pointer to the created @ref KartObject
/// @details Creates a @ref KartParam object based on the provided character and vehicle enums. If
/// the vehicle is a kart, creates a @ref KartObject. If the vehicle is a bike, creates a @ref
/// KartObjectBike. Initializes the kart's subsystems and shares the subsystem pointers with all
/// subsystems. Finally, initializes the suspensions and tires.
KartObject *KartObject::Create(Character character, Vehicle vehicle, u8 playerIdx) {
    s_proxyList.clear();

    KartParam *param = EGG::egg_new<KartParam>(character, vehicle, playerIdx);

    KartObject *object = nullptr;
    if (vehicle < Vehicle::Standard_Bike_S) {
        object = EGG::egg_new<KartObject>(param);
    } else {
        object = EGG::egg_new<KartObjectBike>(param);
    }

    object->init();
    object->m_pointers.sub->copyPointers(object->m_pointers);

    // Applies a valid pointer to all of the proxies we create
    ApplyAll(&object->m_pointers);

    for (u16 i = 0; i < object->suspCount(); ++i) {
        object->suspension(i)->initPhysics();
    }

    for (u16 i = 0; i < object->tireCount(); ++i) {
        object->tire(i)->initBsp();
    }

    return object;
}

/// @addr{0x8058F20C}
/// @brief Constructs a KartObjectBike with the provided KartParam pointer
/// @param param Pointer to the kart's parameter structure containing its configuration and stats
KartObjectBike::KartObjectBike(KartParam *param) : KartObject(param) {}

/// @addr{0x8058F8B0}
/// @brief Default virtual destructor
KartObjectBike::~KartObjectBike() = default;

/// @addr{0x8058F2E8}
void KartObjectBike::createTires() {
    for (u16 wheelIdx = 0; wheelIdx < m_pointers.param->suspCount(); ++wheelIdx) {
        KartSuspension *sus = nullptr;
        KartTire *tire = nullptr;

        if (wheelIdx == 0 || wheelIdx == 2) {
            sus = EGG::egg_new<KartSuspensionFrontBike>();
            tire = EGG::egg_new<KartTireFrontBike>(KartSuspensionPhysics::TireType::Bike, 0);
        } else {
            sus = EGG::egg_new<KartSuspensionRearBike>();
            tire = EGG::egg_new<KartTireRearBike>(KartSuspensionPhysics::TireType::Bike, 1);
        }

        m_pointers.suspensions.push_back(sus);
        m_pointers.tires.push_back(tire);

        sus->init(wheelIdx, KartSuspensionPhysics::TireType::Bike, wheelIdx);
    }
}
} // namespace Kinoko::Kart
