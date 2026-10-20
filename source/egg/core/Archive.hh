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

/// @brief Represents an archive (`.szs` file) loaded in memory
/// @details Implements refcounting for archives, allowing for safe destruction when no longer in
/// use. Exposes methods for mounting, unmounting, and accessing files within the archive.
class Archive : Disposer {
    /// @brief Grants access to the static archive list so a @ref Host::Context can restore the
    /// list on context switch
    friend Host::Context;

public:
    void unmount();
    const void *getFileFast(s32 entryId, Abstract::ArchiveHandle::FileInfo &info) const;
    [[nodiscard]] s32 convertPathToEntryId(const char *path) const;

    [[nodiscard]] static Archive *FindArchive(void *archiveStart);
    [[nodiscard]] static Archive *Mount(void *archiveStart);

private:
    EGG_NEW_DELETE_FRIEND

    Archive(void *archiveStart);
    ~Archive() override;

    [[nodiscard]] static constexpr uintptr_t GetLinkOffset();

    const Abstract::ArchiveHandle m_handle; ///< The handle to the archive in memory
    s32 m_refCount; ///< Reference count. When it reaches 0, the archive is destroyed
    Abstract::Memory::MEMLink m_link; ///< This archive's link in the static archive linked list

    static Abstract::Memory::MEMList s_archiveList; ///< The linked list of all mounted archives.
};

} // namespace EGG

} // namespace Kinoko
