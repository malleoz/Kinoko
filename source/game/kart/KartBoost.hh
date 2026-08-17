#pragma once

#include <Common.hh>

namespace Kinoko::Kart {

/// @brief State management for boosts (start boost, mushrooms, mini-turbos)
class KartBoost {
public:
    /// @brief The different types of boosts that can be applied to the kart
    enum class Type {
        MiniTurbo,             ///< Boost applied after a drift, race start, or respawn
        MushroomAndBoostPanel, ///< Using a mushroom or driving over a boost panel
        TrickAndZipper,        ///< Boost applied after landing from a trick or a zipper
        Max,                   ///< The maximum number of boost types, used for array sizing
    };

    KartBoost();
    ~KartBoost();

    [[nodiscard]] bool activate(Type type, s16 frames);
    [[nodiscard]] bool calc();

    /// @addr{0x80588D74}
    /// @brief Initializes the current boost state
    void init() {
        m_timers.fill(0);
        m_multiplier = 1.0f;
        m_acceleration = 1.0f;
        m_speedLimit = -1.0f;
    }

    /// @addr{0x80588E18}
    /// @brief Clears the current boost state
    void resetActive() {
        m_active.fill(false);
    }

    /// @beginGetters
    [[nodiscard]] f32 multiplier() const {
        return m_multiplier;
    }

    [[nodiscard]] f32 acceleration() const {
        return m_acceleration;
    }

    [[nodiscard]] f32 speedLimit() const {
        return m_speedLimit;
    }
    /// @endGetters

private:
    /// @brief The number of boost types, used for array sizing
    static constexpr size_t BOOST_TYPE_COUNT = static_cast<size_t>(Type::Max);

    std::array<s16, BOOST_TYPE_COUNT> m_timers;  ///< Remaining frames for the different boost types
    std::array<bool, BOOST_TYPE_COUNT> m_active; ///< Whether the different boost types are active
    f32 m_multiplier;                            ///< Multiplier applied to vehicle speed
    f32 m_acceleration; ///< Acceleration of the currently active boost type
    f32 m_speedLimit;   ///< Maximum speed while in boost, or -1 if no speed limit is applied
};

} // namespace Kinoko::Kart
