#pragma once

#include <Common.hh>

#include "egg/core/Heap.hh"

#include <abstract/Archive.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Abstract::Memory {

struct MEMList;

} // namespace Abstract::Memory

/// @brief EGG core library
namespace EGG {

class Archive : Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    void unmount();
    void *getFileFast(s32 entryId, Abstract::ArchiveHandle::FileInfo &info) const;
    [[nodiscard]] s32 convertPathToEntryId(const char *path) const;
    [[nodiscard]] static Archive *FindArchive(void *archiveStart);
    [[nodiscard]] static Archive *Mount(void *archiveStart);

private:
    EGG_NEW_DELETE_FRIEND

    Archive(void *archiveStart);
    ~Archive();

    [[nodiscard]] static constexpr uintptr_t GetLinkOffset();

    const Abstract::ArchiveHandle m_handle;
    s32 m_refCount;
    Abstract::Memory::MEMLink m_link;

    static Abstract::Memory::MEMList s_archiveList; ///< The linked list of all mounted archives.
};

} // namespace EGG

} // namespace Kinoko
