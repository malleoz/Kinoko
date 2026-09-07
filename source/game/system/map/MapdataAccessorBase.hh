#pragma once

#include <Common.hh>

#include <egg/core/Heap.hh>

namespace Kinoko::System {

/// @brief The header for a section in the course KCL file
struct MapSectionHeader {
    s32 sectionName; ///< The signature identifying the section
    u16 count;       ///< The number of entries in the section
};

/// @brief Base class for accessing entries in a section of the course KMP file
/// @tparam T The type of the entries in the section
/// @tparam TData The type of the raw data for each entry
template <typename T, typename TData>
class MapdataAccessorBase {
public:
    /// @brief Constructor
    /// @param header Pointer to the section header for the section being accessed
    MapdataAccessorBase(const MapSectionHeader *header) : m_sectionHeader(header) {}

    /// @brief Deleted copy constructor
    MapdataAccessorBase(const MapdataAccessorBase &) = delete;

    /// @brief Deleted move constructor
    MapdataAccessorBase(MapdataAccessorBase &&) = delete;

    /// @brief Deleted copy assignment operator
    MapdataAccessorBase &operator=(const MapdataAccessorBase &) = delete;

    /// @brief Deleted move assignment operator
    MapdataAccessorBase &operator=(MapdataAccessorBase &&) = delete;

    /// @brief Virtual destructor which deletes all entries in the section
    virtual ~MapdataAccessorBase() {
        if (m_entries.initialized()) {
            for (auto *&entry : m_entries) {
                EGG::egg_delete(entry);
            }
        }
    }

    /// @brief Returns the entry at the specified index, or nullptr if the index is out of bounds
    /// @param i The index of the entry to retrieve
    /// @return A pointer to the entry at the specified index, or nullptr if the index is out of
    /// bounds
    [[nodiscard]] T *get(u16 i) const {
        return i < m_entries.size() ? m_entries[i] : nullptr;
    }

    /// @brief Returns the raw data for the entry at the specified index, or nullptr if the index is
    /// out of bounds
    /// @param i The index of the entry to retrieve
    /// @return A pointer to the raw data for the entry at the specified index, or nullptr if the
    /// index is out of bounds
    [[nodiscard]] TData *getData(u16 i) const {
        return i < m_entries.size() ? m_entries[i]->data() : nullptr;
    }

    /// @brief Returns the number of entries in the section
    /// @return The number of entries in the section
    [[nodiscard]] u16 size() const {
        return m_entries.size();
    }

    /// @brief Returns whether the section has no entries
    /// @return True if the section has no entries, false otherwise
    [[nodiscard]] bool empty() const {
        return m_entries.empty();
    }

    /// @brief Initializes the entries in the section with the given raw data
    /// @param start A pointer to the raw data for the entries
    /// @param count The number of entries to initialize
    void init(const TData *start, u16 count) {
        if (count != 0) {
            m_entries.reserve(count);
        }

        for (u16 i = 0; i < count; ++i) {
            m_entries.push_back(EGG::egg_new<T>(&start[i]));
        }
    }

protected:
    fixed_vector<T *> m_entries;             ///< Array of entries in the section
    const MapSectionHeader *m_sectionHeader; ///< Pointer to the section header
};

} // namespace Kinoko::System
