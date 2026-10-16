#pragma once

#include <Common.hh>

namespace Kinoko::Field {

/// @brief Base class which is used to represent scenarios that toggle Lakitu on or off
class JugemSwitch {
public:
    JugemSwitch();
    virtual ~JugemSwitch();

    virtual void init();

    /// @brief Updates the internal state of the switch, checking if Lakitu should be toggled on/off
    virtual void calc() = 0;

    /// @beginGetters

    [[nodiscard]] bool isOn() const;

    /// @endGetters

protected:
    bool m_isOn; ///< `true` if the switch has been toggled on, `false` otherwise
};

/// @brief Represents a switch that toggles Lakitu on when the player is driving backwards
/// @details Lakitu will be toggled on if the player is driving the wrong way for 60 frames.
class JugemSwitchReverse final : public JugemSwitch {
public:
    JugemSwitchReverse();
    ~JugemSwitchReverse() override;

    void init() override;
    void calc() override;

private:
    f32 m_activationPercent; ///< Lakitu is activated when this reaches `1.0f`
};

} // namespace Kinoko::Field
