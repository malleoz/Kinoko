#include "KartReject.hh"

#include "game/kart/KartMove.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Kart {

/// @addr{Inlined in 0x80577FC4}
/// @brief Default constructor
KartReject::KartReject() = default;

/// @addr{0x8057815C}
/// @brief Default destructor
KartReject::~KartReject() = default;

/// @addr{0x80585AF8}
/// @brief Runs every frame to handle reject road interactions
/// @details This function has two different branches. The first branch involves checking if a
/// rejection should actually be applied to the kart. This is done by comparing the kart's up vector
/// against its last direction to see if it is traveling upwards along the reject road. If so, it
/// sets the @ref eStatus::RejectRoadTrigger flag to indicate that a rejection should be applied.
/// In doing so, it also computes in what direction (left or right) the rejection should be applied.
/// Once @ref eStatus::RejectRoadTrigger is set, the second branch of this function updates the
/// kart's rotation and calls @ref calcRejection to apply the rejection on the kart's position.
void KartReject::calc() {
    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::InAction)) {
        return;
    }

    if (status.onBit(eStatus::RejectRoadTrigger)) {
        EGG::Vector3f down = -EGG::Vector3f::ey;
        down = down.perpInPlane(move()->up(), true);
        f32 cos = down.dot(move()->lastDir());
        f32 sin = down.cross(move()->lastDir()).length();
        f32 angle = EGG::Mathf::atan2(sin, cos);
        angle = angle > 0.0f ? angle : -angle;

        f32 minAngle = 60.0f;
        angle *= RAD2DEG;
        f32 dVar11 = 1.0f;

        if (move()->up().dot(EGG::Vector3f::ey) < -0.7f) {
            minAngle *= 0.5f;
            dVar11 *= 2.0f;
        }

        if (angle > minAngle) {
            angle = 0.05f * (angle - 60.0f);
            EGG::Quatf rot = EGG::Quatf::FromRPY(0.0f,
                    (1.0f + angle * speedRatio()) * dVar11 * DEG2RAD * m_rejectSign, 0.0f);
            EGG::Quatf local_78 = mainRot().multSwap(rot);
            local_78.normalise();

            dynamics()->setAngVel0(EGG::Vector3f::zero);
            dynamics()->setFullRot(local_78);
            dynamics()->setMainRot(local_78);
        }

        status.resetBit(eStatus::Hop);

        bool didReject = calcRejection();

        if (status.offBit(eStatus::NoSparkInvisibleWall) && !didReject) {
            move()->clearRejectRoad();
        }

        return;
    }

    if (status.onBit(eStatus::RejectRoad) &&
            status.offBit(eStatus::ZipperInvisibleWall, eStatus::OverZipper,
                    eStatus::HalfPipeRamp)) {
        EGG::Vector3f upXZ = move()->up();
        upXZ.y = 0.0f;

        if (upXZ.length() > 0.0f && speed() > 0.0f) {
            upXZ.normalise();
            EGG::Vector3f local_88 = move()->lastDir().perpInPlane(upXZ, true);

            if (local_88.y > 0.0f) {
                EGG::Vector3f upCross = EGG::Vector3f::ey.cross(local_88);
                m_rejectSign = upCross.dot(move()->up()) > 0.0f ? 1.0f : -1.0f;

                status.resetBit(eStatus::Hop).setBit(eStatus::RejectRoadTrigger);
            }
        }
    }
}

/// @addr{0x805860BC}
/// @brief Performs a collision check and computes the resulting rejection if applicable
/// @details Runs a maximum of two collision checks: one with a sphere laying below the kart and one
/// with a sphere lying at the kart's position. If a collision occurs, calls @ref calcCollision to
/// see if the collision warrants a rejection. If so, the kart's up vector is snapped to the
/// collision's tangent vector. If the angle of rejection is very steep, or if the kart's external
/// velocity is downwards, updates the kart's position along the tangent direction based off a
/// speed-dependent scalar.
bool KartReject::calcRejection() {
    Field::CollisionInfo colInfo;
    Field::KCLTypeMask mask = KCL_NONE;
    auto &status = KartObjectProxy::status();
    status.resetBit(eStatus::NoSparkInvisibleWall);
    EGG::Vector3f upperPos = pos() + bodyUp() * 100.0f;
    f32 posOffset = 100.0f;
    f32 radius = posOffset;

    for (size_t i = 0; i < 2; ++i) {
        EGG::Vector3f lowerPos =
                pos() + (-posOffset * scale().y) * mainRot().rotateVector(EGG::Vector3f::ey);

        auto *colDir = Field::CollisionDirector::Instance();
        if (!colDir->checkSphereFullPush(radius, lowerPos, upperPos, KCL_TYPE_B0E82DFF, &colInfo,
                    &mask, 0)) {
            if (i == 0) {
                posOffset = 0.0f;
            }

            continue;
        }

        EGG::Vector3f tangentOff = EGG::Vector3f::zero;

        if (!calcCollision(colInfo, mask, tangentOff)) {
            continue;
        }

        EGG::Vector3f tangentUp = (tangentOff - move()->up()) * 1.0f;
        move()->setUp(move()->up() + tangentUp);
        move()->setSmoothedUp(move()->up());

        bool isSteepTangent = tangentOff.dot(EGG::Vector3f::ey) < -0.17f;
        if (isSteepTangent || extVel().y < 0.0f || status.onBit(eStatus::NoSparkInvisibleWall)) {
            colInfo.tangentOff += lowerPos;

            f32 yOffset = bsp().offsetY * scale().y;
            f32 speedScalar = isSteepTangent ?
                    1.0f :
                    static_cast<f32>(static_cast<f64>(EGG::Mathf::abs(speed()) * 0.01f) - 0.3);
            speedScalar = std::min(1.0f, std::max(0.0f, speedScalar));

            EGG::Vector3f posOffset =
                    colInfo.tangentOff + -radius * tangentOff + yOffset * tangentOff;
            posOffset.y += move()->hopPosY();
            posOffset -= pos();
            setPos(pos() + posOffset * speedScalar);
        }

        EGG::Vector3f newDir = move()->lastDir().perpInPlane(move()->smoothedUp(), true);
        move()->setDir(newDir);
        move()->setVel1Dir(newDir);

        return true;
    }

    return false;
}

/// @brief Checks if the collision should result in a rejection
/// @param colInfo The collision data of the colliding object
/// @param mask A mask representing the type of collision that occurred
/// @param tangentOff Output of the tangent offset vector, if a rejection is applicable
/// @return True if a rejection should be applied, false otherwise
bool KartReject::calcCollision(Field::CollisionInfo &colInfo, Field::KCLTypeMask mask,
        EGG::Vector3f &tangentOff) {
    auto *colDir = Field::CollisionDirector::Instance();

    if (mask & KCL_TYPE_INVISIBLE_WALL) {
        if (colDir->findClosestCollisionEntry(&mask, KCL_TYPE_INVISIBLE_WALL) &&
                colDir->closestCollisionEntry()->variant() == 0) {
            tangentOff = colInfo.wallNrm;
            status().setBit(eStatus::NoSparkInvisibleWall);
            return true;
        }
    }

    Field::KCLTypeMask halfPipeInvisMask = KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL);
    if (mask & halfPipeInvisMask) {
        if (colDir->findClosestCollisionEntry(&mask, halfPipeInvisMask)) {
            tangentOff = colInfo.wallNrm;
            status().setBit(eStatus::NoSparkInvisibleWall);
            return true;
        }
    }

    if (mask & KCL_TYPE_DRIVER_FLOOR) {
        if (colDir->findClosestCollisionEntry(&mask, KCL_TYPE_DRIVER_FLOOR) &&
                colDir->closestCollisionEntry()->attribute.onBit(
                        Field::CollisionDirector::eCollisionAttribute::RejectRoad)) {
            tangentOff = colInfo.floorNrm;
            return true;
        }
    }

    return false;
}

} // namespace Kinoko::Kart
