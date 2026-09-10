#include "ObjectCarTGE.hh"

#include "game/field/ObjectCollisionCylinder.hh"
#include "game/field/ObjectDirector.hh"
#include "game/field/RailManager.hh"
#include "game/field/obj/ObjectHighwayManager.hh"

#include "game/system/RaceConfig.hh"

namespace Kinoko::Field {

/// @addr{0x806D5EE4}
/// @brief Constructor
/// @param params The parameters used to initialize the object
/// @details Pre-computes all floor normals along the rail (this is likely unnecessary for Kinoko).
/// Determines the @ref m_carType based off the object name and @ref m_mdlName based off the
/// resource name and car variant. Also determines the @ref m_dummyId based off the car type.
/// Finally, registers the object with the highway manager if the course is Moonview Highway, so
/// that the highway manager can enforce squish cooldowns for the player.
ObjectCarTGE::ObjectCarTGE(const System::MapdataGeoObj &params)
    : ObjectCollidable(params),
      StateManager(this, STATE_ENTRIES),
      m_auxCollision(nullptr),
      m_highwayVel(static_cast<f32>(params.setting(2))),
      m_localVel(static_cast<f32>(params.setting(1))),
      m_carName{},
      m_mdlName{},
      m_carType(CarType::Normal),
      m_dummyId(ObjectId::None),
      m_scaledTangentDir(EGG::Vector3f::zero),
      m_currSpeed(0.0f),
      m_up(EGG::Vector3f::zero),
      m_tangent(EGG::Vector3f::zero) {
    u32 carVariant = static_cast<u32>(params.setting(3));
    s16 pathId = params.pathId();

    // The base game returns before the StateManager sets its state entries.
    // Since we handle this in the template specialization's constructor in Kinoko,
    // we need to reset the StateManager parameters back to their default values before returning.
    if (pathId == -1) {
        m_nextStateId = -1;
        m_currentStateId = 0;
        m_currentFrame = 0;

        return;
    }

    // The base game preemptively calculates collision for all points along the rail.
    RailManager::Instance()->rail(pathId)->checkSphereFull();

    const auto *name = getName();

    if (strcmp(name, "car_body") == 0) {
        snprintf(m_carName, sizeof(m_carName), "%s", "K_car_body");
        m_carType = CarType::Normal;
    } else if (strcmp(name, "kart_truck") == 0) {
        snprintf(m_carName, sizeof(m_carName), "%s", "K_truck");
        m_carType = CarType::Truck;
    } else if (strcmp(name, "K_bomb_car") == 0) {
        PANIC("Bomb cars are not implemented!");
    }

    const auto *resourceName = getResources();

    switch (carVariant) {
    case 0: {
        if (strcmp(resourceName, "K_truck") == 0) {
            snprintf(m_mdlName, sizeof(m_mdlName), "%s_b", "K_truck");
        } else if (strcmp(resourceName, "K_car_body") == 0) {
            snprintf(m_mdlName, sizeof(m_mdlName), "%s_b", "K_car");
        }
    } break;
    case 1: {
        if (strcmp(resourceName, "K_truck") == 0) {
            snprintf(m_mdlName, sizeof(m_mdlName), "%s_o", "K_truck");
        } else if (strcmp(resourceName, "K_car_body") == 0) {
            snprintf(m_mdlName, sizeof(m_mdlName), "%s_r", "K_car");
        }
    } break;
    case 2: {
        if (strcmp(resourceName, "K_truck") == 0) {
            snprintf(m_mdlName, sizeof(m_mdlName), "%s_g", "K_truck");
        } else if (strcmp(resourceName, "K_car_body") == 0) {
            snprintf(m_mdlName, sizeof(m_mdlName), "%s_y", "K_car");
        }
    } break;
    default: {
        PANIC("Bomb cars are not implemented!");
        break;
    }
    }

    switch (m_carType) {
    case CarType::Normal:
        m_dummyId = ObjectDirector::Instance()->flowTable().getIdFromName("car_body_dummy");
        break;
    case CarType::Truck:
        m_dummyId = ObjectDirector::Instance()->flowTable().getIdFromName("kart_truck_dummy");
        break;
    default:
        PANIC("Bomb cars are not implemented!");
        break;
    }

    if (System::RaceConfig::Instance()->raceScenario().course == Course::Moonview_Highway) {
        registerManagedObject();
    }
}

/// @addr{0x806D6B14}
/// @copybrief ObjectBase::init()
/// @details Sets the rail interpolator's velocity and @ref m_nextStateId based on whether the car
/// is starting on the highway or not. Sets the car's position to the current position of the rail
/// interpolator. Also sets @ref m_hitAngle based on the car type.
void ObjectCarTGE::init() {
    constexpr f32 HIT_ANGLE_TRUCK = 20.0f;
    constexpr f32 HIT_ANGLE_NORMAL = 40.0f;

    ASSERT(m_mapObj);
    if (m_mapObj->pathId() == -1) {
        return;
    }

    u16 idx = m_mapObj->setting(0);
    m_railInterpolator->init(0.0f, idx);

    auto *rail = RailManager::Instance()->rail(m_mapObj->pathId());
    u16 speedSetting = rail->points()[idx].setting[1];
    if (speedSetting == 1) {
        m_railInterpolator->setCurrVel(m_highwayVel);
    } else if (speedSetting == 0) {
        m_railInterpolator->setCurrVel(m_localVel);
    }

    calcStateFromRailPointSetting();

    m_squashed = false;
    setPos(m_railInterpolator->curPos());
    m_currSpeed = m_railInterpolator->speed();
    m_scaledTangentDir = m_railInterpolator->curTangentDir() * m_currSpeed;
    m_hitAngle = (m_carType == CarType::Truck) ? HIT_ANGLE_TRUCK : HIT_ANGLE_NORMAL;
}

/// @addr{0x806D7AF8}
/// @copybrief ObjectBase::createCollision()
/// @details Creates two collision objects, the second being a cylinder scaled based off vehicle
/// type.
void ObjectCarTGE::createCollision() {
    constexpr f32 TRUCK_RADIUS = 190.0f;
    constexpr f32 TRUCK_HEIGHT = 500.0f;
    constexpr f32 NORMAL_RADIUS = 150.0f;
    constexpr f32 NORMAL_HEIGHT = 200.0f;

    ASSERT(m_carType == CarType::Truck || m_carType == CarType::Normal);

    bool isTruck = (m_carType == CarType::Truck);

    ObjectCollidable::createCollision();

    f32 radius = isTruck ? TRUCK_RADIUS : NORMAL_RADIUS;
    f32 height = isTruck ? TRUCK_HEIGHT : NORMAL_HEIGHT;

    m_auxCollision = EGG::egg_new<ObjectCollisionCylinder>(radius, height, collisionCenter());
}

/// @addr{0x806D7BF8}
/// @copybrief ObjectBase::calcCollisionTransform()
/// @details Refreshes the collision transform to reflect the car's updated position and rotation.
/// Refreshes the auxiliary transform to reflect the same updates, but flattens the car's forward
/// direction to world up, thereby removing any pitch or tilt.
void ObjectCarTGE::calcCollisionTransform() {
    auto *col = collision();

    if (!col) {
        return;
    }

    calcTransform();
    col->transform(transform(), scale(), m_scaledTangentDir);
    calcTransform();
    EGG::Matrix34f mat;
    SetRotTangentHorizontal(mat, transform().base(2), EGG::Vector3f::ey);
    calcTransform();
    mat.setBase(3, transform().base(3));
    m_auxCollision->transform(mat, scale(), m_scaledTangentDir);
}

/// @addr{0x806D7328}
/// @brief Causes the player to be hit, squished, or bounce depending on the collision scenario
/// @param kartObj The kart object that collided with this car
/// @param reactionOnKart The reaction that should be applied to the kart
/// @param hitDepth The depth of the collision between the kart and the car
/// @details Interfaces with the @ref ObjectHighwayManager to enforce a squish cooldown for the
/// player.
Kart::Reaction ObjectCarTGE::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f &hitDepth) {
    constexpr u32 SQUASH_INVULNERABILITY = 200;

    if (!m_hasAuxCollision) {
        EGG::Vector3f hitDepthNorm = hitDepth;
        hitDepthNorm.normalise2();

        if (hitDepthNorm.y > 0.9f) {
            return Kart::Reaction::UntrickableJumpPad;
        }
    }

    if (m_highwayMgr && m_highwayMgr->squashTimer() < SQUASH_INVULNERABILITY) {
        const auto &hitTable = ObjectDirector::Instance()->hitTableKart();
        reactionOnKart = hitTable.reaction(hitTable.slot(static_cast<ObjectId>(m_dummyId)));
    }

    // In the base game, behavior branches on reactionOnObj, but for time trials it's always 0.
    if (reactionOnKart != Kart::Reaction::None && reactionOnKart != Kart::Reaction::Wall) {
        m_squashed = true;
        calcTransform();
        EGG::Vector3f v2 = transform().base(2);
        v2.y = 0.0f;
        v2.normalise2();
        EGG::Vector3f posDelta = kartObj->pos() - pos();
        posDelta.y = 0.0f;
        posDelta.normalise2();

        if (v2.dot(posDelta) < EGG::Mathf::CosFIdx(0.7111111f * m_hitAngle) && !m_hasAuxCollision) {
            reactionOnKart = Kart::Reaction::Sideways;
        }

        hitDepth.setZero();
    }

    return reactionOnKart;
}

/// @addr{0x806D9000}
/// @brief Calculates the position and orientation of the car along the rail
/// @details Computes position and orientation by computing a cubic bezier along the rail to find
/// the position of the rear of the car.
void ObjectCarTGE::calcPos() {
    constexpr f32 NORMAL_SPEED = 1500.0f;
    constexpr f32 TRUCK_SPEED = 1600.0f;

    f32 speed = (m_carType == CarType::Truck) ? TRUCK_SPEED : NORMAL_SPEED;
    f32 t = speed * scale().z * 0.5f;

    EGG::Vector3f rearPos;
    EGG::Vector3f rearTangentDir;
    m_railInterpolator->evalPositionAndTangentBehind(t, rearPos, rearTangentDir);

    const EGG::Vector3f curPos = m_railInterpolator->curPos();
    EGG::Vector3f posDelta = curPos - rearPos;
    posDelta.normalise2();
    m_tangent += 0.1f * (posDelta - m_tangent);
    m_tangent.y = posDelta.y;
    m_tangent.normalise2();

    if (m_tangent.y > -0.05f) {
        setPos(curPos - m_tangent * t * 0.5f);
    } else {
        setPos(curPos - m_tangent * t * 0.5f - EGG::Vector3f::ey * 5.0f);
    }

    m_up = OrthonormalBasis(m_tangent).base(1);
    setMatrixTangentTo(m_up, m_tangent);
}

} // namespace Kinoko::Field
