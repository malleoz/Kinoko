#include "File.hh"

#include <egg/core/Heap.hh>

#include <cstdlib>
#include <fstream>

namespace Kinoko::Abstract::File {

/// @brief Constructs a `std::filesystem::path` from the provided C-style string path
/// @param path The C-style string path
/// @return A `std::filesystem::path` object representing the constructed path
/// @details Ignores a leading slash, if it exists.
std::filesystem::path Path(const char *path) {
    char filepath[256];

    if (path[0] == '/') {
        path++;
    }

    snprintf(filepath, sizeof(filepath), "%s", path);
    return std::filesystem::path(filepath);
}

/// @brief Loads a file from the specified filesystem path into memory
/// @param path The filesystem path to the file to load
/// @return A span containing the contents of the loaded file
/// @warning Panics if the file cannot be opened or read correctly.
std::span<const u8> Load(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        PANIC("File with provided path %s was not loaded correctly!", path.string().c_str());
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    u8 *buffer = static_cast<u8 *>(EGG::egg_alloc(size, 4));
    file.read(reinterpret_cast<char *>(buffer), size);

    return std::span<const u8>(buffer, size);
}

/// @brief Loads a game file from the specified path into memory
/// @param path Path to the game file to load, such as `/Race/Course/castle_course.szs`
/// @return A span containing the contents of the loaded file
/// @details If the `KINOKO_FILESYSTEM_ROOT` environment variable is set, the path is relative to
/// it. Otherwise, the path is relative to the current working directory.
/// @warning Panics if the file cannot be opened or read correctly.
std::span<const u8> Load(const char *path) {
    std::filesystem::path filepath = Path(path);

    const char *root = std::getenv("KINOKO_FILESYSTEM_ROOT");
    if (root && root[0] != '\0') {
        filepath = std::filesystem::path(root) / filepath;
    }

    return Load(filepath);
}

/// @brief Loads a file from the host system into memory, outside of the game's heaps
/// @param path The filesystem path to the file to load
/// @return A span containing the contents of the loaded file. The caller is responsible for
/// freeing the buffer via `delete[]`.
/// @details This is used for files that are not part of the game, such as ghosts and test cases.
/// Since they are not allocated from the game's heaps, they persist across scene changes and are
/// excluded from @ref Host::Context copies.
/// @warning Panics if the file cannot be opened or read correctly.
std::span<const u8> LoadHost(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        PANIC("File with provided path %s was not loaded correctly!", path.string().c_str());
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    u8 *buffer = new u8[size];
    file.read(reinterpret_cast<char *>(buffer), size);

    return std::span<const u8>(buffer, size);
}

/// @brief Writes data to the end of the file at the specified path
/// @param path Path to the file to which data will be appended
/// @param data Pointer to the buffer to write
/// @param size Number of bytes to write from the buffer
void Append(const char *path, const char *data, size_t size) {
    std::ofstream stream;
    stream.open(path, std::ios::app | std::ios::binary);
    stream.write(data, size);
}

/// @brief Deletes the file at the specified path from the user's filesystem
/// @param path The path to the file to delete
/// @return `0` if the file was successfully deleted, or a non-zero value if an error occurred
int Remove(const char *path) {
    return std::remove(path);
}

} // namespace Kinoko::Abstract::File
