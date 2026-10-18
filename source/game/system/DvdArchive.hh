#pragma once

#include <Common.hh>

#include <span>

namespace Kinoko {

namespace EGG {

class Archive;

} // namespace EGG

namespace System {

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

    DvdArchive();
    ~DvdArchive();

    [[nodiscard]] std::span<const u8> getFile(const char *filename) const;
    void load(const char *path, bool decompress_);
    void load(void *fileStart, size_t fileSize, bool decompress_);
    void rip(const char *path);
    void clear();
    void unmount();

    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] bool isRipped() const;

private:
    void move();
    void decompress();
    void mount();
    void clearArchive();
    void clearFile();

    EGG::Archive *m_archive; ///< Pointer to the mounted archive
    void *m_archiveStart;    ///< Pointer to the decompressed archive data in memory
    size_t m_archiveSize;    ///< Size of the decompressed archive data in memory
    void *m_fileStart;       ///< Pointer to the raw file data
    size_t m_fileSize;       ///< Size of the raw file data
    State m_state;           ///< Current state of the archive and file data
};

} // namespace System

} // namespace Kinoko
