#pragma once

#include "game/kart/KartParam.hh"

#include "game/system/ResourceManager.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Kart {

/// @brief Abstraction for the process of retrieving kart parameters from files
/// @details Hooks into the following parameter files:
/// - `kartParam.bin` (@ref m_kartParam)
/// - `driverParam.bin` (@ref m_driverParam)
/// - `bikePartsDispParam.bin` (@ref m_bikeDispParam)
/// - `kartPartsDispParam.bin` (@ref m_kartDispParam)
/// - `kartCameraParam.bin` (@ref m_kartCameraParam)
/// @note This has been modified from the base game in order to perform validation and make the
/// class accessible as a singleton.
class KartParamFileManager : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /// @addr{0x80591C9C}
    /// @brief Clears all loaded parameter files from the manager (but does not clear them from
    /// memory)
    void clear() {
        m_kartParam = {};
        m_driverParam = {};
        m_bikeDispParam = {};
        m_kartDispParam = {};
        m_kartCameraParam = {};
    }

    /// @addr{0x805919F4}
    /// @brief Loads and validates the kart parameter files.
    void init() {
        m_kartParam = load("kartParam.bin");
        m_driverParam = load("driverParam.bin");
        m_bikeDispParam = load("bikePartsDispParam.bin");
        m_kartDispParam = load("kartPartsDispParam.bin");
        m_kartCameraParam = load("kartCameraParam.bin");
        if (!validate()) {
            PANIC("Parameter files could not be validated!");
        }
    }

    [[nodiscard]] EGG::RamStream getDriverStream(Character character) const;
    [[nodiscard]] EGG::RamStream getVehicleStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getHitboxStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getBikeDispParamsStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getKartDispParamsStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getKartCameraStream(Character character) const;

    /// @brief Creates the singleton instance of the @ref KartParamFileManager.
    /// @return A pointer to the newly created @ref KartParamFileManager instance.
    /// @pre Requires that the singleton instance has not already been created.
    static KartParamFileManager *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<KartParamFileManager>();
        return s_instance;
    }

    /// @brief Destroys the singleton instance of the @ref KartParamFileManager.
    /// @pre Requires that the singleton instance has already been created.
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref KartParamFileManager.
    /// @return The singleton instance of the @ref KartParamFileManager.
    [[nodiscard]] static KartParamFileManager *Instance() {
        return s_instance;
    }

private:
    /// @brief A template struct that represents a parameter file containing a `T` array with length
    /// `count`.
    /// @tparam T The type of the struct that is stored in the parameter file.
    /// @details `kartParam.bin`, `bikePartsDispParam.bin`, and `kartPartsDispParam.bin` all contain
    /// an array of entries. The number of array elements is stored in the first 4 bytes of the
    /// file, and the rest of the file contains the array of structs. This struct is used to
    /// represent that data in memory and enforce that array accesses are within bounds.
    template <typename T>
    struct ParamFile {
        u32 count;
        T params[];

        /// @brief Returns a reference to the `T` at the given index in the `@ref params` array.
        /// @param index The index in the @ref params at which to get the `T` element
        /// @return A const reference to the `T` at the given index in the `@ref params` array.
        /// @pre Requires that `index` is in-bounds (less than @ref count).
        const T &operator[](u32 index) const {
            ASSERT(index < count);
            return params[index];
        }
    };

    EGG_NEW_DELETE_FRIEND

    /// @brief Private constructor that loads and validates the kart parameter files
    /// @details Simply calls the @ref init() method to load and validate the kart parameter files.
    KartParamFileManager() {
        init();
    }

    /// @brief Private virtual destructor
    ~KartParamFileManager() override {
        if (s_instance) {
            s_instance = nullptr;
            WARN("KartParamFileManager instance not explicitly handled!");
        }
    }

    [[nodiscard]] bool validate() const;

    /// @brief Loads the provided filename from the core archive (`Common.szs`) into memory
    /// @param filename The name of the file to load from the core archive
    /// @return A read-only span of the file data in memory
    [[nodiscard]] std::span<const u8> load(const char *filename) {
        return System::ResourceManager::Instance()->getFile(filename, System::ArchiveId::Core);
    }

    std::span<const u8> m_kartParam;       ///< File pointer and size for `kartParam.bin`
    std::span<const u8> m_driverParam;     ///< File pointer and size for `driverParam.bin`
    std::span<const u8> m_bikeDispParam;   ///< File pointer and size for `bikePartsDispParam.bin`
    std::span<const u8> m_kartDispParam;   ///< File pointer and size for `kartPartsDispParam.bin`
    std::span<const u8> m_kartCameraParam; ///< File pointer and size for `kartCameraParam.bin`

    /// @brief Pointer to the singleton instance of the @ref KartParamFileManager
    /// @note This is not actually a singleton in the base game, but since we only implement one
    /// player instance in Kinoko, we can manage this class as if it were a singleton.
    static KartParamFileManager *s_instance;
};

} // namespace Kart

} // namespace Kinoko
