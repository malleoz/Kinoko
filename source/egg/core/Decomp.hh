#pragma once

#include <Common.hh>

namespace Kinoko::EGG::Decomp {

/// @addr{0x802198F8}
[[nodiscard]] inline constexpr bool IsCompressed(const u8 *src) {
    return src[0] == 'Y' && src[1] == 'a' && src[2] == 'z';
}

/// @addr{0x8021997C}
[[nodiscard]] inline constexpr s32 GetExpandSize(const u8 *src) {
    return IsCompressed(src) ? form<s32>(&src[4]) : -1;
}

s32 DecodeSZS(const u8 *src, u8 *dst);

} // namespace Kinoko::EGG::Decomp
