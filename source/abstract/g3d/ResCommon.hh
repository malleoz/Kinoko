#pragma once

// Credit: kiwi515/ogws

namespace Kinoko::Abstract::g3d {

typedef uint16_t u16;
typedef uint32_t u32;

/// @brief The header of a resource block
struct ResBlockHeaderData {
    u32 signature; ///< The signature of the resource block (`CHR0`, `MDL0`, etc.)
    u32 size;      ///< The length of the block
};
STATIC_ASSERT(sizeof(ResBlockHeaderData) == 0x8);

/// @brief The header of a `.brres` resource file
struct ResFileHeaderData {
    u32 signature;  ///< The signature of the resource file (always `bres` in ASCII)
    u16 byteOrder;  ///< Byte order mark (always `0xFEFF`)
    u16 version;    ///< Version of the resource file (usually 0x0000)
    u32 fileSize;   ///< Length of the file in bytes
    u16 headerSize; ///< Size of the header / offset to the root section
    u16 dataBlocks; ///< Number of sections (including the root section)
};
STATIC_ASSERT(sizeof(ResFileHeaderData) == 0x10);

} // namespace Kinoko::Abstract::g3d
