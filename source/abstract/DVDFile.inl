#pragma once

#include "DVDFile.hh"

#include <fstream>
#include <type_traits>

namespace Abstract {

template <typename TChar>
    requires std::is_same_v<TChar, char> || std::is_same_v<TChar, wchar_t>
void DVDFile::load_(const TChar *path) {
    ASSERT(path);
    if (ok()) {
        PANIC("File already loaded!");
    }

    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        if constexpr (std::is_same_v<TChar, wchar_t>) {
            WARN("File with path %ls wasn't loaded correctly!", path);
        } else {
            WARN("File with path %s wasn't loaded correctly!", path);
        }
        return;
    }

    file.seekg(0, std::ios::end);
    mSize = file.tellg();
    file.seekg(0, std::ios::beg);

    mData = new u8[mSize];
    file.read(cast<char>(), mSize);
}

} // namespace Abstract
