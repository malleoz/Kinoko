#pragma once

#include <Common.hh>

// Credit: kiwi515/ogws

namespace Kinoko::Abstract::g3d {

/// @brief Essentially a lookup table to find different sections within a `.brres` resource file.
/// @details The dictionary consists of a radix tree of nodes, each containing an offset to a
/// name string and associated data. The nodes are used to efficiently look up sections within the
/// resource file by name.
class ResDic {
public:
    /** @brief A node in the radix tree of the dictionary
     * @details The node's @ref ref represents one bit in the name being looked up. @ref
     * ResDic::get() splits it as follows:
     * @code{.unparsed}
     *  15                           3 2     0
     * ┌──────────────────────────────┬───────┐
     * │      byte index (ref >> 3)   │  bit  │
     * └──────────────────────────────┴───────┘
     *                                (ref & 7)
     * @endcode
     * - The byte index `(ref >> 3)` indicates which character of the name is being examined.
     * - The bit `(ref & 7)` indicates which bit within that character is being examined.
     *
     * @ref idxLeft and @ref idxRight are indices into the @ref Data::data array of nodes. These are
     * not strictly child nodes, as radix trees can point back up the tree. A link to a node whose
     * @ref ref is greater or equal to the current node is a link back to the ancestor or the node
     * itself. There is a special case for the sentinel node, whose @ref idxLeft points to the real
     * root node of the tree.
     *
     * When traversing down a tree, the algorithm follows the left or right child based on the bit
     * of the character being examined. If the bit is 0, it goes to @ref idxLeft; if the bit is 1,
     * it goes to @ref idxRight.
     **/
    struct NodeData {
        u16 ref;      ///< Bit index for traversing the tree of nodes
        u16 flag;     ///< Unused, always zero
        u16 idxLeft;  ///< Index of the left child node (or the parent node if this is a back link)
        u16 idxRight; ///< Index of the right child node (or the parent node if this is a back link)
        s32 ofsString; ///< Offset to the name string
        s32 ofsData;   ///< Offset to the associated data
    };
    STATIC_ASSERT(sizeof(NodeData) == 0x10);

    /// @brief The header of the `.brres` index group
    struct Data {
        u32 size;         ///< Length of the index group in bytes
        u32 numData;      ///< Number of data entries in the index group
        NodeData data[1]; ///< Array of node data entries
    };

    ResDic(const void *data);
    ~ResDic();

    [[nodiscard]] void *operator[](const char *name) const;
    [[nodiscard]] void *operator[](size_t idx) const;

private:
    [[nodiscard]] const NodeData *get(const char *pName, u32 len) const;

    const Data *const m_data; ///< Pointer to the index group data
};

} // namespace Kinoko::Abstract::g3d
