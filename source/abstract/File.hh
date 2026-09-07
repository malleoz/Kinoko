#pragma once

#include <Common.hh>

#include <filesystem>

namespace Kinoko::Abstract::File {

[[nodiscard]] std::filesystem::path Path(const char *path);
[[nodiscard]] std::span<const u8> Load(const std::filesystem::path &path);
[[nodiscard]] std::span<const u8> Load(const char *path);
void Append(const char *path, const char *data, size_t size);
int Remove(const char *path);

} // namespace Kinoko::Abstract::File
