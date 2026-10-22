#include "ResAnmChr.hh"

#include "abstract/g3d/ResDic.hh"

// Credit: kiwi515/ogws

namespace Kinoko::Abstract::g3d {

/// @brief Constructor
/// @param data Pointer to the raw `CHR0` data block
/// @details Sets @ref m_rawData to the provided `data`. Parses @ref m_infoData from a stream of the
/// raw data.
ResAnmChr::ResAnmChr(const void *data) : m_rawData(reinterpret_cast<const Data *>(data)) {
    EGG::RamStream stream = EGG::RamStream(data, sizeof(Data));
    read(stream);
}

/// @brief Default destructor
ResAnmChr::~ResAnmChr() = default;

/// @brief Initializes @ref m_infoData from a stream of raw `CHR0` block data
/// @param stream The input stream to read the `CHR0` info header from
void ResAnmChr::read(EGG::Stream &stream) {
    stream.jump(sizeof(Data));
    m_infoData.read(stream);
}

/// @addr{0x80055540}
/// @brief Samples the animation for a specific node at a given frame
/// @param frame The current animation frame
/// @param idx The index of the node to sample
/// @return The sampled animation result for the specified node and frame
ChrAnmResult ResAnmChr::getAnmResult(f32 frame, size_t idx) const {
    s32 offset = parse<s32>(m_rawData->toChrDataDic);
    ResDic dic = ResDic(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(m_rawData) + offset));
    const NodeData *data = reinterpret_cast<const NodeData *>(dic[idx]);

    u32 flags = parse<u32>(data->flags);

    ChrAnmResult result;
    result.flags = flags &
            (ChrAnmResult::Flag::FLAG_ANM_EXISTS | ChrAnmResult::Flag::FLAG_MTX_IDENT |
                    ChrAnmResult::Flag::FLAG_ROT_TRANS_ZERO | ChrAnmResult::Flag::FLAG_SCALE_ONE |
                    ChrAnmResult::Flag::FLAG_SCALE_UNIFORM | ChrAnmResult::Flag::FLAG_ROT_ZERO |
                    ChrAnmResult::Flag::FLAG_TRANS_ZERO | ChrAnmResult::Flag::FLAG_PATCH_SCALE |
                    ChrAnmResult::Flag::FLAG_PATCH_ROT | ChrAnmResult::Flag::FLAG_PATCH_TRANS |
                    ChrAnmResult::Flag::FLAG_SSC_APPLY | ChrAnmResult::Flag::FLAG_SSC_PARENT |
                    ChrAnmResult::Flag::FLAG_XSI_SCALING);

    u32 index = (flags & NodeData::Flag::FLAG_HAS_SRT_MASK) >> 22;

    ASSERT(index < s_getAnmResultTable.size());

    (*s_getAnmResultTable[index])(frame, result, m_infoData, data);

    return result;
}

/// @brief Gets the framecount of the animation
/// @return The framecount of the animation
u16 ResAnmChr::frameCount() const {
    return m_infoData.numFrame;
}

/// @brief Gets the animation policy of the `CHR0` data
/// @return The animation policy
AnmPolicy ResAnmChr::policy() const {
    return m_infoData.policy;
}

/// @brief Small helper class that acts like an iterator over animation keyframes
/// @tparam TData The type of the animation keyframe data (either @ref Frm48Data or @ref Frm96Data)
/// @tparam TDerived The derived class type that inherits from this base class
/// @details `TDerived` is used so that this class models CRTP (Curiously Recurring Template
/// Pattern), allowing the base class to return instances of the derived class.
template <typename TData, typename TDerived>
class CResAnmChrFrmBase {
public:
    /// @brief Constructor
    /// @param ptr Pointer to the animation keyframe data of type @ref TData
    /// @details Simply sets @ref m_ptr to @ptr.
    CResAnmChrFrmBase(const TData *ptr) : m_ptr(ptr) {}

    /// @brief Post-increment operator
    /// @details Increments the internal pointer to the next keyframe.
    void operator++(int) {
        m_ptr++;
    }
    /// @brief Post-decrement operator
    /// @details Decrements the internal pointer to the previous keyframe.
    void operator--(int) {
        m_ptr--;
    }

    /// @brief Addition operator
    /// @param n The number of keyframes to advance
    /// @return A new instance of the derived class pointing to the keyframe `n` steps ahead
    [[nodiscard]] TDerived operator+(int n) {
        return TDerived(m_ptr + n);
    }

protected:
    const TData *m_ptr; ///< Pointer to the current animation keyframe data
};

/// @brief Convenience template class for animation keyframe iterators
/// @tparam T The type of the animation keyframe data
/// @details This alias simplifies the creation of animation keyframe iterators by
/// automatically deducing the derived class type.
template <typename T>
class CResAnmChrFrm;

/// @brief Template specialization class for iterating @ref ResAnmChr::Frm48Data animation keyframes
template <>
class CResAnmChrFrm<ResAnmChr::Frm48Data>
    : public CResAnmChrFrmBase<ResAnmChr::Frm48Data, CResAnmChrFrm<ResAnmChr::Frm48Data>> {
public:
    /// @brief Constructor
    /// @param ptr Pointer to the animation keyframe data of type @ref ResAnmChr::Frm48Data
    CResAnmChrFrm(const ResAnmChr::Frm48Data *ptr) : CResAnmChrFrmBase(ptr) {}

    /// @brief Retrieves the frame index of the current keyframe
    /// @return The fixed-point frame index from @ref Frm48Data::frame
    [[nodiscard]] s16 GetFrame() const {
        return parse<s16>(m_ptr->frame);
    }

    /// @brief Retrieves the frame index of the current keyframe as a floating-point value
    /// @return The frame index converted to a floating-point value from @ref Frm48Data::frame
    /// @details Converts the fixed-point frame index to a floating-point value by dividing it by
    /// `32.0f`.
    [[nodiscard]] f32 GetFrameF32() const {
        return static_cast<f32>(GetFrame()) * (1.0f / 32.0f);
    }

    /// @brief Retrieves the value of the current keyframe
    /// @param pFVSData Pointer to the keyframe value data containing scale and offset information
    /// @return The calculated value using the formula `value = scale * pFVSData->value + offset`
    /// @details The value is computed using the fixed-point value from @ref Frm48Data::value,
    /// scaled and offset by the corresponding FVS data. The terms are multiple and added using
    /// paired-singles implementation via @ref EGG::Mathf::fma().
    [[nodiscard]] f32 GetValue(const ResAnmChr::FVSData *pFVSData) const {
        f32 x = parse<f32>(pFVSData->fvs48.scale);
        f32 y = static_cast<f32>(parse<u16>(m_ptr->value));
        f32 z = parse<f32>(pFVSData->fvs48.offset);

        return EGG::Mathf::fma(x, y, z);
    }

    /// @brief Retrieves the rate of change of the animation at the current keyframe
    /// @return The rate of change of the animation at the current keyframe, converted to a
    /// floating-point value from @ref Frm48Data::slope
    /// @details The slope is computed using the fixed-point value from @ref Frm48Data::slope and
    /// converted to a floating-point value by dividing it by `256.0f`.
    [[nodiscard]] f32 GetSlope() const {
        return static_cast<f32>(parse<s16>(m_ptr->slope)) * (1.0f / 256.0f);
    }
};

/// @brief Template specialization class for iterating @ref ResAnmChr::Frm96Data animation keyframes
template <>
class CResAnmChrFrm<ResAnmChr::Frm96Data>
    : public CResAnmChrFrmBase<ResAnmChr::Frm96Data, CResAnmChrFrm<ResAnmChr::Frm96Data>> {
public:
    /// @brief Constructor
    /// @param ptr Pointer to the animation keyframe data of type @ref ResAnmChr::Frm96Data
    CResAnmChrFrm(const ResAnmChr::Frm96Data *ptr) : CResAnmChrFrmBase(ptr) {}

    /// @brief Retrieves the frame index of the current keyframe
    /// @return The frame index as a floating-point value from @ref Frm96Data::frame
    [[nodiscard]] f32 GetFrame() const {
        return parse<f32>(m_ptr->frame);
    }

    /// @brief Retrieves the frame index of the current keyframe
    /// @return The frame index as a floating-point value from @ref Frm96Data::frame
    /// @details This is identical to @ref GetFrame() since the underlying type is already a
    /// floating-point value.
    [[nodiscard]] f32 GetFrameF32() const {
        return parse<f32>(m_ptr->frame);
    }

    /// @brief Retrieves the value of the current keyframe
    /// @return The value from @ref Frm96Data::value
    [[nodiscard]] f32 GetValue(const ResAnmChr::FVSData * /*pFVSData*/) const {
        return parse<f32>(m_ptr->value);
    }

    /// @brief Retrieves the rate of change of the animation at the current keyframe
    /// @return The rate of change of the animation at the current keyframe from @ref
    /// Frm96Data::slope
    [[nodiscard]] f32 GetSlope() const {
        return parse<f32>(m_ptr->slope);
    }
};

/// @brief Animation trait class template that maps each keyframe track format to the appropriate
/// types and helper functions for reading keyframe data
template <typename T>
class CAnmFmtTraits;

template <>
class CAnmFmtTraits<ResAnmChr::FVS48Data> {
public:
    typedef ResAnmChr::Frm48Data TFrmData;
    typedef s16 TFrame;

    [[nodiscard]] static CResAnmChrFrm<TFrmData> GetKeyFrame(const ResAnmChr::FVSData *pFVSData,
            int index) {
        return CResAnmChrFrm<TFrmData>(&pFVSData->fvs48.frameValues[index]);
    }

    [[nodiscard]] static TFrame QuantizeFrame(f32 frame) {
        return static_cast<TFrame>(frame * 32.0f);
    }
};

template <>
class CAnmFmtTraits<ResAnmChr::FVS96Data> {
public:
    typedef ResAnmChr::Frm96Data TFrmData;
    typedef f32 TFrame;

    [[nodiscard]] static CResAnmChrFrm<TFrmData> GetKeyFrame(const ResAnmChr::FVSData *pFVSData,
            int index) {
        return CResAnmChrFrm<TFrmData>(&pFVSData->fvs96.frameValues[index]);
    }

    [[nodiscard]] static TFrame QuantizeFrame(f32 frame) {
        return frame;
    }
};

/// @brief Calculate frame values (FVS) result
template <typename T>
[[nodiscard]] f32 CalcResultFVS(f32 frame, const ResAnmChr::NodeData *nodeData,
        const ResAnmChr::NodeData::AnmData *anmData, bool constant) {
    if (constant) {
        return parse<f32>(anmData->constValue);
    }

    const ResAnmChr::AnmData *pFVSAnmData = reinterpret_cast<const ResAnmChr::AnmData *>(
            reinterpret_cast<uintptr_t>(nodeData) + parse<s32>(anmData->toResAnmChrAnmData));

    return CalcAnimationFVS<CAnmFmtTraits<T>>(frame, &pFVSAnmData->fvs);
}

[[nodiscard]] inline f32 CalcResult48(f32 frame, const ResAnmChr::NodeData *nodeData,
        const ResAnmChr::NodeData::AnmData *anmData, bool constant) {
    return CalcResultFVS<ResAnmChr::FVS48Data>(frame, nodeData, anmData, constant);
}

[[nodiscard]] inline f32 CalcResult96(f32 frame, const ResAnmChr::NodeData *nodeData,
        const ResAnmChr::NodeData::AnmData *anmData, bool constant) {
    return CalcResultFVS<ResAnmChr::FVS96Data>(frame, nodeData, anmData, constant);
}

/// @brief Frame values (FVS) implementation
template <typename TTraits>
[[nodiscard]] f32 CalcAnimationFVS(f32 frame, const ResAnmChr::FVSData *pFVSData) {
    auto first = TTraits::GetKeyFrame(pFVSData, 0);
    auto last = TTraits::GetKeyFrame(pFVSData, parse<u16>(pFVSData->numFrameValues) - 1);

    if (frame <= first.GetFrameF32()) {
        return first.GetValue(pFVSData);
    }

    if (last.GetFrameF32() <= frame) {
        return last.GetValue(pFVSData);
    }

    f32 frameOffset = frame - first.GetFrameF32();
    f32 numKeyFrame = static_cast<f32>(parse<u16>(pFVSData->numFrameValues));

    f32 f_estimatePos = parse<f32>(pFVSData->invKeyFrameRange) * (frameOffset * numKeyFrame);
    u16 i_estimatePos = static_cast<u16>(f_estimatePos);

    auto left = TTraits::GetKeyFrame(pFVSData, i_estimatePos);
    auto quantized = TTraits::QuantizeFrame(frame);

    if (quantized < left.GetFrame()) {
        do {
            left--;
        } while (quantized < left.GetFrame());
    } else {
        do {
            left++;
        } while (left.GetFrame() <= quantized);

        left--;
    }

    if (frame == left.GetFrameF32()) {
        return left.GetValue(pFVSData);
    }

    auto right = left + 1;

    f32 v0 = left.GetValue(pFVSData);
    f32 t0 = left.GetSlope();
    f32 v1 = right.GetValue(pFVSData);
    f32 t1 = right.GetSlope();

    f32 f0 = left.GetFrameF32();
    f32 f1 = right.GetFrameF32();

    f32 frameDelta = frame - f0;
    f32 keyFrameDelta = f1 - f0;
    f32 keyFrameDeltaInv = EGG::Mathf::finv(keyFrameDelta);
    f32 t = frameDelta * keyFrameDeltaInv;
    f32 tMinus1 = EGG::Mathf::fms(frameDelta, keyFrameDeltaInv, 1.0f);
    f32 tanInterp = EGG::Mathf::fma(tMinus1, t0, t * t1);
    f32 scaledCurve = t * (EGG::Mathf::fms(2.0f, t, 3.0f) * (v0 - v1));

    return EGG::Mathf::fma(frameDelta * tMinus1, tanInterp, EGG::Mathf::fma(t, scaledCurve, v0));
}

const ResAnmChr::NodeData::AnmData *GetAnmScale(f32 frame, EGG::Vector3f &result,
        const ResAnmChr::NodeData *nodeData, const ResAnmChr::NodeData::AnmData *anmData) {
    u32 flags = parse<u32>(nodeData->flags);

    switch (flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_FMT_MASK) {
    case 0:
    case ResAnmChr::NodeData::Flag::FLAG_SCALE_FVS32_FMT:
    case ResAnmChr::NodeData::Flag::FLAG_SCALE_FVS48_FMT: {
        result.x = CalcResult48(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::FLAG_SCALE_X_CONST);

        if (flags & ResAnmChr::NodeData::FLAG_SCALE_UNIFORM) {
            result.y = result.x;
            result.z = result.x;
        } else {
            result.y = CalcResult48(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_Y_CONST);
            result.z = CalcResult48(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_Z_CONST);
        }
    } break;
    case ResAnmChr::NodeData::Flag::FLAG_SCALE_FVS96_FMT: {
        result.x = CalcResult96(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::FLAG_SCALE_X_CONST);

        if (flags & ResAnmChr::NodeData::FLAG_SCALE_UNIFORM) {
            result.y = result.x;
            result.z = result.x;
        } else {
            result.y = CalcResult96(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::FLAG_SCALE_Y_CONST);
            result.z = CalcResult96(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::FLAG_SCALE_Z_CONST);
        }
    } break;
    default: {
        result.setZero();
    }
    }

    return anmData;
}

void GetAnmResult_Stub(f32 /*frame*/, ChrAnmResult & /*result*/,
        const ResAnmChr::InfoData & /*infoData*/, const ResAnmChr::NodeData * /*nodeData*/) {}

void GetAnmResult_S(f32 frame, ChrAnmResult &result, const ResAnmChr::InfoData & /*infoData*/,
        const ResAnmChr::NodeData *nodeData) {
    GetAnmScale(frame, result.s, nodeData, nodeData->anms);
    result.rt = EGG::Matrix34f::ident;
}

/// @brief Constructor
/// @param chr The `CHR0` resource data to be used for the animation object
/// @details Initializes @ref m_startFrame to zero and @ref m_endFrame to the total number of frames
/// in the animation. Sets @ref m_playPolicy based on the animation policy of the `CHR0` resource.
/// Finally, sets @ref m_resAnmChr to the provided `chr`.
AnmObjChrRes::AnmObjChrRes(const ResAnmChr &chr)
    : FrameCtrl(0.0f, chr.frameCount(), GetAnmPlayPolicy(chr.policy())),
      m_resAnmChr(chr) {}

/// @brief Default destructor
AnmObjChrRes::~AnmObjChrRes() = default;

/// @brief Gets the framecount of the animation
/// @return The number of frames in the animation
u16 AnmObjChrRes::frameCount() const {
    return m_resAnmChr.frameCount();
}

const std::array<ResAnmChr::GetAnmResultFunc, ResAnmChr::NUM_RESULT_FUNCS>
        ResAnmChr::s_getAnmResultTable = {{
                &GetAnmResult_Stub,
                &GetAnmResult_S,
                &GetAnmResult_Stub,
                &GetAnmResult_Stub,
                &GetAnmResult_Stub,
                &GetAnmResult_Stub,
                &GetAnmResult_Stub,
                &GetAnmResult_Stub,
        }};

} // namespace Kinoko::Abstract::g3d
