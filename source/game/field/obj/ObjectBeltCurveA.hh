#pragma once

#include "game/field/obj/ObjectBelt.hh"

namespace Kinoko::Field {

/// @brief The set of curved conveyor belts at the end of the factory on Toad's Factory
/// @details This conveyor switches direction twice.
class ObjectBeltCurveA final : public ObjectBelt {
public:
    /// @addr{0x807FC90C}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectBeltCurveA(const System::MapdataGeoObj &params)
        : ObjectBelt(params),
          m_startForward(params.setting(1) == 1),
          m_dirChange1Frame(params.setting(2) * 60),
          m_dirChange2Frame(params.setting(3) * 60) {
        constexpr EGG::Vector3f INITIAL_ROT = EGG::Vector3f(0.0f, HALF_PI, 0.0f);

        m_initRt.makeR(INITIAL_ROT);
    }

    /// @addr{0x807FD7BC}
    /// @brief Default virtual destructor
    ~ObjectBeltCurveA() override = default;

    [[nodiscard]] EGG::Vector3f calcRoadVelocity(u32 variant, const EGG::Vector3f &pos,
            u32 timeOffset) const override;

    /// @addr{0x807FCCA4}
    /// @brief Determines whether or not the conveyer belt is moving based on its variant
    /// @param variant The variant of the conveyer belt
    /// @return Whether or not the conveyer belt is moving at the given position and variant
    [[nodiscard]] bool isMoving(u32 variant, const EGG::Vector3f & /*pos*/) const override {
        return variant == 4 || variant == 5;
    }

private:
    [[nodiscard]] f32 calcDirSwitchSpeed(u32 t) const;
    [[nodiscard]] bool isMovingForward(u32 t) const;

    const bool m_startForward;   ///< Whether the belts start moving forward or backward
    const u16 m_dirChange1Frame; ///< Frame of first direction change
    const u16 m_dirChange2Frame; ///< Frame of second direction change
    EGG::Matrix34f m_initRt;     ///< The initial transformation matrix
};

} // namespace Kinoko::Field
