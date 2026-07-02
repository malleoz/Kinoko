#pragma once

#include "game/field/obj/ObjectDossun.hh"

namespace Kinoko::Field {

/// @brief Individual Thwomps that move along a rail and stomp
class ObjectDossunSyuukai final : public ObjectDossun {
public:
    ObjectDossunSyuukai(const System::MapdataGeoObj &params);
    ~ObjectDossunSyuukai() override;

    void init() override;
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

    void calcMoving();
    void calcRotating();

    State m_state;   ///< Current motion of the Thwomp
    f32 m_initYaw;   ///< Initial rotation about the Y-axis
    bool m_rotating; ///< Whether the Thwomp is currently rotating
};

} // namespace Kinoko::Field
