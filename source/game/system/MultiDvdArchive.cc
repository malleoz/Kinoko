#include "MultiDvdArchive.hh"

#include <cstring>
#include <ranges>

namespace Kinoko::System {

static size_t SUFFIX_SIZE = 8;

/// @addr{0x8052A538}
/// @brief Constructor
/// @param archiveCount The number of sub-archives to manage
/// @details Allocates arrays for the sub-archives and the file starts, file sizes, suffixes, and
/// formats for each sub-archive.
MultiDvdArchive::MultiDvdArchive(u16 archiveCount)
    : m_archives(archiveCount),
      m_fileStarts(archiveCount),
      m_fileSizes(archiveCount),
      m_suffixes(archiveCount),
      m_formats(archiveCount) {
    for (u16 i = 0; i < m_archives.size(); i++) {
        m_fileStarts[i] = nullptr;
        m_fileSizes[i] = 0;
        m_suffixes[i] = static_cast<char *>(EGG::egg_alloc(SUFFIX_SIZE));
        strncpy(m_suffixes[i], SZS_EXTENSION, SUFFIX_SIZE);
        m_formats[i] = Format::Double;
    }
}

/// @addr{0x8052A6DC}
/// @brief Default destructor
MultiDvdArchive::~MultiDvdArchive() = default;

/// @addr{0x8052A760}
/// @brief Retrieves a file from the sub-archives
/// @param filename The name of the file to retrieve
/// @return A span over the file data if found, or an empty span otherwise
/// @details This function iterates over the sub-archives in reverse order, so that a resource which
/// is present in the course archive takes precedence over the resource present in `Common.szs`.
std::span<const u8> MultiDvdArchive::getFile(const char *filename) const {
    for (const auto &archive : std::views::reverse(m_archives)) {
        if (!archive.isLoaded()) {
            continue;
        }

        std::span<const u8> file = archive.getFile(filename);
        if (!file.empty()) {
            return file;
        }
    }

    return {};
}

/// @addr{0x8052A954}
/// @brief Loads a sub-archive specified by the provided filename
/// @param filename The name of the sub-archive to load
void MultiDvdArchive::load(const char *filename) {
    char buffer[256];

    for (auto [i, archive] : std::views::enumerate(m_archives)) {
        switch (m_formats[i]) {
        case Format::Double:
            snprintf(buffer, sizeof(buffer), "%s%s", filename, m_suffixes[i]);
            break;
        case Format::Single:
            snprintf(buffer, sizeof(buffer), "%s", filename);
            break;
        case Format::None:
            break;
        default:
            continue;
        }

        if (m_formats[i] == Format::None) {
            archive.load(m_fileStarts[i], m_fileSizes[i], true);
        } else {
            archive.load(buffer, true);
        }
    }
}

/// @addr{0x8052AB6C}
/// @brief Loads a sub-archive specified by the provided filename into memory
/// @param filename The name of the sub-archive to rip
void MultiDvdArchive::rip(const char *filename) {
    char buffer[256];

    for (auto [i, archive] : std::views::enumerate(m_archives)) {
        switch (m_formats[i]) {
        case Format::Double:
            snprintf(buffer, sizeof(buffer), "%s%s", filename, m_suffixes[i]);
            break;
        case Format::Single:
            snprintf(buffer, sizeof(buffer), "%s", filename);
            break;
        case Format::None:
            break;
        default:
            continue;
        }
        archive.rip(buffer);
    }
}

} // namespace Kinoko::System
