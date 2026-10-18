#pragma once

#include <Types.hh>

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

    void clear();
    void unmount();

    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] u16 rippedArchiveCount() const;

private:
    owning_span<DvdArchive> m_archives; ///< Array of archives managed by this container
    owning_span<void *> m_fileStarts;   ///< Array of pointers to the start of each archive file
    owning_span<size_t> m_fileSizes;    ///< Array of sizes for each archive file
    owning_span<char *> m_suffixes;     ///< Array of filename suffixes for each archive file
    owning_span<Format> m_formats;      ///< Array of formats for each archive file
};

} // namespace Kinoko::System
