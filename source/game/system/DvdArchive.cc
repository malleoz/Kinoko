#include "DvdArchive.hh"

#include <abstract/File.hh>

#include <egg/core/Archive.hh>
#include <egg/core/Decomp.hh>

namespace Kinoko::System {

/// @addr{0x80518CC0}
/// @brief Constructor
/// @details Initializes @ref m_archive, @ref m_archiveStart, and @ref m_fileStart to `nullptr`.
/// Initializes @ref m_archiveStart and @ref m_fileSize to zero. Initializes @ref m_state to
/// @ref State::Cleared.
DvdArchive::DvdArchive()
    : m_archive(nullptr),
      m_archiveStart(nullptr),
      m_archiveSize(0),
      m_fileStart(nullptr),
      m_fileSize(0),
      m_state(State::Cleared) {}

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

    const void *file = m_archive->getFileFast(entryId, fileInfo);
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

/// @addr{0x805190F0}
/// @brief Loads the file from the specified path into memory
/// @param path The path to the file to load from the game disc
/// @details Loads the file into memory and updates @ref m_fileStart and @ref m_fileSize to
/// reflect the loaded file. Sets @ref m_state to @ref State::Ripped.
void DvdArchive::rip(const char *path) {
    std::span<const u8> file = Abstract::File::Load(path);
    m_fileStart = const_cast<u8 *>(file.data());
    m_fileSize = file.size();
    if (m_fileSize != 0 && m_fileStart) {
        m_state = State::Ripped;
    }
}

/// @addr{0x80519240}
/// @brief Clears the archive and file data from memory
void DvdArchive::clear() {
    clearArchive();
    clearFile();
}

/// @addr{0x805192CC}
/// @brief Unmounts the archive and clears all associated data from memory
/// @details If the archive is mounted, it will be unmounted first. Then, all archive and file
/// data will be cleared from memory, and @ref m_state will be set to @ref State::Cleared.
void DvdArchive::unmount() {
    if (m_state == State::Mounted) {
        m_archive->unmount();
    }
    clear();
    m_state = State::Cleared;
}

/// @brief Checks if the archive is loaded into memory
/// @return `true` if @ref m_state is @ref State::Mounted, `false` otherwise
bool DvdArchive::isLoaded() const {
    return m_state == State::Mounted;
}

/// @brief Checks if the file pointer has been loaded
/// @return `true` if @ref m_state is @ref State::Ripped, `false` otherwise
bool DvdArchive::isRipped() const {
    return m_state == State::Ripped;
}

/// @addr{0x805195A4}
/// @brief Sets the archive pointer to the file pointer
/// @pre Requires that the raw file is uncompressed.
/// @details Sets @ref m_archiveStart to @ref m_fileStart and @ref m_archiveSize to @ref
/// m_fileSize. Clears @ref m_fileStart and @ref m_fileSize. Updates @ref m_state to @ref
/// State::Decompressed.
void DvdArchive::move() {
    m_archiveStart = m_fileStart;
    m_archiveSize = m_fileSize;
    m_fileStart = nullptr;
    m_fileSize = 0;
    m_state = State::Decompressed;
}

/// @addr{0x80519508}
/// @brief Decompresses the file into memory
/// @pre Requires that the file is compressed.
/// @details Allocates memory for the decompressed file and decodes the compressed data into it.
/// Updates @ref m_state to @ref State::Decompressed.
void DvdArchive::decompress() {
    m_archiveSize = EGG::Decomp::GetExpandSize(reinterpret_cast<u8 *>(m_fileStart));
    m_archiveStart = static_cast<u8 *>(EGG::egg_alloc(m_archiveSize));
    EGG::Decomp::DecodeSZS(reinterpret_cast<u8 *>(m_fileStart),
            reinterpret_cast<u8 *>(m_archiveStart));
    m_state = State::Decompressed;
}

/// @addr{0x80518DCC}
/// @brief Mounts the decompressed file as an @ref EGG::Archive
/// @pre Requires that the file has been loaded into memory.
/// @details Mounts the decompressed file to @ref m_archive and sets @ref m_state to @ref
/// State::Mounted.
void DvdArchive::mount() {
    m_archive = EGG::Archive::Mount(m_archiveStart);
    m_state = State::Mounted;
}

/// @addr{0x80519370}
/// @brief Clears the archive data from memory
/// @details Frees the memory allocated for the decompressed archive data and resets @ref
/// m_archiveStart and @ref m_archiveSize to zero.
void DvdArchive::clearArchive() {
    if (!m_archiveStart) {
        return;
    }

    EGG::egg_free(static_cast<u8 *>(m_archiveStart));
    m_archiveStart = nullptr;
    m_archiveSize = 0;
}

/// @addr{0x805193C8}
/// @brief Clears the file data from memory
/// @details Frees the memory allocated for the raw file data and resets @ref m_fileStart and
/// @ref m_fileSize to zero.
void DvdArchive::clearFile() {
    if (!m_fileStart) {
        return;
    }

    EGG::egg_free(static_cast<u8 *>(m_fileStart));
    m_fileStart = nullptr;
    m_fileSize = 0;
}

} // namespace Kinoko::System
