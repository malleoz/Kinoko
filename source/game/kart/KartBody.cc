#include "KartBody.hh"

#include <egg/math/Math.hh>

namespace Kinoko::Kart {

/// @addr{0x8056C394}
KartBody::KartBody(KartPhysics *physics) : m_physics(physics) {
    m_anAngle = 0.0f;
    m_sinkDepth = 0.0f;
    m_targetSinkDepth = 0.0f;
}

/// @addr{0x8056CCC0}
KartBodyKart::KartBodyKart(KartPhysics *physics) : KartBody(physics) {}

/// @addr{0x8056E494}
KartBodyKart::~KartBodyKart() {
    EGG::egg_delete(m_physics);
}

/// @addr{0x8056D858}
KartBodyBike::KartBodyBike(KartPhysics *physics) : KartBody(physics) {}

/// @addr{0x8056E2BC}
KartBodyBike::~KartBodyBike() {
    EGG::egg_delete(m_physics);
}

/// @addr{0x8056DD54}
/// @brief Computes a matrix to represent the rotation of a wheel.
/// @details For the front wheel, we factor in the handlebar rotation.
/// For the rear wheel, we only factor in the kart's rotation.
/// @param wheelIdx 0 for front wheel, 1 for rear wheel
EGG::Matrix34f KartBodyBike::wheelMatrix(u16 wheelIdx) {
    EGG::Matrix34f mat;

    mat.makeQT(fullRot(), pos());
    if (wheelIdx != 0) {
        return mat;
    }

    EGG::Vector3f position = param()->bikeDisp().m_handlePos * scale();
    EGG::Vector3f rotation = DEG2RAD * param()->bikeDisp().m_handleRot;

    EGG::Matrix34f handleMatrix;
    handleMatrix.makeRT(rotation, position);
    EGG::Matrix34f tmp = mat.multiplyTo(handleMatrix);

    EGG::Vector3f yRotation = EGG::Vector3f(0.0f, DEG2RAD * m_anAngle, 0.0f);
    EGG::Matrix34f yRotMatrix;
    yRotMatrix.makeR(yRotation);
    mat = tmp.multiplyTo(yRotMatrix);

    return mat;
}

} // namespace Kinoko::Kart
