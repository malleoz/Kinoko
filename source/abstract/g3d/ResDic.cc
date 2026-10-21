#include "ResDic.hh"

#include <cstring>

namespace Kinoko::Abstract::g3d {

/// @brief Constructor
/// @param data Pointer to the index group data
/// @details Sets @ref m_data to `data`.
ResDic::ResDic(const void *data) : m_data(reinterpret_cast<const Data *>(data)) {}

/// @brief Default destructor
ResDic::~ResDic() = default;

/// @addr{0x8004C050}
/// @brief Lookup a section by name in the `.brres` index group
/// @param name Name of the section to look up
/// @return Pointer to the associated data if found, otherwise `nullptr`
void *ResDic::operator[](const char *name) const {
    if (!m_data || !name) {
        return nullptr;
    }

    const NodeData *node = get(name, std::strlen(name));

    if (node) {
        return reinterpret_cast<void *>(
                reinterpret_cast<uintptr_t>(m_data) + parse<s32>(node->ofsData));
    }

    return nullptr;
}

/// @brief Lookup a section by index in the `.brres` index group
/// @param idx Index of the section to look up
/// @return Pointer to the associated data if found, otherwise `nullptr`
void *ResDic::operator[](size_t idx) const {
    if (!m_data) {
        return nullptr;
    }

    const NodeData *node = &m_data->data[idx + 1];

    return reinterpret_cast<void *>(
            reinterpret_cast<uintptr_t>(m_data) + parse<s32>(node->ofsData));
}

/// @addr{0x8004BF70}
/// @brief Get a node by name in the `.brres` index group
/// @param pName Name of the section to look up
/// @param len Length of the name string
/// @return Pointer to the node if found, otherwise `nullptr`
/// @details Loops so long as the current node's @ref NodeData::ref is greater than the next node's
/// ref. On each iteration, it moves to the next node based on the bit of the character being
/// examined. If the bit is 0, it moves to the left child; if the bit is 1, it moves to the right
/// child. Once the loop ends, it checks if the name of the current node matches the requested name
/// and returns a pointer to the node if it matches.
const ResDic::NodeData *ResDic::get(const char *pName, u32 len) const {
    const NodeData *c = &m_data->data[0];
    const NodeData *x = &m_data->data[parse<u16>(c->idxLeft)];

    while (parse<u16>(c->ref) > parse<u16>(x->ref)) {
        c = x;

        u32 wd = parse<u16>(x->ref) >> 3;
        u32 pos = parse<u16>(x->ref) & 7;

        if (wd < len && (pName[wd] >> pos) & 1) {
            x = &m_data->data[parse<u16>(x->idxRight)];
        } else {
            x = &m_data->data[parse<u16>(x->idxLeft)];
        }
    }

    s32 stringOffset = parse<s32>(x->ofsString);

    if (stringOffset != 0) {
        const char *xName =
                reinterpret_cast<const char *>(reinterpret_cast<uintptr_t>(m_data) + stringOffset);

        if (std::strcmp(pName, xName) == 0) {
            return x;
        }
    }

    return nullptr;
}

} // namespace Kinoko::Abstract::g3d
