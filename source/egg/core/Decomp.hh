#pragma once

#include <Common.hh>

#include <cstring>

namespace Kinoko::EGG::Decomp {

/// @addr{0x8021997C}
/// @brief Gets the expanded size of a YAZ0 compressed buffer
/// @param src Pointer to the YAZ0 compressed buffer
/// @return The expanded size of the buffer, or `-1` if the buffer is not a valid YAZ0 compressed
/// buffer
[[nodiscard]] inline constexpr s32 GetExpandSize(const u8 *src) {
    if (src[0] == 'Y' && src[1] == 'a' && src[2] == 'z') {
        s32 val;
        memcpy(&val, &src[4], sizeof(val));
        return parse<s32>(val);
    }

    return -1;
}

/// @addr{0x80218C2C}
/// @brief Performs YAZ0 decompression on a given buffer
/// @param src Pointer to the YAZ0 compressed buffer
/// @param dst Pointer to the buffer where the decompressed data will be stored
/// @return The size of the decompressed data.
/// @bug In the base game, this function can lead to arbitrary code execution if the decompressed
/// buffer exceeds the size specified by @ref GetExpandSize(). To prevent this buffer overflow, we
/// `PANIC` in Kinoko if the decompressed data exceeds the expected size.
s32 inline constexpr DecodeSZS(const u8 *src, u8 *dst) {
    s32 expandSize = GetExpandSize(src);
    s32 srcIdx = 0x10;
    u8 code = 0;

    u8 byte;

    for (s32 destIdx = 0; destIdx < expandSize; code >>= 1) {
        if (!code) {
            code = 0x80;
            byte = src[srcIdx++];
        }

        // Direct copy (code bit = 1)
        if (byte & code) {
            dst[destIdx++] = src[srcIdx++];
        }
        // RLE compressed data (code bit = 0)
        else {
            // Lower nibble of byte1 + byte2
            s32 distToDest = (src[srcIdx] << 8) | src[srcIdx + 1];
            srcIdx += sizeof(u8) * 2;
            s32 runSrcIdx = destIdx - (distToDest & 0xfff);

            // Upper nibble of byte 1
            s32 runLen = ((distToDest >> 12) == 0) ? src[srcIdx++] + 0x12 : (distToDest >> 12) + 2;

            for (; runLen > 0; runLen--, destIdx++, runSrcIdx++) {
                if (destIdx >= expandSize) {
                    PANIC("Malformed compressed SZS data.");
                }

                dst[destIdx] = dst[runSrcIdx - 1];
            }
        }
    }
    return expandSize;
}

} // namespace Kinoko::EGG::Decomp
