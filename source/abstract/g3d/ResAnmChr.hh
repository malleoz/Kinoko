#pragma once

#include "abstract/g3d/AnmObj.hh"
#include "abstract/g3d/ResCommon.hh"

#include <egg/math/Matrix.hh>

// Credit: kiwi515/ogws

namespace Kinoko::Abstract::g3d {

/// @brief Result of sampling a `CHR0` animation for one node on a given frame
/// @details Most of this implementation is unused since we only need to implement scaling
/// animations for @ref Field::ObjectPropeller and @ref Field::ObjectPress.
struct ChrAnmResult {
    /// @brief Animation flags. Unused in Kinoko but kept in case any custom tracks implement unique
    /// animation behavior
    enum Flag {
        FLAG_ANM_EXISTS = (1 << 0),
        FLAG_MTX_IDENT = (1 << 1),
        FLAG_ROT_TRANS_ZERO = (1 << 2),
        FLAG_SCALE_ONE = (1 << 3),
        FLAG_SCALE_UNIFORM = (1 << 4),
        FLAG_ROT_ZERO = (1 << 5),
        FLAG_TRANS_ZERO = (1 << 6),
        FLAG_PATCH_SCALE = (1 << 7),
        FLAG_PATCH_ROT = (1 << 8),
        FLAG_PATCH_TRANS = (1 << 9),

        // Maya Scale Compensation
        FLAG_SSC_APPLY = (1 << 10),
        FLAG_SSC_PARENT = (1 << 11),

        // Softimage Hierarchical Scaling
        FLAG_XSI_SCALING = (1 << 12),
    };

    u32 flags;          ///< Set but not used in Kinoko, contains the animation flags for the node
    EGG::Vector3f s;    ///< Scale vector
    EGG::Vector3f rawR; ///< Unused in Kinoko, raw rotation vector
    EGG::Matrix34f rt;  ///< Unused in Kinoko, rotation-translation matrix

    /// @addr{0x800555C0}
    /// @brief Gets the scale vector
    /// @return The scale vector, @ref s
    [[nodiscard]] const EGG::Vector3f &scale() const {
        return s;
    }
};

/// @brief Represents the CHR0 file format, which pertains to model movement animations.
class ResAnmChr {
public:
    /// @brief Keyframe values (FVS) comprised of 2-byte integers
    /// @details This is more compact than @ref FVS96Data but has lower precision since frames are
    /// limited to 1/32 increments.
    struct Frm48Data {
        s16 frame; ///< Fixed-point frame number with 5 fractional bits
        u16 value; ///< The keyframe value
        s16 slope; ///< Fixed-point value rate of change (units / frame) with 8 fractional bits
    };

    /// @brief Keyframe value (FVS) data comprised of 2-byte integers
    struct FVS48Data {
        f32 scale;                ///< The scaled size of one step in the keyframe value
        f32 offset;               ///< Base value for the keyframe values
        Frm48Data frameValues[1]; ///< The keyframe values
    };

    /// @brief Keyframe values (FVS) comprised of 4-byte floats
    struct Frm96Data {
        f32 frame; ///< Fractional frame number
        f32 value; ///< The keyframe value
        f32 slope; ///< The rate of change of the keyframe value (units / frame)
    };

    /// @brief Keyframe value (FVS) data comprised of 4-byte floats
    /// @details Unlike @ref FVS48Data, there is no `scale` or `offset` since the keyframe values
    /// are stored as full-precision floats rather than fixed-point integers.
    struct FVS96Data {
        Frm96Data frameValues[1]; ///< The keyframe values
    };

    /// @brief Keyframe value (FVS) data, which can be either 2-byte integers or 4-byte floats
    struct FVSData {
        u16 numFrameValues;   ///< The number of keyframe values in the animation data
        u16 padding;          ///< Padding
        f32 invKeyFrameRange; ///< Reciprocal of the anim's frame range (1 / lastFrame - firstFrame)

        union {
            FVS48Data fvs48; ///< FVS data comprised of 2-byte integers
            FVS96Data fvs96; ///< FVS data comprised of 4-byte floats
        };
    };

    /// @brief Const value (CV) animation data, simply an array of floating-point values
    struct CV32Data {
        f32 values[1]; ///< Array of values for the animation
    };

    /// @brief Simple wrapper around @ref CV32Data
    struct CVData {
        CV32Data cv32; ///< The underlying array of values for the animation
    };

    /// @brief Animation data, which can be either keyframe values (FVS) or const values (CV)
    struct AnmData {
        union {
            FVSData fvs; ///< Keyframe animation data
            CVData cv;   ///< Const value animation data
        };
    };

    /// @brief Information for one animation node (bone)
    struct NodeData {
        /// @brief Animation flags for the node
        /// @details For Kinoko, we only care about @ref FLAG_SCALE_X_CONST, @ref
        /// FLAG_SCALE_Y_CONST, and @ref FLAG_SCALE_Z_CONST as these identify that the nodes use a
        /// constant data value rather than a @ref ResAnmChr::AnmData block.
        enum Flag {
            FLAG_ANM_EXISTS = (1 << 0),
            FLAG_MTX_IDENT = (1 << 1),
            FLAG_ROT_TRANS_ZERO = (1 << 2),
            FLAG_SCALE_ONE = (1 << 3),
            FLAG_SCALE_UNIFORM = (1 << 4),
            FLAG_ROT_ZERO = (1 << 5),
            FLAG_TRANS_ZERO = (1 << 6),
            FLAG_PATCH_SCALE = (1 << 7),
            FLAG_PATCH_ROT = (1 << 8),
            FLAG_PATCH_TRANS = (1 << 9),

            // Maya Scale Compensation
            FLAG_SSC_APPLY = (1 << 10),
            FLAG_SSC_PARENT = (1 << 11),

            // Softimage Hierarchical Scaling
            FLAG_XSI_SCALING = (1 << 12),

            FLAG_SCALE_X_CONST = (1 << 13),
            FLAG_SCALE_Y_CONST = (1 << 14),
            FLAG_SCALE_Z_CONST = (1 << 15),

            FLAG_ROT_X_CONST = (1 << 16),
            FLAG_ROT_Y_CONST = (1 << 17),
            FLAG_ROT_Z_CONST = (1 << 18),

            FLAG_TRANS_X_CONST = (1 << 19),
            FLAG_TRANS_Y_CONST = (1 << 20),
            FLAG_TRANS_Z_CONST = (1 << 21),

            FLAG_HAS_SCALE = (1 << 22),
            FLAG_HAS_ROT = (1 << 23),
            FLAG_HAS_TRANS = (1 << 24),

            FLAG_SCALE_FVS32_FMT = (1 << 25),
            FLAG_SCALE_FVS48_FMT = (1 << 26),
            FLAG_SCALE_FVS96_FMT = FLAG_SCALE_FVS32_FMT | FLAG_SCALE_FVS48_FMT,

            FLAG_ROT_FVS32_FMT = (1 << 27),
            FLAG_ROT_FVS48_FMT = (1 << 28),
            FLAG_ROT_FVS96_FMT = FLAG_ROT_FVS32_FMT | FLAG_ROT_FVS48_FMT,

            FLAG_ROT_CV8_FMT = (1 << 29),
            FLAG_ROT_CV16_FMT = (1 << 27) | (1 << 29),
            FLAG_ROT_CV32_FMT = (1 << 28) | (1 << 29),

            FLAG_TRANS_FVS32_FMT = (1 << 30),
            FLAG_TRANS_FVS48_FMT = (1 << 31),
            FLAG_TRANS_FVS96_FMT = FLAG_TRANS_FVS32_FMT | FLAG_TRANS_FVS48_FMT,

            FLAG_HAS_SRT_MASK = FLAG_HAS_SCALE | FLAG_HAS_ROT | FLAG_HAS_TRANS,

            FLAG_SCALE_FMT_MASK =
                    FLAG_SCALE_FVS32_FMT | FLAG_SCALE_FVS48_FMT | FLAG_SCALE_FVS96_FMT,

            FLAG_ROT_FMT_MASK = FLAG_ROT_FVS32_FMT | FLAG_ROT_FVS48_FMT | FLAG_ROT_FVS96_FMT |
                    FLAG_ROT_CV8_FMT | FLAG_ROT_CV16_FMT | FLAG_ROT_CV32_FMT,

            FLAG_TRANS_FMT_MASK = FLAG_TRANS_FVS32_FMT | FLAG_TRANS_FVS48_FMT | FLAG_TRANS_FVS96_FMT
        };

        /// @brief Union representing animation data for a node
        /// @details This can either be a constant value (identified with the
        /// FLAG_SCALE_X/Y/Z/_CONST @ref Flag) or an offset to the actual animation data.
        union AnmData {
            s32 toResAnmChrAnmData; ///< Offset to the animation data block
            f32 constValue;         ///< Constant value for the animation data
        };

        s32 name;        ///< Unused in Kinoko, offset to the node's name string
        u32 flags;       ///< The raw @ref Flag values for the node (not byte-swapped)
        AnmData anms[1]; ///< Array of animation data (size 1 if uniform scale, size 3 otherwise)
    };
    STATIC_ASSERT(sizeof(NodeData) == 0xC);

    /// @brief `CHR0` info header containing animation metadata
    struct InfoData {
        u16 numFrame;     ///< Framecount of the animation
        u16 numNode;      ///< Number of keyframes in the animation
        AnmPolicy policy; ///< Animation policy for the `CHR0` data
        u32 scalingRule;  ///< Unused in Kinoko

        /// @brief Initializes a @ref InfoData instance by reading data from a stream
        /// @param stream The input stream to read the @ref InfoData from
        void read(EGG::Stream &stream) {
            numFrame = stream.read_u16();
            numNode = stream.read_u16();
            policy = static_cast<AnmPolicy>(stream.read_u32());
            scalingRule = stream.read_u32();
        }
    };
    STATIC_ASSERT(sizeof(InfoData) == 0xC);

    /// @brief `CHR0` file format header
    struct Data {
        ResBlockHeaderData header; ///< Resource block header info
        u32 revision;              ///< Unused in Kinoko, revision of the `CHR0` file format
        s32 toResFileData;         ///< Unused in Kinoko, offset to the start of the `.brres` file
        s32 toChrDataDic;          ///< Offset to the animation data section
        s32 toResUserData;         ///< Unused in Kinoko
        s32 name;                  ///< Unused in Kinoko
        s32 originalPath;          ///< Unused in Kinoko
        InfoData info;             ///< `CHR0` info header containing animation metadata
    };
    STATIC_ASSERT(sizeof(Data) == 0x2C);

    ResAnmChr(const void *data);
    ~ResAnmChr();

    void read(EGG::Stream &stream);

    [[nodiscard]] ChrAnmResult getAnmResult(f32 frame, size_t idx) const;

    /// @beginGetters

    [[nodiscard]] u16 frameCount() const;
    [[nodiscard]] AnmPolicy policy() const;

    /// @endGetters

private:
    /// @brief Function pointer type for retrieving animation sampling results for a node
    /// @param frame The current animation frame
    /// @param result The structure to store the sampled animation result
    /// @param infoData The `CHR0` info header containing animation metadata
    /// @param nodeData The node data
    typedef void (*GetAnmResultFunc)(f32 frame, ChrAnmResult &result,
            const ResAnmChr::InfoData &infoData, const ResAnmChr::NodeData *nodeData);

    const Data *m_rawData; ///< Pointer to the raw `CHR0` data block
    InfoData m_infoData;   ///< `CHR0` info header containing animation metadata

    /// @brief The number of different @ref GetAnmResultFunc types available in the lookup table
    static constexpr size_t NUM_RESULT_FUNCS = 8;

    /// @brief Lookup table for different @ref GetAnmResultFunc types indexed based on node flags
    static const std::array<GetAnmResultFunc, NUM_RESULT_FUNCS> s_getAnmResultTable;
};

/// @brief Wrapper class that pairs a @ref ResAnmChr resource with a @ref FrameCtrl for animation
/// control
class AnmObjChrRes : public FrameCtrl {
public:
    AnmObjChrRes(const ResAnmChr &chr);
    ~AnmObjChrRes();

    /// @beginGetters

    [[nodiscard]] u16 frameCount() const;

    /// @endGetters

private:
    const ResAnmChr m_resAnmChr; ///< The underlying `CHR0` resource data
};

} // namespace Kinoko::Abstract::g3d
