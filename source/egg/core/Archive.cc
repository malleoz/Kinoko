#include "Archive.hh"

#include <abstract/memory/List.hh>

namespace Kinoko::EGG {

/// @addr{0x8020FA38}
void Archive::unmount() {
    if (--m_refCount <= 0) {
        egg_delete(this);
    }
}

/// @addr{0x8020FA80}
void *Archive::getFileFast(s32 entryId, Abstract::ArchiveHandle::FileInfo &info) const {
    m_handle.open(entryId, info);
    return m_handle.getFileAddress(info);
}

/// @addr{0x8020FA78}
s32 Archive::convertPathToEntryId(const char *path) const {
    return m_handle.convertPathToEntryId(path);
}

/// @addr{Inlined in 0x8020F768}
/// @brief Checks to see if a given archive is already mounted.
/// @param archiveStart The address of the archive to search for.
/// @return The archive if it is already mounted, or nullptr if it is not.
Archive *Archive::FindArchive(void *archiveStart) {
    ASSERT(archiveStart);

    auto *iter = reinterpret_cast<Archive *>(s_archiveList.getFirst());

    while (iter && iter->m_handle.startAddress() != archiveStart) {
        iter = reinterpret_cast<Archive *>(s_archiveList.getNext(iter));
    }

    return iter;
}

/// @addr{0x8020F768}
/// @brief Creates a new Archive object or increments the ref count for an already existing Archive.
/// @param archiveStart The address of the archive to mount.
/// @return The Archive, regardless if it is new or already exists.
Archive *Archive::Mount(void *archiveStart) {
    Archive *archive = FindArchive(archiveStart);

    if (!archive) {
        // Create a new archive and add it to the list
        archive = EGG::egg_new<Archive>(archiveStart);
        s_archiveList.append(archive);
    } else {
        // It already exists, increase the reference count
        archive->m_refCount++;
    }

    return archive;
}

/// @addr{Inlined in 0x8020F768}
Archive::Archive(void *archiveStart) : m_handle(archiveStart) {
    m_refCount = 1;
}

/// @addr{0x8020F6EC}
/// @brief Removes the archive from the static list.
/// @details Called when the archive's reference count becomes 0.
Archive::~Archive() {
    s_archiveList.remove(this);
}

constexpr uintptr_t Archive::GetLinkOffset() {
    // offsetof doesn't work, so instead of hardcoding an offset, we derive it ourselves
    return reinterpret_cast<uintptr_t>(&reinterpret_cast<Archive *>(NULL)->m_link);
}

Abstract::Memory::MEMList Archive::s_archiveList =
        Abstract::Memory::MEMList(Archive::GetLinkOffset());

} // namespace Kinoko::EGG
