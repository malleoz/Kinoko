#include "DvdArchive.hh"

namespace Kinoko::System {

/// @addr{0x80518CC0}
/// @brief Constructor
DvdArchive::DvdArchive()
    : m_archive(nullptr), m_archiveStart(nullptr), m_archiveSize(0), m_fileStart(nullptr),
      m_fileSize(0), m_state(State::Cleared) {}

/// @addr{0x80518CF4}
/// @brief Destructor that unmounts the archive and clears all associated data from memory
DvdArchive::~DvdArchive() {
    unmount();
}

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
