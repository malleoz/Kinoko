#pragma once

#include "game/field/obj/ObjectDossun.hh"

namespace Kinoko::Field {

/// @brief A Thwomp that remains in one position and stomps downward.
class ObjectDossunNormal final : public ObjectDossun {
public:
    ObjectDossunNormal(const System::MapdataGeoObj &params);
    ~ObjectDossunNormal() override;

    void init() override;
    void calc() override;

    void startStill() override;

    /// @addr{0x80760964}
    /// @brief Runs once when the Thwomp begins rising before stomping down
    void startBeforeFall() {
        m_stompState = StompState::Active;
        m_anmState = AnmState::BeforeFall;
        m_beforeFallTimer = static_cast<s32>(BEFORE_FALL_DURATION);
        m_stompDuration = static_cast<s32>(m_fullDuration);
    }

private:
    void calcInactive();
};

} // namespace Kinoko::Field
