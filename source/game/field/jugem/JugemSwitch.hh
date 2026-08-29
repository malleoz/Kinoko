#pragma once

#include <Common.hh>

namespace Kinoko::Field {

/// @brief Base class which is used to represent scenarios that toggle Lakitu on or off
class JugemSwitch {
public:
    /// @addr{0x807259EC}
    /// @brief Constructor
    JugemSwitch() : m_isOn(false) {}

    /// @addr{0x80725A0C}
    /// @brief Default virtual destructor
    virtual ~JugemSwitch() = default;

    [[nodiscard]] bool isOn() const {
        return m_isOn;
    }

    /// @brief Initializes the internal state of the switch
    /// @addr{0x80725A4C}
    virtual void init() {
        m_isOn = false;
    }

    /// @brief Updates the internal state of the switch, checking if Lakitu should be toggled on/off
    virtual void calc() = 0;

protected:
    bool m_isOn; ///< True if the switch has been toggled on, false otherwise
};

/// @brief Represents a switch that toggles Lakitu on when the player is driving backwards
/// @details Lakitu will be toggled on if the player is driving the wrong way for 60 frames.
class JugemSwitchReverse : public JugemSwitch {
public:
    /// @addr{0x80725C1C}
    /// @brief Default constructor
    JugemSwitchReverse() = default;

    /// @addr{0x80725C3C}
    /// @brief Default virtual destructor
    ~JugemSwitchReverse() override = default;

    /// @addr{0x80725C7C}
    void init() override {
        m_isOn = false;
        m_activationPercent = 0.0f;
    }

    void calc() override;

private:
    f32 m_activationPercent; ///< Lakitu is activated when this reaches 1.0f
};

} // namespace Kinoko::Field
