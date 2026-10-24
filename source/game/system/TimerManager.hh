#pragma once

#include "game/system/Timer.hh"

namespace Kinoko::System {

/// @brief Manages the race timer to create lap splits and final times
class TimerManager {
public:
    /// @addr{Inlined in 0x805327A0}
    /// @brief Constructor
    /// @details Simply calls @ref init().
    TimerManager() {
        init();
    }

    /// @addr{0x805376E0}
    /// @brief Default destructor
    ~TimerManager() = default;

    /// @addr{0x80535864}
    /// @brief Zeroes out the current timer and resets the frame counter
    void init() {
        m_currentTimer.min = 0;
        m_currentTimer.sec = 0;
        m_currentTimer.mil = 0;
        m_currentTimer.valid = true;

        m_started = false;
        m_frameCounter = 0;
    }

    /// @addr{0x80535904}
    /// @brief Every frame after the race starts, updates the timer based on the frame counter
    /// @details The game runs at 59.94fps and the timer is updated accordingly each frame.
    void calc() {
        constexpr f32 REFRESH_PERIOD = 1000.0f / 59.94f;
        constexpr f32 MILLISECONDS_TO_MINUTES = 1.0f / 60000.0f;
        constexpr f32 MILLISECONDS_TO_SECONDS = 1.0f / 1000.0f;
        constexpr f32 SECONDS_TO_MILLISECONDS = 1000.0f;
        constexpr f32 MINUTES_TO_MILLISECONDS = 60000.0f;

        if (!m_started) {
            return;
        }

        u32 minutesMs = static_cast<u32>(static_cast<f32>(m_frameCounter) * REFRESH_PERIOD);
        u16 minutes = static_cast<u16>(static_cast<f32>(minutesMs) * MILLISECONDS_TO_MINUTES);
        u32 secondsMs = static_cast<u32>(
                static_cast<f32>(minutesMs) - static_cast<f32>(minutes) * MINUTES_TO_MILLISECONDS);
        u8 seconds = static_cast<u8>(static_cast<f32>(secondsMs) * MILLISECONDS_TO_SECONDS);
        u16 milliseconds = static_cast<u16>(
                static_cast<f32>(secondsMs) - static_cast<f32>(seconds) * SECONDS_TO_MILLISECONDS);

        m_currentTimer.min = minutes;
        m_currentTimer.sec = seconds;
        m_currentTimer.mil = milliseconds;
        m_currentTimer.valid = true;

        ++m_frameCounter;
    }

    /// @beginGetters

    /// @brief Retrieves the current race timer
    /// @return A const ref to the current race timer, @ref m_currentTimer
    [[nodiscard]] const Timer &currentTimer() const {
        return m_currentTimer;
    }

    /// @endGetters

    /// @beginSetters

    /// @brief Toggles whether the race has started
    /// @param isSet A boolean indicating whether the race has started
    /// @details Called when the countdown has ended to set @ref m_started to `true`.
    void setStarted(bool isSet) {
        m_started = isSet;
    }

    /// @endSetters

private:
    Timer m_currentTimer; ///< The current race timer
    bool m_started;       ///< Indicates whether the race has started
    u32 m_frameCounter;   ///< The number of frames since the race started
};

} // namespace Kinoko::System
