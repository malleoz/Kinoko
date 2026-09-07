#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents the Pokies on Dry Dry Ruins and DS Desert Hills
/// @details Pokies walk back and forth along their rail. They also perform collision checks against
/// the floor and other objects, including the Dry Dry Ruins sandcones, so that they don't clip
/// inside the sandcones.
class ObjectSanbo final : public ObjectCollidable {
public:
    ObjectSanbo(const System::MapdataGeoObj &params);
    ~ObjectSanbo() override;

    void init() override;

    /// @addr{0x8077A36C}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        calcMove();
    }

    /// @addr{0x8077BD38}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

private:
    void calcMove();
    void checkSphere();

    bool m_standstill;       ///< Whether or not the pokey is currently walking
    u32 m_stillTimer;        ///< Frames before the pokey will start to walk
    f32 m_yVel;              ///< Falling speed
    EGG::Vector3f m_up;      ///< Upwards direction of the Pokey
    EGG::Vector3f m_tangent; ///< Forward direction of the Pokey
};

} // namespace Kinoko::Field
