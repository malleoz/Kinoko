#pragma once

#include <Common.hh>
#include <Types.hh>

#include <egg/core/Heap.hh>

#include <span>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace System {

class MultiDvdArchive;

/// @brief Unique identifiers for the archives loaded by @ref ResourceManager
enum class ArchiveId {
    Core = 0,   ///< Represents `Common.szs`
    Course = 1, ///< Represents course-specific archives
};

/// @addr{0x809BD738}
/// @brief Highest level abstraction for archive management and subsequent file retrieval
/// @details ResourceManager is responsible for loading and unloading archives. For example, it is
/// used by @ref Field::CourseColMgr to load the KCL collision file from a particular course
/// archive.
class ResourceManager : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    [[nodiscard]] std::span<const u8> getFile(const char *filename, ArchiveId id);
    [[nodiscard]] std::span<const u8> getBsp(Vehicle vehicle);
    [[nodiscard]] MultiDvdArchive *load(Course courseId);
    [[nodiscard]] MultiDvdArchive *load(s32 idx, const char *filename);
    void unmount(MultiDvdArchive *archive);

    [[nodiscard]] static const char *GetVehicleName(Vehicle vehicle);

    static ResourceManager *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static ResourceManager *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    ResourceManager();
    ~ResourceManager() override;

    fixed_vector<MultiDvdArchive *> m_archives; ///< One archive for each @ref ArchiveId

    [[nodiscard]] static MultiDvdArchive *Create(u8 i);

    /// @brief The total number of archives managed by the ResourceManager
    static constexpr size_t ARCHIVE_COUNT = 2;

    /// @addr{0x809BD738}
    /// @brief The singleton instance of the @ref ResourceManager
    static ResourceManager *s_instance;
};

} // namespace System

} // namespace Kinoko
