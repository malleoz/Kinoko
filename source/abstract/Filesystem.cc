#include "Filesystem.hh"

// TODO: Apple (how do we test Apple?)
#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace Abstract::Filesystem {

/* ================================================================ *
        Alias for the underlying path character type.
 * ================================================================ */
using CharType = std::filesystem::path::value_type;

/* ================================================================ *
            Path relative to the executable.
       ----------------------------------------------------------------
            WARNING: Do not make assumptions about char/wchar_t!!
            WARNING: Heap allocation on initialization!
     * ================================================================ */
std::filesystem::path sSystemPath;

/* ================================================================ *
        Checks if the filesystem is initialized.
 * ================================================================ */
static bool ok(void) {
    return !sSystemPath.empty();
}

void initialize(void) {
    ASSERT(!ok());

#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0) {
        PANIC("Malformed system path!!!");
    }
#elif defined(__linux__)
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len < 0) {
        PANIC("Malformed system path!!!");
    }
    buffer[len] = '\0';
#endif
    sSystemPath = std::filesystem::path(buffer).parent_path();
}

template <typename TChar>
    requires std::is_same_v<TChar, char> || std::is_same_v<TChar, wchar_t>
static DVDFile loadFile_(const TChar *path) {
    ASSERT(path);

    if (!std::filesystem::exists(path)) {
        if constexpr (std::is_same_v<TChar, wchar_t>) {
            WARN("Requested file %ls doesn't exist", path);
        } else {
            WARN("Requested file %s doesn't exist", path);
        }
        return DVDFile();
    }

    if (!std::filesystem::is_regular_file(path)) {
        if constexpr (std::is_same_v<TChar, wchar_t>) {
            WARN("Requested path %ls is not a file", path);
        } else {
            WARN("Requested path %s is not a file", path);
        }
        return DVDFile();
    }

    return DVDFile(path);
}

DVDFile loadFile(const char *path) {
    return loadFile_(path);
}

DVDFile loadFile(const wchar_t *path) {
    return loadFile_(path);
}

/* ================================================================ *
        See Filesystem.inl for Filesystem::loadFile_( ... ).
 * ================================================================ */

DVDFile loadSystemFile(const char *path) {
    // TODO: Scope lock flipped alignment for STL new

    ASSERT(path);

    // System files should be indicated by a root path identifier
    // Assume system filepaths are narrow for now
    ASSERT(path[0] == '/');
    ASSERT(ok());

    std::filesystem::path filepath = sSystemPath / ++path;
    if (!std::filesystem::exists(filepath)) {
        PANIC("System file \"%s\" missing!!", path);
    }

    return DVDFile(filepath.native().data());
}

template <typename TChar>
    requires std::is_same_v<TChar, char> || std::is_same_v<TChar, wchar_t>
static std::generator<DVDFile> iterate_(const TChar *path, FilenameFilter pathFilter,
        FiledataFilter dataFilter) {
    if (!std::filesystem::exists(path)) {
        if constexpr (std::is_same_v<TChar, wchar_t>) {
            WARN("Requested directory %ls doesn't exist", path);
        } else {
            WARN("Requested directory %s doesn't exist", path);
        }
        co_return;
    }

    if (!std::filesystem::is_directory(path)) {
        if constexpr (std::is_same_v<TChar, wchar_t>) {
            WARN("Requested path %ls is not a directory", path);
        } else {
            WARN("Requested path %s is not a directory", path);
        }
        co_return;
    }

    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        if (!std::filesystem::is_regular_file(entry.path())) {
            continue;
        }

        if (pathFilter) {
            if (!(*pathFilter)(entry.path())) {
                REPORT("Filtering out %s", entry.path().string().c_str());
                continue;
            }
        }

        auto file = DVDFile(entry.path().native().data());

        if (dataFilter) {
            if (!(*dataFilter)(
                        std::span<std::byte>(static_cast<std::byte *>(file.data()), file.size()))) {
                REPORT("Data Filtering out %s", entry.path().string().c_str());
                continue;
            }
        }

        co_yield std::move(file);
    }
}

std::generator<DVDFile> iterate(const char *path, FilenameFilter pathFilter,
        FiledataFilter dataFilter) {
    return iterate_(path, pathFilter, dataFilter);
}

} // namespace Abstract::Filesystem
