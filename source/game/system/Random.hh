#pragma once

/// Based off https://github.com/em-eight/mkw/blob/master/source/game/util/Random.cpp

#include <Common.hh>

namespace Kinoko::System {

/// @brief The game's Random Number Generator (RNG) implementation
/// @details The game's RNG is implemented as a linear congruential generator (LCG) with a 64-bit
/// state with the following formula:
/// @f[
///     X_{n+1} = (A \cdot X_n + C) \bmod 2^{64}
/// @f]
/// such that:
/// - @f$ A = \mathrm{0x690379B2B2E3D431} @f$
/// - @f$ C = \mathrm{0x508EBD} @f$
///
/// When fetching random numbers, the upper 32 bits of the 64-bit state are used.
/// @see https://en.wikipedia.org/wiki/Linear_congruential_generator
class Random {
public:
    /// @brief Constructor which sets the initial RNG value
    /// @param seed The initial seed value for the RNG
    Random(u32 seed) : m_x(seed), m_seed(seed) {}

    /// @brief Copy constructor
    /// @param rhs The @ref Random instance to copy from
    /// @details Marked explicit to prevent accidental copying instead of passing by reference.
    explicit Random(const Random &rhs) = default;

    /// @brief Default destructor
    ~Random() = default;

    /// @brief Advances the RNG state to the next value
    void next() {
        m_x = A * m_x + C;
    }

    /// @brief Advances the RNG state and returns a 4-byte unsigned integer from the upper 4 bytes
    /// of the RNG state
    /// @return A 4-byte unsigned integer from the upper 4 bytes of the new RNG state
    [[nodiscard]] u32 getU32() {
        next();
        return static_cast<u32>(m_x >> 0x20);
    }

    /// @brief Advances the RNG state and returns a 4-byte unsigned integer from the upper 4 bytes
    /// of the RNG state in the range of [0, range]
    /// @param range The upper bound of the random value to generate
    /// @return A 4-byte unsigned integer from the upper 4 bytes of the new RNG state, scaled to the
    /// specified range
    [[nodiscard]] u32 getU32(u32 range) {
        next();
        return ((m_x >> 0x20) * range) >> 0x20;
    }

    /// @brief Advances the RNG state and returns a 4-byte float in the range [0.0f, 1.0f] from the
    /// upper 4 bytes of the new RNG state
    /// @return A 4-byte float in the range [0.0f, 1.0f] from the upper 4 bytes of the new RNG state
    [[nodiscard]] f32 getF32() {
        return MUL * static_cast<f32>(getU32());
    }

    /// @brief Advances the RNG state and returns a 4-byte float in the range [0.0f, range] from the
    /// upper 4 bytes of the new RNG state
    /// @param range The upper bound of the random value to generate
    /// @return A 4-byte float in the range [0.0f, range] from the upper 4 bytes of the new RNG
    /// state
    [[nodiscard]] f32 getF32(f32 range) {
        return range * getF32();
    }

private:
    u64 m_x;    ///< The current state of the RNG
    u64 m_seed; ///< The initial seed value for the RNG

    static constexpr u64 A = 0x690379B2B2E3D431;     ///< The multiplier
    static constexpr u32 C = 0x508EBD;               ///< The increment
    static constexpr f32 MUL = 1.0f / 4294967296.0f; ///< Uint-to-float converter: 1 / (2 ^ 32)
};

} // namespace Kinoko::System
