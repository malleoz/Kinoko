#pragma once

#include <Common.hh>

// Credit: kiwi515/ogws

namespace Kinoko::Abstract::g3d {

/// @brief Essentially a lookup table to find different sections within the resource file.
class ResDic {
public:
    struct NodeData {
        u16 ref;
        u16 flag;
        u16 idxLeft;
        u16 idxRight;
        s32 ofsString;
        s32 ofsData;
    };
    STATIC_ASSERT(sizeof(NodeData) == 0x10);

    struct Data {
        u32 size;
        u32 numData;
        NodeData data[1];
    };

    ResDic(const void *data);
    ~ResDic();

    [[nodiscard]] void *operator[](const char *name) const;
    [[nodiscard]] void *operator[](size_t idx) const;

private:
    [[nodiscard]] const NodeData *get(const char *pName, u32 len) const;

    const Data *m_data;
};

} // namespace Kinoko::Abstract::g3d
