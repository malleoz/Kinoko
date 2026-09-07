#include "File.hh"

#include <egg/core/Heap.hh>

#include <fstream>

namespace Kinoko::Abstract::File {

std::filesystem::path Path(const char *path) {
    char filepath[256];

    if (path[0] == '/') {
        path++;
    }

    snprintf(filepath, sizeof(filepath), "%s", path);
    return std::filesystem::path(filepath);
}

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

std::span<const u8> Load(const char *path) {
    return Load(Path(path));
}

void Append(const char *path, const char *data, size_t size) {
    std::ofstream stream;
    stream.open(path, std::ios::app | std::ios::binary);
    stream.write(data, size);
}

int Remove(const char *path) {
    return std::remove(path);
}

} // namespace Kinoko::Abstract::File
