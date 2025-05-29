#include "ResAnmChr.hh"

// Credit: kiwi515/ogws

namespace Abstract {
namespace g3d {

inline f32 HermiteInterpolation(f32 v0, f32 t0, f32 v1, f32 t1, f32 s, f32 d) {
    f32 s_1 = s - 1.0f;

    return v0 + s * (s * ((2.0f * s - 3.0f) * (v0 - v1))) + d * s_1 * (s_1 * t0 + s * t1);
}

/// @addr{0x80055540}
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

    (*s_getAnmResultTable[index])(frame, result, m_infoData, data);

    return result;
}

/******************************************************************************
 *
 * Animation keyframe
 *
 ******************************************************************************/
template <typename TData, typename TDerived>
class CResAnmChrFrmBase {
protected:
    const TData *mPtr;

public:
    CResAnmChrFrmBase(const TData *pPtr) : mPtr(pPtr) {}

    void operator++(int) {
        mPtr++;
    }
    void operator--(int) {
        mPtr--;
    }

    TDerived operator+(int n) {
        return TDerived(mPtr + n);
    }
};

template <typename T>
class CResAnmChrFrm : public CResAnmChrFrmBase<T, CResAnmChrFrm<T>> {};

template <>
class CResAnmChrFrm<ResAnmChr::Frm32Data>
    : public CResAnmChrFrmBase<ResAnmChr::Frm32Data, CResAnmChrFrm<ResAnmChr::Frm32Data>> {
public:
    CResAnmChrFrm(const ResAnmChr::Frm32Data *pPtr) : CResAnmChrFrmBase(pPtr) {}

    u8 GetFrame() const {
        return parse<u8>(mPtr->fvs.frame);
    }

    f32 GetFrameF32() const {
        return static_cast<f32>(GetFrame());
    }

    f32 GetValue(const ResAnmChr::FVSData *pFVSData) const {
        u16 value = parse<u32>((mPtr->fvsU32 & 0x00FFF000) >> 12);
        return parse<f32>(pFVSData->fvs32.scale) * static_cast<f32>(value) +
                parse<f32>(pFVSData->fvs48.offset);
    }

    f32 GetSlope() const {
        s16 slope = parse<s16>(static_cast<s32>((mPtr->fvsU32 & 0x00000FFF) << 20) >> 20);
        return static_cast<f32>(slope) * (1.0f / 32.0f);
    }
};

template <>
class CResAnmChrFrm<ResAnmChr::Frm48Data>
    : public CResAnmChrFrmBase<ResAnmChr::Frm48Data, CResAnmChrFrm<ResAnmChr::Frm48Data>> {
public:
    CResAnmChrFrm(const ResAnmChr::Frm48Data *pPtr) : CResAnmChrFrmBase(pPtr) {}

    s16 GetFrame() const {
        return parse<s16>(mPtr->frame);
    }

    f32 GetFrameF32() const {
        return static_cast<f32>(parse<s16>(mPtr->frame)) * (1.0f / 32.0f);
    }

    f32 GetValue(const ResAnmChr::FVSData *pFVSData) const {
        return parse<f32>(pFVSData->fvs48.scale) * static_cast<f32>(parse<u16>(mPtr->value)) +
                parse<f32>(pFVSData->fvs48.offset);
    }

    f32 GetSlope() const {
        return static_cast<f32>(parse<s16>(mPtr->slope)) * (1.0f / 256.0f);
    }
};

template <>
class CResAnmChrFrm<ResAnmChr::Frm96Data>
    : public CResAnmChrFrmBase<ResAnmChr::Frm96Data, CResAnmChrFrm<ResAnmChr::Frm96Data>> {
public:
    CResAnmChrFrm(const ResAnmChr::Frm96Data *pPtr) : CResAnmChrFrmBase(pPtr) {}

    f32 GetFrame() const {
        return parse<f32>(mPtr->frame);
    }

    f32 GetFrameF32() const {
        return parse<f32>(mPtr->frame);
    }

    f32 GetValue(const ResAnmChr::FVSData * /* pFVSData */) const {
        return parse<f32>(mPtr->value);
    }

    f32 GetSlope() const {
        return parse<f32>(mPtr->slope);
    }
};

/******************************************************************************
 *
 * Animation traits
 *
 ******************************************************************************/
template <typename T>
class CAnmFmtTraits {};

template <>
class CAnmFmtTraits<ResAnmChr::FVS32Data> {
public:
    typedef ResAnmChr::Frm32Data TFrmData;
    typedef u8 TFrame;

public:
    static CResAnmChrFrm<TFrmData> GetKeyFrame(const ResAnmChr::FVSData *pFVSData, int index) {
        return CResAnmChrFrm<TFrmData>(&pFVSData->fvs32.frameValues[index]);
    }

    static TFrame QuantizeFrame(f32 frame) {
        return static_cast<TFrame>(frame);
    }
};

template <>
class CAnmFmtTraits<ResAnmChr::FVS48Data> {
public:
    typedef ResAnmChr::Frm48Data TFrmData;
    typedef s16 TFrame;

public:
    static CResAnmChrFrm<TFrmData> GetKeyFrame(const ResAnmChr::FVSData *pFVSData, int index) {
        return CResAnmChrFrm<TFrmData>(&pFVSData->fvs48.frameValues[index]);
    }

    static TFrame QuantizeFrame(f32 frame) {
        return static_cast<TFrame>(frame * 32.0f);
    }
};

template <>
class CAnmFmtTraits<ResAnmChr::FVS96Data> {
public:
    typedef ResAnmChr::Frm96Data TFrmData;
    typedef f32 TFrame;

public:
    static CResAnmChrFrm<TFrmData> GetKeyFrame(const ResAnmChr::FVSData *pFVSData, int index) {
        return CResAnmChrFrm<TFrmData>(&pFVSData->fvs96.frameValues[index]);
    }

    static TFrame QuantizeFrame(f32 frame) {
        return frame;
    }
};

template <>
class CAnmFmtTraits<ResAnmChr::CV8Data> {
public:
    static f32 GetAt(const ResAnmChr::CVData *pCVData, u16 idx) {
        const ResAnmChr::CV8Data &rCV8 = pCVData->cv8;
        return parse<f32>(rCV8.values[idx]) * parse<f32>(rCV8.scale) + parse<f32>(rCV8.offset);
    }
};

template <>
class CAnmFmtTraits<ResAnmChr::CV16Data> {
public:
    static f32 GetAt(const ResAnmChr::CVData *pCVData, u16 idx) {
        const ResAnmChr::CV16Data &rCV16 = pCVData->cv16;
        return parse<f32>(rCV16.values[idx]) * parse<f32>(rCV16.scale) + parse<f32>(rCV16.offset);
    }
};

template <>
class CAnmFmtTraits<ResAnmChr::CV32Data> {
public:
    static f32 GetAt(const ResAnmChr::CVData *pCVData, u16 idx) {
        const ResAnmChr::CV32Data &rCV32 = pCVData->cv32;
        return parse<f32>(rCV32.values[idx]);
    }
};

/******************************************************************************
 *
 * Calculate frame values (FVS) result
 *
 ******************************************************************************/
template <typename T>
f32 CalcResultFVS(f32 frame, const ResAnmChr::NodeData *nodeData,
        const ResAnmChr::NodeData::AnmData *anmData, bool constant) {
    if (constant) {
        return parse<f32>(anmData->constValue);
    }

    const ResAnmChr::AnmData *pFVSAnmData = reinterpret_cast<const ResAnmChr::AnmData *>(
            reinterpret_cast<uintptr_t>(nodeData) + parse<s32>(anmData->toResAnmChrAnmData));

    return CalcAnimationFVS<CAnmFmtTraits<T>>(frame, &pFVSAnmData->fvs);
}

inline f32 CalcResult32(f32 frame, const ResAnmChr::NodeData *nodeData,
        const ResAnmChr::NodeData::AnmData *anmData, bool constant) {
    return CalcResultFVS<ResAnmChr::FVS32Data>(frame, nodeData, anmData, constant);
}
inline f32 CalcResult48(f32 frame, const ResAnmChr::NodeData *nodeData,
        const ResAnmChr::NodeData::AnmData *anmData, bool constant) {
    return CalcResultFVS<ResAnmChr::FVS48Data>(frame, nodeData, anmData, constant);
}
inline f32 CalcResult96(f32 frame, const ResAnmChr::NodeData *nodeData,
        const ResAnmChr::NodeData::AnmData *anmData, bool constant) {
    return CalcResultFVS<ResAnmChr::FVS96Data>(frame, nodeData, anmData, constant);
}

/******************************************************************************
 *
 * Frame values (FVS) implementation
 *
 ******************************************************************************/
template <typename TTraits>
f32 CalcAnimationFVS(f32 frame, const ResAnmChr::FVSData *pFVSData) {
    CResAnmChrFrm<typename TTraits::TFrmData> first = TTraits::GetKeyFrame(pFVSData, 0);
    CResAnmChrFrm<typename TTraits::TFrmData> last =
            TTraits::GetKeyFrame(pFVSData, parse<u16>(pFVSData->numFrameValues) - 1);

    if (frame <= first.GetFrameF32()) {
        return first.GetValue(pFVSData);
    }

    if (last.GetFrameF32() <= frame) {
        return last.GetValue(pFVSData);
    }

    f32 frameOffset = frame - first.GetFrameF32();
    f32 numKeyFrame = static_cast<f32>(parse<u16>(pFVSData->numFrameValues));

    f32 f_estimatePos = frameOffset * numKeyFrame * parse<f32>(pFVSData->invKeyFrameRange);
    u16 i_estimatePos = static_cast<u16>(f_estimatePos);

    CResAnmChrFrm<typename TTraits::TFrmData> left = TTraits::GetKeyFrame(pFVSData, i_estimatePos);

    const typename TTraits::TFrame quantized = TTraits::QuantizeFrame(frame);

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

    CResAnmChrFrm<typename TTraits::TFrmData> right = left + 1;

    f32 v0 = left.GetValue(pFVSData);
    f32 t0 = left.GetSlope();
    f32 v1 = right.GetValue(pFVSData);
    f32 t1 = right.GetSlope();

    f32 f0 = left.GetFrameF32();
    f32 f1 = right.GetFrameF32(); // f1 wrong 1144 (256 instead of 8)

    f32 frameDelta = frame - f0;
    f32 keyFrameDelta = f1 - f0;
    f32 keyFrameDeltaInv = EGG::Mathf::finv(keyFrameDelta);

    return HermiteInterpolation(v0, t0, v1, t1, frameDelta * keyFrameDeltaInv, frameDelta);
}

/******************************************************************************
 *
 * Const value (CV) implementation
 *
 ******************************************************************************/
template <typename TTraits>
f32 CalcAnimationCV(const ResAnmChr::CVData *pCVData, u16 numFrame, f32 frame) {
    u16 frame_i = static_cast<u16>(frame);

    if (frame <= 0.0f) {
        return TTraits::GetAt(pCVData, 0);
    }

    if (numFrame <= frame_i) {
        return TTraits::GetAt(pCVData, numFrame);
    }

    f32 r = frame - static_cast<f32>(frame_i);

    f32 v0 = TTraits::GetAt(pCVData, frame_i);
    if (r == 0.0f) {
        return v0;
    }

    f32 v1 = TTraits::GetAt(pCVData, frame_i + 1);
    return r * (v1 - v0) + v0;
}

/******************************************************************************
 *
 * Calculate const value (CV) result
 *
 ******************************************************************************/
template <typename T>
f32 CalcResultCV(f32 frame, const ResAnmChr::NodeData *pNodeData,
        const ResAnmChr::NodeData::AnmData *pAnmData, u16 numFrame, bool constant) {
    if (constant) {
        return parse<f32>(pAnmData->constValue);
    }

    const ResAnmChr::AnmData *pCVAnmData = reinterpret_cast<const ResAnmChr::AnmData *>(
            reinterpret_cast<uintptr_t>(pNodeData) + pAnmData->toResAnmChrAnmData);

    return CalcAnimationCV<CAnmFmtTraits<T>>(&pCVAnmData->cv, numFrame, frame);
}

inline f32 CalcResultFrm8(f32 frame, const ResAnmChr::NodeData *pNodeData,
        const ResAnmChr::NodeData::AnmData *pAnmData, u16 numFrame, bool constant) {
    return CalcResultCV<ResAnmChr::CV8Data>(frame, pNodeData, pAnmData, numFrame, constant);
}
inline f32 CalcResultFrm16(f32 frame, const ResAnmChr::NodeData *pNodeData,
        const ResAnmChr::NodeData::AnmData *pAnmData, u16 numFrame, bool constant) {
    return CalcResultCV<ResAnmChr::CV16Data>(frame, pNodeData, pAnmData, numFrame, constant);
}
inline f32 CalcResultFrm32(f32 frame, const ResAnmChr::NodeData *pNodeData,
        const ResAnmChr::NodeData::AnmData *pAnmData, u16 numFrame, bool constant) {
    return CalcResultCV<ResAnmChr::CV32Data>(frame, pNodeData, pAnmData, numFrame, constant);
}

const ResAnmChr::NodeData::AnmData *GetAnmScale(f32 frame, EGG::Vector3f &result,
        const ResAnmChr::NodeData *nodeData, const ResAnmChr::NodeData::AnmData *anmData) {
    u32 flags = parse<u32>(nodeData->flags);

    switch (flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_FMT_MASK) {
    case 0: {
        result.x = parse<f32>(anmData++->constValue);

        if (flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_UNIFORM) {
            result.y = result.x;
            result.z = result.x;
        } else {
            result.y = parse<f32>(anmData++->constValue);
            result.y = parse<f32>(anmData++->constValue);
        }

        break;
    }
    case ResAnmChr::NodeData::Flag::FLAG_SCALE_FVS32_FMT: {
        result.x = CalcResult32(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::FLAG_SCALE_X_CONST);

        if (flags & ResAnmChr::NodeData::FLAG_SCALE_UNIFORM) {
            result.y = result.x;
            result.z = result.x;
        } else {
            result.y = CalcResult32(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_Y_CONST);
            result.z = CalcResult32(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_Z_CONST);
        }

        break;
    }
    case ResAnmChr::NodeData::Flag::FLAG_SCALE_FVS48_FMT: {
        result.x = CalcResult48(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::FLAG_SCALE_X_CONST);

        if (flags & ResAnmChr::NodeData::FLAG_SCALE_UNIFORM) {
            result.y = result.x;
            result.z = result.x;
        } else {
            result.y = CalcResult48(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_Y_CONST); // off-by-one 1163
            result.z = CalcResult48(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_Z_CONST);
        }

        break;
    }
    case ResAnmChr::NodeData::Flag::FLAG_SCALE_FVS96_FMT: {
        result.x = CalcResult96(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::FLAG_SCALE_X_CONST);

        if (flags & ResAnmChr::NodeData::FLAG_SCALE_UNIFORM) {
            result.y = result.x;
            result.z = result.x;
        } else {
            result.y = CalcResult96(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_Y_CONST);
            result.z = CalcResult96(frame, nodeData, anmData++,
                    flags & ResAnmChr::NodeData::Flag::FLAG_SCALE_Z_CONST);
        }

        break;
    }
    default: {
        result.setZero();
    }
    }

    return anmData;
}

const ResAnmChr::NodeData::AnmData *GetAnmRotation(f32 frame, EGG::Matrix34f &result,
        EGG::Vector3f &rawResult, const ResAnmChr::InfoData &infoData,
        const ResAnmChr::NodeData *nodeData, const ResAnmChr::NodeData::AnmData *anmData) {
    u32 flags = parse<u32>(nodeData->flags);

    switch (flags & ResAnmChr::NodeData::FLAG_ROT_FMT_MASK) {
    case 0: {
        f32 x = parse<f32>(anmData++->constValue);
        f32 y = parse<f32>(anmData++->constValue);
        f32 z = parse<f32>(anmData++->constValue);

        result.makeR({x, y, z});
        rawResult = EGG::Vector3f(x, y, z);
        break;
    }
    case ResAnmChr::NodeData::FLAG_ROT_FVS32_FMT: {
        f32 x = CalcResult32(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_X_CONST);
        f32 y = CalcResult32(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Y_CONST);
        f32 z = CalcResult32(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Z_CONST);

        result.makeR({x, y, z});
        rawResult = EGG::Vector3f(x, y, z);
        break;
    }
    case ResAnmChr::NodeData::FLAG_ROT_FVS48_FMT: {
        f32 x = CalcResult48(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_X_CONST);
        f32 y = CalcResult48(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Y_CONST);
        f32 z = CalcResult48(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Z_CONST);

        result.makeR({x, y, z});
        rawResult = EGG::Vector3f(x, y, z);
        break;
    }
    case ResAnmChr::NodeData::FLAG_ROT_FVS96_FMT: {
        f32 x = CalcResult96(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_X_CONST);
        f32 y = CalcResult96(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Y_CONST);
        f32 z = CalcResult96(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Z_CONST);

        result.makeR({x, y, z});
        rawResult = EGG::Vector3f(x, y, z);
        break;
    }
    case ResAnmChr::NodeData::FLAG_ROT_CV8_FMT: {
        f32 x = CalcResultFrm8(frame, nodeData, anmData++, parse<u16>(infoData.numFrame),
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_X_CONST);
        f32 y = CalcResultFrm8(frame, nodeData, anmData++, parse<u16>(infoData.numFrame),
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Y_CONST);
        f32 z = CalcResultFrm8(frame, nodeData, anmData++, parse<u16>(infoData.numFrame),
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Z_CONST);

        result.makeR({x, y, z});
        rawResult = EGG::Vector3f(x, y, z);
        break;
    }
    case ResAnmChr::NodeData::FLAG_ROT_CV16_FMT: {
        f32 x = CalcResultFrm16(frame, nodeData, anmData++, parse<u16>(infoData.numFrame),
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_X_CONST);
        f32 y = CalcResultFrm16(frame, nodeData, anmData++, parse<u16>(infoData.numFrame),
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Y_CONST);
        f32 z = CalcResultFrm16(frame, nodeData, anmData++, parse<u16>(infoData.numFrame),
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Z_CONST);

        result.makeR({x, y, z});
        rawResult = EGG::Vector3f(x, y, z);
        break;
    }
    case ResAnmChr::NodeData::FLAG_ROT_CV32_FMT: {
        f32 x = CalcResultFrm32(frame, nodeData, anmData++, parse<u16>(infoData.numFrame),
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_X_CONST);
        f32 y = CalcResultFrm32(frame, nodeData, anmData++, parse<u16>(infoData.numFrame),
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Y_CONST);
        f32 z = CalcResultFrm32(frame, nodeData, anmData++, parse<u16>(infoData.numFrame),
                flags & ResAnmChr::NodeData::Flag::FLAG_ROT_Z_CONST);

        result.makeR({x, y, z});
        rawResult = EGG::Vector3f(x, y, z);
        break;
    }
    default: {
        result.makeR(EGG::Vector3f::zero);
        rawResult.setZero();
    }
    }

    return anmData;
}

const ResAnmChr::NodeData::AnmData *GetAnmTranslation(f32 frame, EGG::Vector3f &trans,
        const ResAnmChr::NodeData *nodeData, const ResAnmChr::NodeData::AnmData *anmData) {
    u32 flags = parse<u32>(nodeData->flags);

    switch (flags & ResAnmChr::NodeData::FLAG_TRANS_FMT_MASK) {
    case 0: {
        f32 x = parse<f32>(anmData++->constValue);
        f32 y = parse<f32>(anmData++->constValue);
        f32 z = parse<f32>(anmData++->constValue);

        trans = EGG::Vector3f(x, y, z);
        break;
    }
    case ResAnmChr::NodeData::FLAG_ROT_FVS32_FMT: {
        f32 x = CalcResult32(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_TRANS_X_CONST);
        f32 y = CalcResult32(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_TRANS_Y_CONST);
        f32 z = CalcResult32(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_TRANS_Z_CONST);

        trans = EGG::Vector3f(x, y, z);
        break;
    }
    case ResAnmChr::NodeData::FLAG_ROT_FVS48_FMT: {
        f32 x = CalcResult48(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_TRANS_X_CONST);
        f32 y = CalcResult48(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_TRANS_Y_CONST);
        f32 z = CalcResult48(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_TRANS_Z_CONST);

        trans = EGG::Vector3f(x, y, z);
        break;
    }
    case ResAnmChr::NodeData::FLAG_ROT_FVS96_FMT: {
        f32 x = CalcResult96(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_TRANS_X_CONST);
        f32 y = CalcResult96(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_TRANS_Y_CONST);
        f32 z = CalcResult96(frame, nodeData, anmData++,
                flags & ResAnmChr::NodeData::Flag::FLAG_TRANS_Z_CONST);

        trans = EGG::Vector3f(x, y, z);
        break;
    }
    default: {
        trans.setZero();
    }
    }

    return anmData;
}

void GetAnmResult_(f32 /*frame*/, ChrAnmResult &result, const ResAnmChr::InfoData & /*infoData*/,
        const ResAnmChr::NodeData * /*nodeData*/) {
    result.s = EGG::Vector3f(1.0f, 1.0f, 1.0f);
    result.rt = EGG::Matrix34f::ident;
}

void GetAnmResult_S(f32 frame, ChrAnmResult &result, const ResAnmChr::InfoData & /*infoData*/,
        const ResAnmChr::NodeData *nodeData) {
    const auto *anmData = nodeData->anms;
    GetAnmScale(frame, result.s, nodeData, anmData);
    result.rt = EGG::Matrix34f::ident;
}

void GetAnmResult_R(f32 frame, ChrAnmResult &result, const ResAnmChr::InfoData &infoData,
        const ResAnmChr::NodeData *nodeData) {
    const auto *anmData = nodeData->anms;
    result.s = EGG::Vector3f(1.0f, 1.0f, 1.0f);
    GetAnmRotation(frame, result.rt, result.rawR, infoData, nodeData, anmData);
    result.flags |= ChrAnmResult::FLAG_ROT_RAW_FMT;
}

void GetAnmResult_SR(f32 frame, ChrAnmResult &result, const ResAnmChr::InfoData & /*infoData*/,
        const ResAnmChr::NodeData *nodeData) {
    const auto *anmData = nodeData->anms;
    anmData = GetAnmScale(frame, result.s, nodeData, anmData);
    result.rt = EGG::Matrix34f::ident;
    EGG::Vector3f t;
    GetAnmTranslation(frame, t, nodeData, anmData);
    result.rt.setBase(3, t);
}

void GetAnmResult_T(f32 frame, ChrAnmResult &result, const ResAnmChr::InfoData & /*infoData*/,
        const ResAnmChr::NodeData *nodeData) {
    const auto *anmData = nodeData->anms;
    EGG::Vector3f t;
    result.rt = EGG::Matrix34f::ident;
    GetAnmTranslation(frame, t, nodeData, anmData);
    result.rt.setBase(3, t);
}

void GetAnmResult_ST(f32 frame, ChrAnmResult &result, const ResAnmChr::InfoData & /*infoData*/,
        const ResAnmChr::NodeData *nodeData) {
    const auto *anmData = nodeData->anms;
    anmData = GetAnmScale(frame, result.s, nodeData, anmData);
    EGG::Vector3f t;
    result.rt = EGG::Matrix34f::ident;
    GetAnmTranslation(frame, t, nodeData, anmData);
    result.rt.setBase(3, t);
}

void GetAnmResult_RT(f32 frame, ChrAnmResult &result, const ResAnmChr::InfoData &infoData,
        const ResAnmChr::NodeData *nodeData) {
    const auto *anmData = nodeData->anms;
    EGG::Vector3f t;
    result.s = EGG::Vector3f(1.0f, 1.0f, 1.0f);
    anmData = GetAnmRotation(frame, result.rt, result.rawR, infoData, nodeData, anmData);
    result.flags |= ChrAnmResult::FLAG_ROT_RAW_FMT;
    GetAnmTranslation(frame, t, nodeData, anmData);
    result.rt.setBase(3, t);
}

void GetAnmResult_SRT(f32 frame, ChrAnmResult &result, const ResAnmChr::InfoData &infoData,
        const ResAnmChr::NodeData *nodeData) {
    const auto *anmData = nodeData->anms;
    EGG::Vector3f t;
    anmData = GetAnmScale(frame, result.s, nodeData, anmData);
    anmData = GetAnmRotation(frame, result.rt, result.rawR, infoData, nodeData, anmData);
    result.flags |= ChrAnmResult::FLAG_ROT_RAW_FMT;
    result.rt.setBase(3, t);
}

const std::array<ResAnmChr::GetAnmResultFunc, ResAnmChr::NUM_RESULT_FUNCS>
        ResAnmChr::s_getAnmResultTable = {{
                &GetAnmResult_,
                &GetAnmResult_S,
                &GetAnmResult_R,
                &GetAnmResult_SR,
                &GetAnmResult_T,
                &GetAnmResult_ST,
                &GetAnmResult_RT,
                &GetAnmResult_SRT,
        }};

} // namespace g3d
} // namespace Abstract
