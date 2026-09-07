#pragma once

#include "game/system/DvdArchive.hh"

const char *const SZS_EXTENSION = ".szs";

namespace Kinoko::System {

/// @brief A container that manages multiple @ref DvdArchive instances together so that the game can
/// access them as a single logical archive
/// @details In practice, the game uses this class to allow access to both `Common.szs` and the
/// course `.szs` files and their underlying resources through the same interface.
class MultiDvdArchive {
public:
    /// @todo Investigate the significance of the different formats
    enum class Format {
        Double,
        Single,
        None,
    };

    MultiDvdArchive(u16 archiveCount = 1);
    ~MultiDvdArchive();

    [[nodiscard]] std::span<const u8> getFile(const char *filename) const;
    void load(const char *filename);
    void rip(const char *filename);

    /// @addr{0x8052AC40}
    /// @brief Clears all archives and their file data from memory
    void clear() {
        for (auto &archive : m_archives) {
            archive.clear();
        }
    }

    /// @addr{0x8052AA88}
    /// @brief Unmounts all sub-archives and clears all associated data from memory
    void unmount() {
        for (auto &archive : m_archives) {
            archive.unmount();
        }
    }

    /// @addr{0x8052A800}
    /// @brief Checks if any of the sub-archives are loaded
    /// @return `true` if at least one sub-archive is loaded, `false` otherwise
    [[nodiscard]] bool isLoaded() const {
        for (const auto &archive : m_archives) {
            if (archive.isLoaded()) {
                return true;
            }
        }

        return false;
    }

    /// @addr{0x8052AE08}
    /// @brief Returns the number of sub-archives that have been ripped
    /// @return The count of ripped sub-archives
    [[nodiscard]] u16 rippedArchiveCount() const {
        u16 count = 0;
        for (const auto &archive : m_archives) {
            if (archive.isRipped()) {
                count++;
            }
        }

        return count;
    }

private:
    owning_span<DvdArchive> m_archives; ///< Array of archives managed by this container
    owning_span<void *> m_fileStarts;   ///< Array of pointers to the start of each archive file
    owning_span<size_t> m_fileSizes;    ///< Array of sizes for each archive file
    owning_span<char *> m_suffixes;     ///< Array of filename suffixes for each archive file
    owning_span<Format> m_formats;      ///< Array of formats for each archive file
};

} // namespace Kinoko::System
