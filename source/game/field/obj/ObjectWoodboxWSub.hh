#pragma once

#include "game/field/obj/ObjectWoodbox.hh"

namespace Kinoko::Field {

/// @brief Represents a single wooden box spawned by an @ref ObjectWoodboxW
/// @details The @ref ObjectWoodboxW spawner is responsible for using @ref enableCollision() to make
/// the box tangible again and reset its rail position.
class ObjectWoodboxWSub final : public ObjectWoodbox {
public:
    /// @addr{0x8077E34C}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectWoodboxWSub(const System::MapdataGeoObj &params) : ObjectWoodbox(params) {}

    /// @addr{0x8077E388}
    /// @brief Default virtual destructor
    ~ObjectWoodboxWSub() override = default;

    /// @addr{0x8077E3E4}
    /// @copybrief ObjectBase::init()
    void init() override {
        ObjectBreakable::init();
        m_state = 0;
    }

    /// @addr{0x8077E49C}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        if (m_state - 1 > 1) {
            return;
        }

        calcPosition();
    }

    /// @addr{0x8077EDA4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8077E444}
    void enableCollision() override {
        ObjectBreakable::enableCollision();
        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setPerPointVelocities(true);
    }

private:
    /// @addr{0x8077E56C}
    /// @brief Updates the rail interpolator and the box's position along the rail
    void calcPosition() {
        auto status = m_railInterpolator->calc();

        if (status == RailInterpolator::Status::ChangingDirection) {
            m_state = 0;
        }

        setPos(m_railInterpolator->curPos());
    }
};

} // namespace Kinoko::Field
