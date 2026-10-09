#pragma once

#include <abstract/File.hh>

#include <egg/core/Archive.hh>
#include <egg/core/Decomp.hh>

namespace Kinoko::System {

/// @brief Represents an archive loaded from the game disc, such as a `.szs` file or a `.brres` file
class DvdArchive {
public:
    /// @brief Represents the current load state of the archive
    enum class State {
        Cleared = 0,      ///< The file is not loaded
        Ripped = 1,       ///< The file has been found and a pointer obtained
        Decompressed = 2, ///< The file has been decompressed into memory
        Mounted = 3,      ///< The @ref EGG::Archive has been created for the file
    };

    /// @addr{0x80518CC0}
    /// @brief Constructor
    /// @details Initializes @ref m_archive, @ref m_archiveStart, and @ref m_fileStart to `nullptr`.
    /// Initializes @ref m_archiveStart and @ref m_fileSize to zero. Initializes @ref m_state to
    /// @ref State::Cleared.
    DvdArchive()
        : m_archive(nullptr),
          m_archiveStart(nullptr),
          m_archiveSize(0),
          m_fileStart(nullptr),
          m_fileSize(0),
          m_state(State::Cleared) {}

    /// @addr{0x80518CF4}
    /// @brief Destructor that unmounts the archive and clears all associated data from memory
    ~DvdArchive() {
        unmount();
    }

    /// @addr{0x80519508}
    /// @brief Decompresses the file into memory
    /// @pre Requires that the file is compressed.
    /// @details Allocates memory for the decompressed file and decodes the compressed data into it.
    /// Updates @ref m_state to @ref State::Decompressed.
    void decompress() {
        m_archiveSize = EGG::Decomp::GetExpandSize(reinterpret_cast<u8 *>(m_fileStart));
        m_archiveStart = static_cast<u8 *>(EGG::egg_alloc(m_archiveSize));
        EGG::Decomp::DecodeSZS(reinterpret_cast<u8 *>(m_fileStart),
                reinterpret_cast<u8 *>(m_archiveStart));
        m_state = State::Decompressed;
    }

    [[nodiscard]] std::span<const u8> getFile(const char *filename) const;
    void load(const char *path, bool decompress_);
    void load(void *fileStart, size_t fileSize, bool decompress_);

    /// @addr{0x80518DCC}
    /// @brief Mounts the decompressed file as an @ref EGG::Archive
    /// @pre Requires that the file has been loaded into memory.
    /// @details Mounts the decompressed file to @ref m_archive and sets @ref m_state to @ref
    /// State::Mounted.
    void mount() {
        m_archive = EGG::Archive::Mount(m_archiveStart);
        m_state = State::Mounted;
    }

    /// @addr{0x805195A4}
    /// @brief Sets the archive pointer to the file pointer
    /// @pre Requires that the raw file is uncompressed.
    /// @details Sets @ref m_archiveStart to @ref m_fileStart and @ref m_archiveSize to @ref
    /// m_fileSize. Clears @ref m_fileStart and @ref m_fileSize. Updates @ref m_state to @ref
    /// State::Decompressed.
    void move() {
        m_archiveStart = m_fileStart;
        m_archiveSize = m_fileSize;
        m_fileStart = nullptr;
        m_fileSize = 0;
        m_state = State::Decompressed;
    }

    /// @addr{0x805190F0}
    /// @brief Loads the file from the specified path into memory
    /// @param path The path to the file to load from the game disc
    /// @details Loads the file into memory and updates @ref m_fileStart and @ref m_fileSize to
    /// reflect the loaded file. Sets @ref m_state to @ref State::Ripped.
    void rip(const char *path) {
        std::span<const u8> file = Abstract::File::Load(path);
        m_fileStart = const_cast<u8 *>(file.data());
        m_fileSize = file.size();
        if (m_fileSize != 0 && m_fileStart) {
            m_state = State::Ripped;
        }
    }

    /// @addr{0x80519240}
    /// @brief Clears the archive and file data from memory
    void clear() {
        clearArchive();
        clearFile();
    }

    /// @addr{0x80519370}
    /// @brief Clears the archive data from memory
    /// @details Frees the memory allocated for the decompressed archive data and resets @ref
    /// m_archiveStart and @ref m_archiveSize to zero.
    void clearArchive() {
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
    void clearFile() {
        if (!m_fileStart) {
            return;
        }

        EGG::egg_free(static_cast<u8 *>(m_fileStart));
        m_fileStart = nullptr;
        m_fileSize = 0;
    }

    /// @addr{0x805192CC}
    /// @brief Unmounts the archive and clears all associated data from memory
    /// @details If the archive is mounted, it will be unmounted first. Then, all archive and file
    /// data will be cleared from memory, and @ref m_state will be set to @ref State::Cleared.
    void unmount() {
        if (m_state == State::Mounted) {
            m_archive->unmount();
        }
        clear();
        m_state = State::Cleared;
    }

    /// @brief Checks if the archive is loaded into memory
    /// @return `true` if @ref m_state is @ref State::Mounted, `false` otherwise
    [[nodiscard]] bool isLoaded() const {
        return m_state == State::Mounted;
    }

    /// @brief Checks if the file pointer has been loaded
    /// @return `true` if @ref m_state is @ref State::Ripped, `false` otherwise
    [[nodiscard]] bool isRipped() const {
        return m_state == State::Ripped;
    }

private:
    EGG::Archive *m_archive; ///< Pointer to the mounted archive
    void *m_archiveStart;    ///< Pointer to the decompressed archive data in memory
    size_t m_archiveSize;    ///< Size of the decompressed archive data in memory
    void *m_fileStart;       ///< Pointer to the raw file data
    size_t m_fileSize;       ///< Size of the raw file data
    State m_state;           ///< Current state of the archive and file data
};

} // namespace Kinoko::System
