#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief A flamepole that erupts from a geyser on Bowser's Castle.
/// @details Though this object does not have any logic to oscillate its position, @ref
/// ObjectFlamePoleFoot dynamically adjusts the pole's Y-scale. The pole will then resize its
/// GJK collision to reflect the scale change.
class ObjectFlamePole final : public ObjectCollidable {
public:
    /// @addr{0x8067E280}
    ObjectFlamePole(const System::MapdataGeoObj &params, const EGG::Vector3f &pos,
            const EGG::Vector3f &rot, const EGG::Vector3f &scale)
        : ObjectCollidable("FlamePoleEff", pos, rot, scale) {}

    /// @addr{0x80681828}
    ~ObjectFlamePole() override = default;

    /// @addr{0x8067E410}
    void calc() override {
        if (m_isActive) {
            resize(RADIUS * scale().y, 0.0f);
        }
    }

    /// @addr{0x80681820}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    [[nodiscard]] const char *getResources() const override {
        return "FlamePole";
    }

    /// @brief Enables or disables GJK collision resizing
    void setActive(bool isSet) {
        m_isActive = isSet;
    }

    static constexpr f32 HEIGHT = 384.0f; ///< Normal height of the pole

private:
    bool m_isActive; ///< Used to toggle collision resizing on and off

    static constexpr f32 RADIUS = 70.0f; ///< Normal radius of the pole
};

} // namespace Kinoko::Field
