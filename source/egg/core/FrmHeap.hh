#pragma once

#include "egg/core/Heap.hh"

#include <abstract/memory/FrameHeap.hh>

namespace Kinoko::EGG {

/// @brief A high-level representation of a frame heap, which allocates sequentially and cannot free
/// individual blocks.
class FrmHeap final : public Heap {
public:
    void destroy() override;

    /// @copybrief Heap::getHeapKind()
    /// @return @ref Kind::Frame
    [[nodiscard]] Kind getHeapKind() const override {
        return Kind::Frame;
    }

    [[nodiscard]] void *alloc(size_t size, s32 align) override;
    void free(void *block) override;
    [[nodiscard]] u32 getAllocatableSize(s32 align = 4) const override;

    [[nodiscard]] Abstract::Memory::MEMiFrmHeapHead *dynamicCastHandleToFrm();
    [[nodiscard]] const Abstract::Memory::MEMiFrmHeapHead *dynamicCastHandleToFrm() const;

    [[nodiscard]] static FrmHeap *Create(void *startAddress, size_t size, u16 opt);
    [[nodiscard]] static FrmHeap *Create(size_t size, Heap *pHeap, u16 opt);

private:
    FrmHeap(Abstract::Memory::MEMiHeapHead *handle);
    ~FrmHeap() override;
};

} // namespace Kinoko::EGG
