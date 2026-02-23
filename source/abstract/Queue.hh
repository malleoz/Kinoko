#include <array>

namespace Abstract {

/// @brief Aims to implement an interface similar to std::queue but does not involve heap
/// allocation.
/// @details Wraps around an std::array, keeping track of the "current" index that represents the
/// front of the queue, and keeps tracks of how many items are in the queue.
template <typename T, size_t N>
class Queue {
public:
    constexpr Queue() : m_frontIdx(0), m_count(0) {}
    constexpr ~Queue() = default;

    constexpr void push(T &&elem) {
        ASSERT(m_count < N);
        new (&back()) T(std::move(elem));
        ++m_count;
    }

    [[nodiscard]] constexpr T &front() {
        ASSERT(m_count > 0);
        return m_arr[m_frontIdx];
    }

    constexpr void pop() {
        ASSERT(m_count > 0);
        delete (&m_arr[m_frontIdx]);
        m_frontIdx = (m_frontIdx + 1) % N;
        --m_count;
    }

    [[nodiscard]] constexpr size_t size() const {
        return m_count;
    }

    [[nodiscard]] constexpr size_t capacity() const {
        return N;
    }

    [[nodiscard]] constexpr bool empty() const {
        return m_count == 0;
    }

private:
    [[nodiscard]] constexpr T &back() {
        // Compute back based on frontIdx and count
        size_t idx = (m_frontIdx + m_count) % N;
        return m_arr[idx];
    }

    std::array<T, N> m_arr;
    size_t m_frontIdx; ///< Array index that represents the front of the queue
    size_t m_count;    ///< Number of elements in the queue
};

} // namespace Abstract
