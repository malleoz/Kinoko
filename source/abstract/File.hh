#pragma once

#include <Common.hh>

#include <filesystem>

/// @brief Provides file-related utility functions for constructing paths, loading, appending, and
/// removing files
namespace Kinoko::Abstract::File {

[[nodiscard]] std::filesystem::path Path(const char *path);
[[nodiscard]] std::span<const u8> Load(const std::filesystem::path &path);
[[nodiscard]] std::span<const u8> Load(const char *path);
[[nodiscard]] std::span<const u8> LoadHost(const std::filesystem::path &path);
void Append(const char *path, const char *data, size_t size);
int Remove(const char *path);

} // namespace Kinoko::Abstract::File
