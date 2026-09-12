#pragma once

#include "game/field/obj/ObjectDossun.hh"

namespace Kinoko::Field {

/// @brief Represents Thwomps that move sideways on the ground on N64 Bowser's Castle
class ObjectDossunYokoMove final : public ObjectDossun {
public:
    /// @addr{0x80763B60}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectDossunYokoMove(const System::MapdataGeoObj &params) : ObjectDossun(params) {}

    /// @addr{0x807645C4}
    /// @brief Default virtual destructor
    ~ObjectDossunYokoMove() override = default;

    /// @addr{0x80763C14}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the rail interpolator.
    void init() override {
        m_railInterpolator->init(0.0f, 0);
    }

    /// @addr{0x80763E0C}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the rail interpolator and sets the object's position accordingly.
    void calc() override {
        m_railInterpolator->calc();
        setPos(m_railInterpolator->curPos());
    }
};

} // namespace Kinoko::Field
