#include "ObjectShip64.hh"

namespace Kinoko::Field {

/// @addr{0x80765C94}
ObjectShip64::ObjectShip64(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

/// @addr{0x80765DB0}
ObjectShip64::~ObjectShip64() {
    EGG::egg_delete(m_paddleWheelCollision);
}

/// @addr{0x80766864}
/// @details Creates the primary and paddle wheel collision objects for the ship.
void ObjectShip64::createCollision() {
    constexpr f32 RADIUS = 1500.0f;
    constexpr f32 HEIGHT = 3500.0f;

    ObjectCollidable::createCollision();
    m_paddleWheelCollision =
            EGG::egg_new<ObjectCollisionCylinder>(RADIUS, HEIGHT, EGG::Vector3f::zero);
}

/// @addr{0x807668D4}
/// @details Calculates the transformation matrices for the primary and paddle wheel collision
/// objects based on the ship's current orientation.
void ObjectShip64::calcCollisionTransform() {
    ObjectCollidable::calcCollisionTransform();
    calcTransform();
    EGG::Matrix34f mat = transform();

    EGG::Vector3f v;
    v = mat.base(0);
    v.normalise();

    mat.setAxisRotation(F_PI / 2.0f, v);
    mat.setBase(3, v);
    m_paddleWheelCollision->transform(mat, scale());
}

/// @addr{0x80766BCC}
/// @details Checks for collision against both the boat and the paddle wheel.
bool ObjectShip64::checkCollision(ObjectCollisionBase *lhs, EGG::Vector3f &dist) {
    EGG::Vector3f colDist = EGG::Vector3f::zero;
    EGG::Vector3f auxDist = EGG::Vector3f::zero;

    bool has_col =
            lhs->check(*m_collision, colDist) || lhs->check(*m_paddleWheelCollision, auxDist);
    dist = colDist + auxDist;

    return has_col;
}

} // namespace Kinoko::Field
