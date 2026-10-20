#include "Archive.hh"

#include <cstring>

namespace Kinoko::Abstract {

/// @addr{0x80124500}
/// @brief Constructor
/// @param archiveStart Pointer to the start of the archive in memory
/// @details Initializes @ref m_startAddress to `archiveStart`, @ref m_nodes to the result of
/// @ref initNodes(), @ref m_count to the number of nodes in the archive, and @ref m_strings to
/// point to the string table immediately following the last node.
ArchiveHandle::ArchiveHandle(void *archiveStart)
    : m_startAddress(archiveStart),
      m_nodes(initNodes()),
      m_count(parse<u32>(node(0)->directory.next)),
      m_strings(reinterpret_cast<const char *>(&m_nodes[m_count])) {}

/// @brief Default destructor
ArchiveHandle::~ArchiveHandle() = default;

/// @addr{0x80124894}
/// @brief Maps a given filepath to its corresponding node entry id
/// @param path The filepath to convert to an entry id
/// @return The entry id corresponding to the given filepath, or ``-1 if the path is invalid
/// @details This function traverses the archive's directory structure based on the provided path,
/// handling special cases such as "/", ".", and ".." to navigate the hierarchy correctly. Compares
/// the filename from `path` with the names of the nodes in the archive. To handle directories, it
/// maintains an `anchor` in order to traverse deeper in the hierarchy.
s32 ArchiveHandle::convertPathToEntryId(const char *path) const {
    u32 entryId = 0;

    while (true) {
        // End search
        if (path[0] == '\0') {
            return entryId;
        }

        // Send initial slash to root directory
        if (path[0] == '/') {
            entryId = 0;
            path++;
            continue;
        }

        // Handle special cases
        if (path[0] == '.') {
            if (path[1] == '.') {
                if (path[2] == '/') {
                    entryId = node(entryId)->directory.parent;
                    path += 3;
                    continue;
                } else if (path[2] == '\0') {
                    return node(entryId)->directory.parent;
                } else {
                    // Malformed "..*" case
                    return -1;
                }
            } else if (path[1] == '/') {
                path += 2;
                continue;
            } else if (path[1] == '\0') {
                return entryId;
            }
        }

        // Main search
        const char *nameEnd = path;
        for (; nameEnd[0] != '\0' && nameEnd[0] != '/'; nameEnd++) {}

        bool endOfPath = nameEnd[0] == '\0';
        s32 nameLength = nameEnd - path;

        bool found = false;
        const u32 anchor = entryId++;
        while (entryId < parse<u32>(node(anchor)->directory.next)) {
            if (!node(anchor)->isDirectory() && endOfPath) {
                entryId++;
                continue;
            }

            const char *entryName = m_strings + node(entryId)->stringOffset();

            if (entryName[0] == '.' && entryName[1] == '\0') {
                entryId++;
                continue;
            }

            if (strncmp(path, entryName, nameLength) == 0) {
                found = true;
                break;
            }

            entryId++;
        }

        if (!found) {
            return -1;
        }

        if (endOfPath) {
            return entryId;
        }

        path += nameLength + 1;
    }
}

/// @addr{0x80124844}
/// @brief Gets the file information for the specified node ID
/// @param entryId The index of the node for which to retrieve file information
/// @param info The structure to populate with the file's start offset and length
/// @return `true` if the file information was successfully retrieved, `false` otherwise
/// @details By setting the `info` structure, the caller can obtain the start offset and length of
/// the file data within the archive in order to read the file contents.
bool ArchiveHandle::open(s32 entryId, FileInfo &info) const {
    if (entryId < 0 || static_cast<u32>(entryId) >= m_count) {
        return false;
    }

    auto *node_ = node(entryId);
    if (node_->isDirectory()) {
        return false;
    }

    info.startOffset = parse<u32>(node_->file.startAddress);
    info.length = parse<u32>(node_->file.length);
    return true;
}

/// @addr{0x80124CC0}
/// @brief Gets a pointer to the file data in memory corresponding to the provided `FileInfo`
/// @param info The file information containing the start offset
/// @return A pointer to the start of the file data in memory
const void *ArchiveHandle::getFileAddress(const FileInfo &info) const {
    return static_cast<const u8 *>(m_startAddress) + info.startOffset;
}

/// @brief Fetches the node for the corresponding `entryId`
/// @param entryId The index of the node to fetch
/// @return A pointer to the node corresponding to the specified `entryId` from @ref m_nodes
const ArchiveHandle::Node *ArchiveHandle::node(s32 entryId) const {
    return &m_nodes[entryId];
}

/// @brief Gets a point to the start of the archive in memory
/// @return A pointer to the start of the archive in memory, @ref m_startAddress
const void *ArchiveHandle::startAddress() const {
    return m_startAddress;
}

/// @brief Helper function that gets a pointer to the first node
/// @return A pointer to the first (root) node in the archive
const ArchiveHandle::Node *ArchiveHandle::initNodes() const {
    const auto *rawArchive = reinterpret_cast<const RawArchive *>(m_startAddress);
    ASSERT(rawArchive->isValidSignature());

    return reinterpret_cast<const Node *>(
            static_cast<const u8 *>(m_startAddress) + parse<u32>(rawArchive->nodesOffset));
}

} // namespace Kinoko::Abstract
