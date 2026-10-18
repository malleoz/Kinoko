#pragma once

#include <Common.hh>

#include <array>

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

    void init();
    void resetActive();

    /// @beginGetters

    [[nodiscard]] f32 multiplier() const;
    [[nodiscard]] f32 acceleration() const;
    [[nodiscard]] f32 speedLimit() const;

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
