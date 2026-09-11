#pragma once

#include "game/field/obj/ObjectDossun.hh"

namespace Kinoko::Field {

/// @brief Individual Thwomps that move along a rail and stomp
class ObjectDossunSyuukai final : public ObjectDossun {
public:
    /// @addr{0x80760B20}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectDossunSyuukai(const System::MapdataGeoObj &params) : ObjectDossun(params) {}

    /// @addr{0x80764B88}
    /// @brief Default virtual destructor
    ~ObjectDossunSyuukai() override = default;

    /// @addr{0x80760BD4}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the Thwomp. including setting its @ref m_state to @ref State::Moving,
    /// caching the Thwomp's initial rotation, and setting the @m_rotating flag.
    void init() override {
        ObjectDossun::init();

        m_state = State::Moving;
        m_initYaw = rot().y;
        m_rotating = true;
    }

    void calc() override;

    /// @addr{0x8076139C}
    void startStill() override {
        ObjectDossun::startStill();
        m_state = State::RotatingAfterStomp;
    }

private:
    /// @brief Describes the current motion state of the Thwomp
    enum class State {
        Moving = 0,              ///< Moving along the rail
        RotatingBeforeStomp = 1, ///< Still and rotating
        Stomping = 2,            ///< Stomping down
        RotatingAfterStomp = 3,  ///< Still and rotating to face rail direction
    };

    /// @addr{0x80760D18}
    /// @brief Runs once per frame while the Thwomp is moving
    void calcMoving() {
        if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd) {
            m_state = State::RotatingBeforeStomp;
        }

        setPos(m_railInterpolator->curPos());
    }

    void calcRotating();

    State m_state;   ///< Current motion of the Thwomp
    f32 m_initYaw;   ///< Initial rotation about the Y-axis
    bool m_rotating; ///< Whether the Thwomp is currently rotating
};

} // namespace Kinoko::Field
