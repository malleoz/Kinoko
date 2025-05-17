#pragma once

#include "abstract/g3d/AnmObj.hh"
#include "abstract/g3d/ResCommon.hh"
#include "abstract/g3d/ResDic.hh"

#include <egg/math/Matrix.hh>
#include <egg/util/Stream.hh>

// Credit: kiwi515/ogws

namespace Abstract {
namespace g3d {

class ChrAnmResult {
public:
    struct Data {
        u32 flags;
        EGG::Vector3f scale;
        EGG::Vector3f _10;
        EGG::Matrix34f mat;
    };

    /// @addr{0x800555C0}
    [[nodiscard]] EGG::Vector3f scale() const {
        const EGG::Vector3f rawScale = m_data.scale;
        f32 x = parse<f32>(rawScale.x);
        f32 y = parse<f32>(rawScale.y);
        f32 z = parse<f32>(rawScale.z);

        return EGG::Vector3f(x, y, z);
    }

    Data m_data;
};

/// @brief Represents the CHR0 file format, which pertains to model movement animations.
class ResAnmChr {
public:
    struct InfoData {
        void read(EGG::Stream &stream) {
            numFrame = stream.read_u16();
            numNode = stream.read_u16();
            policy = static_cast<AnmPolicy>(stream.read_u32());
            scalingRule = stream.read_u32();
        }

        u16 numFrame;
        u16 numNode;
        AnmPolicy policy;
        u32 scalingRule;
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

    ResAnmChr(const void *data) : m_rawData(reinterpret_cast<const Data *>(data)) {
        EGG::RamStream stream = EGG::RamStream(data, sizeof(Data));
        read(stream);
    }

    void read(EGG::Stream &stream) {
        stream.jump(offsetof(Data, info));
        m_infoData.read(stream);
    }

    /// @addr{0x80055540}
    [[nodiscard]] ChrAnmResult getAnmResult(f32 frame, size_t idx) const {
        ResDic dic = ResDic(reinterpret_cast<void *>(
                reinterpret_cast<uintptr_t>(m_rawData) + parse<s32>(m_rawData->toChrDataDic)));

        const auto *data = dic.get(idx);

        // TODO
        ChrAnmResult result;
        result.m_data.flags = parse<u16>(data->idxRight) & 0x1fff;
        f32 frameCount = parse<f32>(m_rawData->info.numFrame);

        return result;
    }

    [[nodiscard]] u16 frameCount() const {
        return m_infoData.numFrame;
    }

    [[nodiscard]] AnmPolicy policy() const {
        return m_infoData.policy;
    }

private:
    const Data *m_rawData;
    InfoData m_infoData;
};

class AnmObjChrRes : public FrameCtrl {
public:
    AnmObjChrRes(const ResAnmChr &chr)
        : FrameCtrl(0.0f, chr.frameCount(), GetAnmPlayPolicy(chr.policy())), m_resAnmChr(chr) {}

    [[nodiscard]] u16 frameCount() const {
        return m_resAnmChr.frameCount();
    }

private:
    ResAnmChr m_resAnmChr;
};

} // namespace g3d
} // namespace Abstract
