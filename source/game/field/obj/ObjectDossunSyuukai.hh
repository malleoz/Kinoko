#pragma once

#include "game/field/obj/ObjectDossun.hh"

namespace Field {

class ObjectDossunSyuukai final : public ObjectDossun {
public:
    ObjectDossunSyuukai(const System::MapdataGeoObj &params);
    ~ObjectDossunSyuukai() override;

    void init() override;
    void calc() override;

    void startStill() override;

private:
    enum class State {
        Moving = 0,
        RotatingBeforeStomp = 1,
        Stomping = 2,
        RotatingAfterStomp = 3,
    };

    void calcRotating();

    State m_state;
    f32 m_initRotY;
    bool m_rotating;
};

} // namespace Field
