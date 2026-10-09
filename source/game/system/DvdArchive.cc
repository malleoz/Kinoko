#include "DvdArchive.hh"

namespace Kinoko::System {

/// @addr{0x80519420}
/// @brief Retrieves a file from the mounted archive
/// @param filename The name of the file to retrieve
/// @return A span over the file data, or an empty span if the file is not found or the archive is
/// not mounted
std::span<const u8> DvdArchive::getFile(const char *filename) const {
    if (m_state != State::Mounted) {
        return {};
    }

    char buffer[256];
    if (filename[0] == '/') {
        snprintf(buffer, sizeof(buffer), "%s", filename);
    } else {
        snprintf(buffer, sizeof(buffer), "/%s", filename);
    }
    buffer[sizeof(buffer) - 1] = '\0';

    Abstract::ArchiveHandle::FileInfo fileInfo{0, 0};
    s32 entryId = m_archive->convertPathToEntryId(buffer);
    if (entryId == -1) {
        return {};
    }

    void *file = m_archive->getFileFast(entryId, fileInfo);
    if (!file) {
        return {};
    }

    return {static_cast<const u8 *>(file), fileInfo.length};
}

/// @addr{0x80518E10}
/// @brief Decompresses and mounts the archive at the provided path
/// @param path The path to the archive file
/// @param decompress_ Whether to decompress the file data
/// @details Loads the file from the specified `path` into memory. If successful, checks to see if
/// the file is compressed and calls @ref decompress(), otherwise calls @ref move(). Finally, mounts
/// the archive via @ref mount().
void DvdArchive::load(const char *path, bool decompress_) {
    if (m_state == State::Cleared) {
        rip(path);
    }

    if (m_state == State::Ripped) {
        if (decompress_) {
            decompress();
            clearFile();
        } else {
            move();
        }
        mount();
    }
}

/// @addr{0x80518FBC}
/// @brief Loads the archive from the provided memory buffer
/// @param fileStart Pointer to the start of the file data in memory
/// @param fileSize Size of the file data in memory
/// @param decompress_ Whether to decompress the file data
/// @details Updates @ref m_fileStart to point to the provided `fileStart` and @ref m_fileSize to
/// the provided `fileSize`. If `decompress_` is `true`, decompresses the file via @ref decompress()
/// and clears @ref m_fileStart and @ref m_fileSize. Finally, mounts the archive via @ref mount().
void DvdArchive::load(void *fileStart, size_t fileSize, bool decompress_) {
    m_fileStart = fileStart;
    m_fileSize = fileSize;
    if (decompress_) {
        decompress();
        m_fileStart = nullptr;
        m_fileSize = 0;
    }
    mount();
}

} // namespace Kinoko::System
