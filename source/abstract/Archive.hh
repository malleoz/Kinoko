#pragma once

#include <Common.hh>

/// @brief An abstraction of components from the nw4r and RVL libraries.
namespace Kinoko::Abstract {

/// @brief Represents a handle to an archive, providing access to its nodes and files
/// @details Each node in the archive represents either a directory or a file, and contains metadata
/// such as its parent, next sibling, and file-specific information.
class ArchiveHandle {
public:
    /// @brief Represents the header information of the archive file
    struct RawArchive {
        u32 signature;   ///< Always 0x55AA382D
        u32 nodesOffset; ///< Offset in the archive to the first node
        u32 nodesSize;   ///< Size of the nodes and string table sections in the archive
        u32 filesOffset; ///< Offset in the archive to the first file

        /// @brief Checks that the archive has a valid signature in the header
        /// @return `true` if the archive has a valid signature, `false` otherwise
        [[nodiscard]] bool isValidSignature() const {
            constexpr u32 ARC_SIGNATURE = 0x55AA382D;

            return parse<u32>(signature) == ARC_SIGNATURE;
        }
    };

    /// @brief Represents a node in the archive, which can be either a directory or a file
    /// @details The first node in an archive is always the root node. The root node's
    /// `directory.next` field represents the number of nodes in the archive.
    struct Node {
        /// @brief Union of the first 4 bytes of the node
        /// @details The first byte represents the node type (0 = file, 1 = directory) and the
        /// remaining three bytes represent the string offset for the node's name.
        union {
            u32 val;   ///< The raw 4-byte value representing the node type and string offset
            u8 str[4]; ///< The raw 4-byte value as an array of bytes
        };

        /// @brief Union representing either a directory or a file within the archive node
        union {
            struct {
                u32 parent; ///< Index of the parent directory
                u32 next;   ///< Index of the next node that is not part of this directory
            } directory;
            struct {
                u32 startAddress; ///< Offset to the beginning of file data
                u32 length;       ///< Length of the file data
            } file;
        };

        /// @brief Checks whether this node represents a directory
        /// @return `true` if the node is a directory, `false` if it represents a file
        [[nodiscard]] bool isDirectory() const {
            return !!(str[0]);
        }

        /// @brief Retrieves the string offset for the node's name
        /// @return The offset in the string table where the node's name is stored
        [[nodiscard]] u32 stringOffset() const {
            return parse<u32>(val) & 0xFFFFFF;
        }
    };
    STATIC_ASSERT(sizeof(Node) == 0xC);

    /// @brief Represents info about a file within the archive
    struct FileInfo {
        u32 startOffset; ///< Offset to the beginning of file data
        u32 length;      ///< Length of the file data
    };

    ArchiveHandle(void *archiveStart);
    ~ArchiveHandle();

    [[nodiscard]] s32 convertPathToEntryId(const char *path) const;
    bool open(s32 entryId, FileInfo &info) const;

    [[nodiscard]] const void *getFileAddress(const FileInfo &info) const;
    [[nodiscard]] const Node *node(s32 entryId) const;
    [[nodiscard]] const void *startAddress() const;

private:
    [[nodiscard]] const Node *initNodes() const;

    const void *const m_startAddress; ///< Pointer to the start of the archive in memory
    const Node *const m_nodes;        ///< Pointer to the array of nodes in the archive
    const u32 m_count;                ///< Number of nodes in the archive
    const char *const m_strings;      ///< Pointer to the string table containing node names
};

} // namespace Kinoko::Abstract
