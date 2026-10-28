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
        m_started = false;
        m_frameCounter = 0;
    }

    /// @addr{0x80535904}
    /// @brief Every frame after the race starts, updates the timer based on the frame counter
    /// @details The game runs at 59.94fps and the timer is updated accordingly each frame.
    /// @note Kinoko-specific: the timer is a pure function of the frame counter, so the conversion
    /// to minutes, seconds, and milliseconds is deferred until the timer is read via @ref
    /// currentTimer(). The result is identical to converting it every frame.
    void calc() {
        if (!m_started) {
            return;
        }

        ++m_frameCounter;
    }

    /// @brief Finds the first frame counter value whose timer reads at least `minutes` minutes
    /// @param minutes The number of minutes to search for
    /// @return The first frame counter value whose timer reads at least `minutes` minutes
    /// @details Kinoko-specific. The timer's minutes never decrease as the frame counter increases,
    /// so callers can compare against this instead of converting the timer every frame.
    [[nodiscard]] static consteval u32 FirstFrameWithMinutes(u16 minutes) {
        u32 frame = 0;
        while (CalcMinutes(CalcTotalMs(frame)) < minutes) {
            ++frame;
        }

        return frame;
    }

    /// @beginGetters

    /// @brief Computes the current race timer
    /// @return A @ref Timer storing the current race time
    [[nodiscard]] Timer currentTimer() const {
        u32 minutesMs = CalcTotalMs(m_frameCounter - 1);
        u16 minutes = CalcMinutes(minutesMs);
        u32 secondsMs = static_cast<u32>(
                static_cast<f32>(minutesMs) - static_cast<f32>(minutes) * MINUTES_TO_MILLISECONDS);
        u8 seconds = static_cast<u8>(static_cast<f32>(secondsMs) * MILLISECONDS_TO_SECONDS);
        u16 milliseconds = static_cast<u16>(
                static_cast<f32>(secondsMs) - static_cast<f32>(seconds) * SECONDS_TO_MILLISECONDS);

        return Timer(minutes, seconds, milliseconds);
    }

    /// @brief Retrieves the number of frames the timer has been updated since the race started
    /// @return The number of frames since the race started, @ref m_frameCounter
    /// @details @ref currentTimer() reflects frame `m_frameCounter - 1`, or zero if no frames
    /// have elapsed.
    [[nodiscard]] u32 frameCounter() const {
        return m_frameCounter;
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
    static constexpr f32 REFRESH_PERIOD = 1000.0f / 59.94f;
    static constexpr f32 MILLISECONDS_TO_MINUTES = 1.0f / 60000.0f;
    static constexpr f32 MILLISECONDS_TO_SECONDS = 1.0f / 1000.0f;
    static constexpr f32 SECONDS_TO_MILLISECONDS = 1000.0f;
    static constexpr f32 MINUTES_TO_MILLISECONDS = 60000.0f;

    /// @brief Converts a frame counter value to the total number of elapsed milliseconds
    [[nodiscard]] static constexpr u32 CalcTotalMs(u32 frame) {
        return static_cast<u32>(static_cast<f32>(frame) * REFRESH_PERIOD);
    }

    /// @brief Converts a total number of elapsed milliseconds to whole minutes
    [[nodiscard]] static constexpr u16 CalcMinutes(u32 totalMs) {
        return static_cast<u16>(static_cast<f32>(totalMs) * MILLISECONDS_TO_MINUTES);
    }

    bool m_started;     ///< Indicates whether the race has started
    u32 m_frameCounter; ///< The number of frames since the race started
};

} // namespace Kinoko::System
