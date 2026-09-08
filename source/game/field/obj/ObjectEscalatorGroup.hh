#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectEscalator;

/// @brief Represents a group of two escalators and the Pianta dancing in the middle.
class ObjectEscalatorGroup final : public ObjectCollidable {
public:
    ObjectEscalatorGroup(const System::MapdataGeoObj &params);
    ~ObjectEscalatorGroup() override;

    /// @addr{0x80802D18}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x80802D00}
    /// @copybrief ObjectBase::getResources()
    /// @details Returns the resource name for the dancing Piantas between the escalators.
    /// @return The resource name for the dancing Piantas between the escalators (`monte_a`).
    [[nodiscard]] const char *getResources() const override {
        return "monte_a";
    }

    /// @addr{0x80802D0C}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the dancing Piantas between the escalators (`monte_a`).
    [[nodiscard]] const char *getKclName() const override {
        return "monte_a";
    }

    /// @copybrief ObjectBase::createCollision()
    /// @details Not overridden in the base game, but it's effectively a no-p because the
    /// corresponding kcl (`monte_a`) doesn't have any primitive collision.
    void createCollision() override {}

private:
    ObjectEscalator *m_rightEscalator; ///< Pointer to the right escalator
    ObjectEscalator *m_leftEscalator;  ///< Pointer to the left escalator
};

} // namespace Kinoko::Field
