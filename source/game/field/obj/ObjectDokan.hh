#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents a pipe, like on SNES Mario Circuit 3
/// @details Pipes act like walls in time trials.
/// @note An interesting oddity of pipe behavior is that they stay at he param's position until they
/// are sent airborne regardless of whether or not they are colliding with the floor. Once they fly
/// airbone due to a kart colliding with the pipe while in a Mega Mushroom or Start, they will fly
/// up and fall downward until they collide with a floor.
class ObjectDokan final : public ObjectCollidable {
public:
    /// @addr{0x807787F0}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectDokan(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x80778FEC}
    /// @brief Default virtual destructor
    ~ObjectDokan() override = default;

    /// @addr{0x80778830}
    /// @copybrief ObjectBase::init()
    /// @details Sets @ref m_isAirborne to `false`, ensuring the pipe starts on the ground.
    void init() override {
        m_isAirborne = false;
    }

    /// @addr{0x807788C8}
    /// @copybrief ObjectBase::calc()
    /// @details If the pipe is not airborne, returns early. Otherwise, updates the pipe's position
    /// and checks for floor collisions. In Kinoko, this is effectively a no-op since karts can
    /// never send the pipe airborne in time trials, but we implement @ref calcPos() and @ref
    /// calcFloor() for completeness.
    void calc() override {
        if (!m_isAirborne) {
            return;
        }

        calcPos();
        calcFloor();
    }

    /// @addr{0x80778FE4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x80778D50}
    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details The collision transform varies between the SNES Mario Circuit and N64 Mario Raceway
    /// piranhas. SNES Mario Circuit 3 piranhas (identified with @ref ObjectId::DokanSFC) simply
    /// call @ref Collidable::calcCollisionTransform(). N64 Mario Raceway piranhas, on the other
    /// hand, extend the piranhas' vertical collision by 300 units upwards.
    void calcCollisionTransform() override {
        if (m_id == ObjectId::DokanSFC) {
            ObjectCollidable::calcCollisionTransform();
        } else {
            calcTransform();
            EGG::Matrix34f mat = transform();
            mat.setBase(3, mat.translation() + EGG::Vector3f::ey * 300.0f);
            m_collision->transform(mat, scale(), getCollisionTranslation());
        }
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    /// @addr{0x8077894C}
    /// @brief Applies gravity to velocity and updates position accordingly
    /// @details Gravity applies a `2.0f` unit downward acceleration to the pipe's velocity each
    /// frame.
    void calcPos() {
        constexpr f32 GRAVITY = 2.0f;

        m_velocity.y -= GRAVITY;
        addPos(m_velocity);
    }

    void calcFloor();

    /// @addr{0x80778BA0}
    /// @brief If the pipe is not already airborne, induces upwards velocity
    /// @details The pipe starts with `100.0f` units of upward velocity when it becomes airborne.
    /// This function does nothing if the pipe is already airborne.
    void tryStartAirborne() {
        constexpr f32 INITIAL_VELOCITY = 100.0f;

        if (!m_isAirborne) {
            m_isAirborne = true;
            m_velocity = INITIAL_VELOCITY * EGG::Vector3f::ey;
        }
    }

    bool m_isAirborne;        ///< Whether the pipe is currently mid-air
    EGG::Vector3f m_velocity; ////< The current velocity of the pipe
};

} // namespace Kinoko::Field
