#pragma once

#include "abstract/DVDFile.hh"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <generator>
#include <optional>
#include <span>

/* ================================================================ *
        High-level system namespace for loading files.
 * ================================================================ */
namespace Abstract::Filesystem {

/* ================================================================ *
 Typedef which represents a function that the caller specifies which
 can be used to filter files based on file path, name, or extension.
* ================================================================ */
using FilenameFilter = std::optional<std::function<bool(const std::filesystem::path &path)>>;

/* ================================================================ *
     Typedef which represents a function that the caller specifies which
     can be used to filter files based on file content.
 * ================================================================ */
using FiledataFilter = std::optional<std::function<bool(std::span<std::byte>)>>;

/* ================================================================ *
        Initializes the system path.
        Implementation is platform-specific.
 * ================================================================ */
void initialize(void);

/* ================================================================ *
        Loads a file from the user-provided path.
 * ================================================================ */
DVDFile loadFile(const char *path);
DVDFile loadFile(const wchar_t *path);

/* ================================================================ *
        Loads a system file from the executable path.
 * ================================================================ */
DVDFile loadSystemFile(const char *path);

std::generator<DVDFile> iterate(const char *path, bool recursive, FilenameFilter pathFilter = std::nullopt,
        FiledataFilter dataFilter = std::nullopt);

}; // namespace Abstract::Filesystem
