#pragma once

#include <Common.hh>

#include <egg/core/Heap.hh>

namespace Kinoko::System {

struct MapSectionHeader {
    s32 magic;
    u16 count;
};

template <typename T, typename TData>
class MapdataAccessorBase {
public:
    MapdataAccessorBase(const MapSectionHeader *header) : m_sectionHeader(header) {}
    MapdataAccessorBase(const MapdataAccessorBase &) = delete;
    MapdataAccessorBase(MapdataAccessorBase &&) = delete;

    virtual ~MapdataAccessorBase() {
        if (m_entries.initialized()) {
            for (auto *&entry : m_entries) {
                EGG::egg_delete(entry);
            }
        }
    }

    [[nodiscard]] T *get(u16 i) const {
        return i < m_entries.size() ? m_entries[i] : nullptr;
    }

    [[nodiscard]] TData *getData(u16 i) const {
        return i < m_entries.size() ? m_entries[i]->data() : nullptr;
    }

    [[nodiscard]] u16 size() const {
        return m_entries.size();
    }

    [[nodiscard]] bool empty() const {
        return m_entries.empty();
    }

    void init(const TData *start, u16 count) {
        if (count != 0) {
            m_entries.reserve(count);
        }

        for (u16 i = 0; i < count; ++i) {
            m_entries.push_back(EGG::egg_new<T>(&start[i]));
        }
    }

protected:
    fixed_vector<T *> m_entries;
    const MapSectionHeader *m_sectionHeader;
};

} // namespace Kinoko::System
