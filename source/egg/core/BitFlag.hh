#pragma once

#include <Common.hh>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace Kinoko {

namespace EGG {

/// @brief Wrapper around an integral type with an enum corresponding to its bits.
/// @tparam T The underlying integral type.
/// @tparam E The enum to correspond to.
template <typename T, typename E>
    requires(std::is_enum_v<E> && std::is_integral_v<T>)
struct TBitFlag {
    /// @brief Default constructor, initializes all flags to off.
    /// @details Matches the expression `bits = 0`.
    constexpr TBitFlag() {
        makeAllZero();
    }

    /// @brief Constructor that initializes the bit flags with a given mask.
    /// @param mask The initial bit mask.
    /// @details Matches the expression `bits = mask`.
    constexpr TBitFlag(T mask) {
        setDirect(mask);
    }

    /// @brief Constructor that initializes the bit flags with a given enum.
    /// @param e The initial enum.
    /// @details Matches the expression `bits = eEnum::val`.
    constexpr TBitFlag(E e) : TBitFlag() {
        setBit(e);
    }

    /// @brief Constructor that initializes the bit flags with the given enums.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to set.
    /// @details Matches the expression `bits = mask`
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    constexpr TBitFlag(Es... es) : TBitFlag() {
        (setBit(es), ...);
    }

    /// @brief Conversion operator to the underlying integral type.
    /// @return The underlying integral value representing the current bit flags.
    [[nodiscard]] constexpr operator T() const {
        return getDirect();
    }

    /// @brief Assignment operator.
    /// @param rhs Source.
    /// @return Reference for chaining.
    constexpr TBitFlag<T, E> &operator=(const TBitFlag<T, E> &rhs) {
        bits = rhs.bits;
        return *this;
    }

    /// @brief Sets the corresponding bits for the provided enum values.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to set.
    /// @return Reference for chaining.
    /// @details Matches the expression `bits |= mask`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    constexpr TBitFlag<T, E> &setBit(Es... es) {
        (setBit_(es), ...);
        return *this;
    }

    /// @brief Resets the corresponding bits for the provided enum values.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to reset.
    /// @return Reference for chaining.
    /// @details Matches the expression `bits &= ~mask`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    constexpr TBitFlag<T, E> &resetBit(Es... es) {
        (resetBit_(es), ...);
        return *this;
    }

    /// @brief Changes the state of the corresponding bits for the provided enum values.
    /// @tparam ...Es Variadic template for packing.
    /// @param on Determines whether to set or reset the bits.
    /// @param ...es Enum values representing the bits to change.
    /// @return Reference for chaining.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    constexpr TBitFlag<T, E> &changeBit(bool on, Es... es) {
        (changeBit_(on, es), ...);
        return *this;
    }

    /// @brief Toggles the corresponding bits for the provided enum values.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to toggle.
    /// @return Reference for chaining.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    constexpr TBitFlag<T, E> &toggleBit(Es... es) {
        (toggleBit_(es), ...);
        return *this;
    }

    /// @brief Checks if any of the corresponding bits for the provided enum values are on.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if one of the specified bits are on, otherwise `false`.
    /// @details Matches the expression `(bits & mask) != 0`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool onBit(Es... es) const {
        return (onAnyBit(es...));
    }

    /// @brief Checks if any of the corresponding bits for the provided enum values are on.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if one of the specified bits are on, otherwise `false`.
    /// @details Matches the expression `(bits & mask) != 0`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool onAnyBit(Es... es) const {
        return (onBit_(es) || ...);
    }

    /// @brief Checks if all of the corresponding bits for the provided enum values are on.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if all of the specified bits are on, otherwise `false`.
    /// @details Matches the expression `(bits & mask) == mask`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool onAllBit(Es... es) const {
        return (onBit_(es) && ...);
    }

    /// @brief Checks if all of the corresponding bits for the provided enum values are off.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if all of the specified bits are off, otherwise `false`.
    /// @details Matches the expression `(bits & mask) == 0`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool offBit(Es... es) const {
        return offAllBit(es...);
    }

    /// @brief Checks if all of the corresponding bits for the provided enum values are off.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if all of the specified bits are off, otherwise `false`.
    /// @details Matches the expression `(bits & mask) == 0`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool offAllBit(Es... es) const {
        return (offBit_(es) && ...);
    }

    /// @brief Checks if any of the corresponding bits for the provided enum values are off.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if any of the specified bits are off, otherwise `false`.
    /// @details Matches the expression `(bits & mask) != mask`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool offAnyBit(Es... es) const {
        return (offBit_(es) || ...);
    }

    /// @brief Creates an applied mask of the corresponding bits for the provided enum values.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return The applied mask for the specified bits.
    /// @details Matches the expression `(bits & mask)`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr T maskBit(Es... es) const {
        return bits & makeMask(es...);
    }

    /// @brief Creates a mask of the corresponding bits for the provided enum values.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return The mask for the specified bits.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr T makeMask(Es... es) const {
        return (makeMask_(es) | ...);
    }

    /// @brief Sets the bits using a direct mask.
    /// @param mask The bit mask to set.
    /// @return Reference for chaining.
    constexpr TBitFlag<T, E> &set(T mask) {
        bits |= mask;
        return *this;
    }

    /// @brief Resets the bits using a direct mask.
    /// @param mask The bit mask to reset.
    /// @return Reference for chaining.
    constexpr TBitFlag<T, E> &reset(T mask) {
        bits &= ~mask;
        return *this;
    }

    /// @brief Changes the bits using a direct mask.
    /// @param on Determines whether to set or reset the bits.
    /// @param mask The bit mask to change.
    /// @return Reference for chaining.
    constexpr TBitFlag<T, E> &change(bool on, T mask) {
        return on ? set(mask) : reset(mask);
    }

    /// @brief Checks if any bits are on in the specified mask.
    /// @param mask The bit mask to check.
    /// @return `true` if any bits in the mask are on, otherwise `false`.
    [[nodiscard]] constexpr bool on(T mask) const {
        return (bits & mask) != 0;
    }

    /// @brief Checks if all bits are on in the specified mask.
    /// @param mask The bit mask to check.
    /// @return `true` if all bits in the mask are on, otherwise `false`.
    [[nodiscard]] constexpr bool onAll(T mask) const {
        return (bits | mask) == bits;
    }

    /// @brief Checks if all bits are off in the specified mask.
    /// @param mask The bit mask to check.
    /// @return `true` if all bits in the mask are off, otherwise `false`.
    [[nodiscard]] constexpr bool off(T mask) const {
        return (bits & mask) == 0;
    }

    /// @brief Resets all the bits to zero.
    constexpr void makeAllZero() {
        bits = 0;
    }

    /// @brief Gets the current bit mask.
    /// @return The current bit mask.
    [[nodiscard]] constexpr T getDirect() const {
        return bits;
    }

    /// @brief Sets the bits using a direct mask.
    /// @param mask The bit mask to set.
    constexpr void setDirect(T mask) {
        bits = mask;
    }

private:
    /// @brief Type alias for the underlying integer type of the enum `E`.
    typedef std::underlying_type_t<E> EI;

    /// @brief Maximum number of bits that can be represented by the underlying type `T`.
    static constexpr size_t MAX_CAPACITY = std::numeric_limits<T>::digits;

    /// @brief Internal. Sets a specific bit.
    /// @param e Enum value representing the bit to set.
    /// @pre The underlying value of `e` must be less than @ref MAX_CAPACITY.
    constexpr void setBit_(E e) {
        ASSERT(static_cast<EI>(e) < MAX_CAPACITY);
        set(makeMask_(e));
    }

    /// @brief Internal. Resets a specific bit.
    /// @param e Enum value representing the bit to reset.
    /// @pre The underlying value of `e` must be less than @ref MAX_CAPACITY.
    constexpr void resetBit_(E e) {
        ASSERT(static_cast<EI>(e) < MAX_CAPACITY);
        reset(makeMask_(e));
    }

    /// @brief Internal. Changes a specific bit.
    /// @param on Determines whether to set or reset the bit.
    /// @param e Enum value representing the bit to change.
    /// @pre The underlying value of `e` must be less than @ref MAX_CAPACITY.
    constexpr void changeBit_(bool on, E e) {
        ASSERT(static_cast<EI>(e) < MAX_CAPACITY);
        change(on, makeMask_(e));
    }

    /// @brief Internal. Toggles a specific bit.
    /// @param e Enum value representing the bit to change.
    /// @pre The underlying value of `e` must be less than @ref MAX_CAPACITY.
    constexpr void toggleBit_(E e) {
        ASSERT(static_cast<EI>(e) < MAX_CAPACITY);
        changeBit_(offBit_(e), e);
    }

    /// @brief Checks if a specific bit is on.
    /// @param e Enum value representing the bit to change.
    /// @return `true` if the specified bit is on, otherwise `false`.
    /// @pre The underlying value of `e` must be less than @ref MAX_CAPACITY.
    [[nodiscard]] constexpr bool onBit_(E e) const {
        ASSERT(static_cast<EI>(e) < MAX_CAPACITY);
        return on(makeMask_(e));
    }

    /// @brief Checks if a specific bit is off.
    /// @param e Enum value representing the bit to change.
    /// @return `true` if the specified bit is off, otherwise `false`.
    /// @pre The underlying value of `e` must be less than @ref MAX_CAPACITY.
    [[nodiscard]] constexpr bool offBit_(E e) const {
        ASSERT(static_cast<EI>(e) < MAX_CAPACITY);
        return off(makeMask_(e));
    }

    /// @brief Creates a mask for a specific bit.
    /// @param e
    /// @return The mask for the specified bit.
    /// @details Matches the expression `(1 << e)`.
    /// @pre The underlying value of `e` must be less than @ref MAX_CAPACITY.
    [[nodiscard]] constexpr T makeMask_(E e) const {
        ASSERT(static_cast<EI>(e) < MAX_CAPACITY);
        return static_cast<T>(1) << static_cast<T>(e);
    }

    T bits; ///< The bit mask representing the flags.
};

/// @brief Wrapper around a variable length bitfield with an enum corresponding to its bits.
/// @tparam N The number of bits in the bitfield. Must be >= 64 to distinguish from TBitFlag.
/// @tparam E The enum to correspond to.
template <size_t N, typename E>
    requires(std::is_enum_v<E> && N > 64)
class TBitFlagExt {
public:
    /// @brief Default constructor, initializes all flags to off.
    /// @details Matches the expression `bits = 0`.
    constexpr TBitFlagExt() {
        makeAllZero();
    }

    /// @brief Resets all the bits to zero across the entire bitfield array.
    /// @return Reference for chaining.
    constexpr TBitFlagExt<N, E> &makeAllZero() {
        std::fill(bits.begin(), bits.end(), 0);
        return *this;
    }

    /// @brief Sets the corresponding bits for the provided enum values.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to set.
    /// @return Reference for chaining.
    /// @details Matches the expression `bits |= mask`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    constexpr TBitFlagExt<N, E> &setBit(Es... es) {
        (setBit_(es), ...);
        return *this;
    }

    /// @brief Resets the corresponding bits for the provided enum values.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to reset.
    /// @return Reference for chaining.
    /// @details Matches the expression `bits &= ~mask`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    constexpr TBitFlagExt<N, E> &resetBit(Es... es) {
        (resetBit_(es), ...);
        return *this;
    }

    /// @brief Changes the state of the corresponding bits for the provided enum values.
    /// @tparam ...Es Variadic template for packing.
    /// @param on Determines whether to set or reset the bits.
    /// @param ...es Enum values representing the bits to change.
    /// @return Reference for chaining.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    constexpr TBitFlagExt<N, E> &changeBit(bool on, Es... es) {
        (changeBit_(on, es), ...);
        return *this;
    }

    /// @brief Checks if any of the corresponding bits for the provided enum values are on.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if one of the specified bits are on, otherwise `false`.
    /// @details Matches the expression `(bits & mask) != 0`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool onBit(Es... es) const {
        return onAnyBit(es...);
    }

    /// @brief Checks if any of the corresponding bits for the provided enum values are on.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if one of the specified bits are on, otherwise `false`.
    /// @details Matches the expression `(bits & mask) != 0`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool onAnyBit(Es... es) const {
        return (onBit_(es) || ...);
    }

    /// @brief Checks if all of the corresponding bits for the provided enum values are on.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if all of the specified bits are on, otherwise `false`.
    /// @details Matches the expression `(bits & mask) == mask`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool onAllBit(Es... es) const {
        return (onBit_(es) && ...);
    }

    /// @brief Checks if all of the corresponding bits for the provided enum values are off.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if all of the specified bits are off, otherwise `false`.
    /// @details Matches the expression `(bits & mask) == 0`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool offBit(Es... es) const {
        return offAllBit(es...);
    }

    /// @brief Checks if all of the corresponding bits for the provided enum values are off.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if all of the specified bits are off, otherwise `false`.
    /// @details Matches the expression `(bits & mask) == 0`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool offAllBit(Es... es) const {
        return (offBit_(es) && ...);
    }

    /// @brief Checks if any of the corresponding bits for the provided enum values are off.
    /// @tparam ...Es Variadic template for packing.
    /// @param ...es Enum values representing the bits to check.
    /// @return `true` if any of the specified bits are off, otherwise `false`.
    /// @details Matches the expression `(bits & mask) != mask`.
    template <typename... Es>
        requires(std::is_same_v<Es, E> && ...)
    [[nodiscard]] constexpr bool offAnyBit(Es... es) const {
        return (offBit_(es) || ...);
    }

private:
    /// @brief Type alias for the underlying integer type of the enum `E`.
    typedef std::underlying_type_t<E> EI;

    /// @brief Internal. Sets a specific bit.
    /// @param e Enum value representing the bit to set.
    /// @warning Requires that `e` is in the range of `N`.
    constexpr void setBit_(E e) {
        auto [field, mask] = makeMask_(e);
        field |= mask;
    }

    /// @brief Internal. Resets a specific bit.
    /// @param e Enum value representing the bit to set.
    /// @warning Requires that `e` is in the range of `N`.
    constexpr void resetBit_(E e) {
        auto [field, mask] = makeMask_(e);
        field &= ~mask;
    }

    /// @brief Internal. Changes a specific bit.
    /// @param on Determines whether to set or reset the bit.
    /// @param e Enum value representing the bit to change.
    /// @warning Requires that `e` is in the range of `N`.
    constexpr void changeBit_(bool on, E e) {
        on ? setBit_(e) : resetBit_(e);
    }

    /// @brief Checks if a specific bit is on.
    /// @param e Enum value representing the bit to change.
    /// @return `true` if the specified bit is on, otherwise `false`.
    /// @warning Requires that `e` is in the range of `N`.
    [[nodiscard]] constexpr bool onBit_(E e) const {
        auto [field, mask] = makeMask_(e);
        return (field & mask) != 0;
    }

    /// @brief Checks if a specific bit is off.
    /// @param e Enum value representing the bit to change.
    /// @return `true` if the specified bit is off, otherwise `false`.
    /// @warning Requires that `e` is in the range of `N`.
    [[nodiscard]] constexpr bool offBit_(E e) const {
        auto [field, mask] = makeMask_(e);
        return (field & mask) == 0;
    }

    /// @brief Type alias for the underlying storage type of the bit field
    typedef u64 T;

    /// @brief Gets bit index and mask for a specific bit.
    /// @param e Enum value representing the bit to make.
    /// @return The index and mask for the specified bit.
    /// @details Matches the expression `(1 << e)`.
    /// @warning Requires that `e` is in the range of `N`.
    [[nodiscard]] constexpr std::pair<T, T> makeMask_(E e) const {
        EI ei = static_cast<EI>(e);
        ASSERT(ei < N);
        return std::pair<T, T>(bits[ei / C], static_cast<T>(1) << (ei % C));
    }

    /// @brief Gets bit index and mask for a specific bit.
    /// @param e Enum value representing the bit to make.
    /// @return The index and mask for the specified bit.
    /// @details Matches the expression `(1 << e)`.
    /// @warning Requires that `e` is in the range of `N`.
    [[nodiscard]] constexpr std::pair<T &, T> makeMask_(E e) {
        EI ei = static_cast<EI>(e);
        ASSERT(ei < N);
        return std::pair<T &, T>(bits[ei / C], static_cast<T>(1) << (ei % C));
    }

    /// @brief Number of bits in the underlying storage type `T`
    static constexpr size_t C = 8 * sizeof(T);

    std::array<T, (N + C - 1) / C> bits; ///< Underlying storage for the bits
};

} // namespace EGG

} // namespace Kinoko
