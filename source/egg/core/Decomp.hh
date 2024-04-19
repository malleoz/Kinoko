#pragma once

#include <Common.hh>

namespace EGG::Decomp {

s32 GetExpandSize(const u8 *src);

/// @brief Performs YAZ0 decompression on a given buffer.
/// @return The size of the decompressed data.
s32 DecodeSZS(const u8 *src, u8 *dst);

} // namespace EGG::Decomp
