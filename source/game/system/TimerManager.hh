#pragma once

#include <Common.hh>

namespace Kinoko::System {

/// @brief A simple struct to represent a lap or race finish time
struct Timer {
    /// @addr{0x8051C374}
    /// @brief Constructor that creates a zeroed "invalid" timer
    Timer() : min(0), sec(0), mil(0), valid(false) {}

    /// @brief Constructor that creates a timer with the specified time
    /// @param min_ The minutes component of the timer
    /// @param sec_ The seconds component of the timer
    /// @param mil_ The milliseconds component of the timer
    Timer(u16 min_, u8 sec_, u16 mil_) : min(min_), sec(sec_), mil(mil_), valid(true) {}

    /// @brief Constructor that parses a time from an RKG's 3 byte time format. @see RawGhostFile
    /// @param data The 3 byte time data from an RKG file
    Timer(u32 data) {
        min = static_cast<u16>(data >> 0x19);
        sec = static_cast<u8>(data >> 0x12) & 0x7F;
        mil = static_cast<u16>(data >> 8) & 0x3FF;
        valid = true;
    }

    /// @addr{0x8051C334}
    /// @brief Default destructor
    ~Timer() = default;

    /// @brief Rocketship operator allowing for two timers to be compared
    /// @param rhs The other timer to compare against
    /// @return A `std::strong_ordering` value indicating the comparison result
    std::strong_ordering operator<=>(const Timer &rhs) const {
        if (auto cmp = min <=> rhs.min; cmp != 0) {
            return cmp;
        }

        if (auto cmp = sec <=> rhs.sec; cmp != 0) {
            return cmp;
        }

        if (auto cmp = mil <=> rhs.mil; cmp != 0) {
            return cmp;
        }

        return valid <=> rhs.valid;
    }

    /// @brief Default equality operator for comparing two timers
    /// @param rhs The other timer to compare against
    /// @return `true` if the timers are equal, `false` otherwise
    bool operator==(const Timer &rhs) const = default;

    /// @brief Default inequality operator for comparing two timers
    /// @param rhs The other timer to compare against
    /// @return `true` if the timers are not equal, `false` otherwise
    bool operator!=(const Timer &rhs) const = default;

    /// @addr{0x807EE860}
    /// @brief Subtraction operator for calculating the difference between two timers
    /// @param rhs The other timer to subtract
    /// @return A new @ref Timer representing the difference
    Timer operator-(const Timer &rhs) const {
        s16 addMin = 0;
        s16 addSec = 0;

        s16 newMs = mil - rhs.mil;
        if (newMs < 0) {
            addSec = -1;
            newMs += 1000;
        }

        s16 newSec = addSec + sec - rhs.sec;
        if (newSec < 0) {
            addMin = -1;
            newSec += 60;
        }

        s16 newMin = addMin + min - rhs.min;
        if (newMin < 0) {
            newMin = 0;
            newSec = 0;
            newMs = 0;
        }

        return Timer(newMin, newSec, newMs);
    }

    /// @brief Addition operator for adding milliseconds to a timer
    /// @param ms The number of milliseconds to add
    /// @return A new @ref Timer representing the updated time
    Timer operator+(f32 ms) const {
        s16 addMin = 0;
        s16 addSec = 0;

        s16 newMs = static_cast<s16>(ms + static_cast<f32>(mil));
        if (newMs > 999) {
            addSec = 1;
            newMs -= 1000;
        }

        s16 newSec = addSec + sec;
        if (newSec > 59) {
            addMin = 1;
            newSec -= 60;
        }

        s16 newMin = addMin + min;
        if (newMin > 999) {
            newMin = 999;
            newSec = 59;
            newMs = 999;
        }

        return Timer(newMin, newSec, newMs);
    }

    u16 min; ///< The number of minutes
    u8 sec;  ///< The number of seconds
    u16 mil; ///< The number of milliseconds (@todo expand to float for more precise finish times)
    bool valid; ///< Indicates whether the timer is valid
};

/// @brief Manages the race timer to create lap splits and final times
class TimerManager {
public:
    /// @addr{Inlined in 0x805327A0}
    /// @brief Constructor which initializes the current timer and framecounter
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
    /// @brief Every frame after he race starts, updates the timer based on the frame counter
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
    [[nodiscard]] const Timer &currentTimer() const {
        return m_currentTimer;
    }
    /// @endGetters

    /// @beginSetters
    /// @brief Called when the countdown has ended
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
