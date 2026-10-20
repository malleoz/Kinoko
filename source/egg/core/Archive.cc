#include "Archive.hh"

#include <abstract/memory/List.hh>

namespace Kinoko::EGG {

/// @addr{0x8020FA38}
/// @brief Decrements the reference count of the archive and deletes it if the count reaches zero
void Archive::unmount() {
    if (--m_refCount <= 0) {
        egg_delete(this);
    }
}

/// @addr{0x8020FA80}
/// @brief Retrieves a file from the archive using its entry ID
/// @param entryId The entry ID of the file to retrieve
/// @param info The file info structure to be filled with details about the requested file
/// @return Pointer to the file data in memory, or `nullptr` if the file could not be found
const void *Archive::getFileFast(s32 entryId, Abstract::ArchiveHandle::FileInfo &info) const {
    m_handle.open(entryId, info);
    return m_handle.getFileAddress(info);
}

/// @addr{0x8020FA78}
/// @brief Gets the corresponding node entry ID for a given filepath in the archive
/// @param path The file path within the archive
/// @return The entry ID corresponding to the given file path, or `-1` if the path does not exist in
/// the archive
s32 Archive::convertPathToEntryId(const char *path) const {
    return m_handle.convertPathToEntryId(path);
}

/// @addr{Inlined in 0x8020F768}
/// @brief Checks to see if a given archive is already mounted
/// @param archiveStart The address of the archive to search for.
/// @return The archive if it is already mounted, or `nullptr` if it is not.
Archive *Archive::FindArchive(void *archiveStart) {
    ASSERT(archiveStart);

    auto *iter = reinterpret_cast<Archive *>(s_archiveList.getFirst());

    while (iter && iter->m_handle.startAddress() != archiveStart) {
        iter = reinterpret_cast<Archive *>(s_archiveList.getNext(iter));
    }

    return iter;
}

/// @addr{0x8020F768}
/// @brief Creates a new @ref Archive object or increments the ref count for an already existing
/// @ref Archive
/// @param archiveStart The address of the archive to mount.
/// @return Pointer to the archive, regardless if it is new or already exists
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
/// @brief Private constructor
/// @param archiveStart The address of the archive to mount
/// @details Constructs @ref m_handle based on the provided archive start address. Initializes @ref
/// m_refCount to 1.
Archive::Archive(void *archiveStart) : m_handle(archiveStart) {
    m_refCount = 1;
}

/// @addr{0x8020F6EC}
/// @brief Private virtual destructor that removes the archive from the static list
/// @details Called when the archive's reference count becomes 0.
Archive::~Archive() {
    s_archiveList.remove(this);
}

/// @brief Gets the offset of the link member within the @ref Archive class
/// @return The offset of the link member within the @ref Archive class
/// @details The C++ standard only guarantees `offsetof` for standard-layout types. Since @ref
/// Archive has a virtual destructor, `offset` of is not universally supported. Instead of
/// hardcoding an offset, we derive it ourselves.
constexpr uintptr_t Archive::GetLinkOffset() {
    // offsetof doesn't work, so instead of hardcoding an offset, we derive it ourselves
    return reinterpret_cast<uintptr_t>(&reinterpret_cast<Archive *>(NULL)->m_link);
}

Abstract::Memory::MEMList Archive::s_archiveList =
        Abstract::Memory::MEMList(Archive::GetLinkOffset());

} // namespace Kinoko::EGG
