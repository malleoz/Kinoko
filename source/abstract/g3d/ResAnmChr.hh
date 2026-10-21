#pragma once

#include "abstract/g3d/AnmObj.hh"
#include "abstract/g3d/ResCommon.hh"

#include <egg/math/Matrix.hh>

// Credit: kiwi515/ogws

namespace Kinoko::Abstract::g3d {
struct ChrAnmResult {
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

    u32 flags;
    EGG::Vector3f s;
    EGG::Vector3f rawR;
    EGG::Matrix34f rt;

    /// @addr{0x800555C0}
    [[nodiscard]] EGG::Vector3f scale() const {
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

    struct NodeData {
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

        union AnmData {
            s32 toResAnmChrAnmData;
            f32 constValue;
        };

        s32 name;
        u32 flags;
        AnmData anms[1];
    };
    STATIC_ASSERT(sizeof(NodeData) == 0xC);

    struct InfoData {
        u16 numFrame;
        u16 numNode;
        AnmPolicy policy;
        u32 scalingRule;

        void read(EGG::Stream &stream) {
            numFrame = stream.read_u16();
            numNode = stream.read_u16();
            policy = static_cast<AnmPolicy>(stream.read_u32());
            scalingRule = stream.read_u32();
        }
    };
    STATIC_ASSERT(sizeof(InfoData) == 0xC);

    struct Data {
        ResBlockHeaderData header;
        u32 revision;
        s32 toResFileData;
        s32 toChrDataDic;
        s32 toResUserData;
        s32 name;
        s32 originalPath;
        InfoData info;
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
    typedef void (*GetAnmResultFunc)(f32 frame, ChrAnmResult &result,
            const ResAnmChr::InfoData &infoData, const ResAnmChr::NodeData *nodeData);

    const Data *m_rawData;
    InfoData m_infoData;

    static constexpr size_t NUM_RESULT_FUNCS = 8;
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
