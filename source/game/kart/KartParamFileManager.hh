#pragma once

#include <Common.hh>

#include <egg/core/Heap.hh>

#include <span>

namespace Kinoko {

namespace EGG {

class RamStream;

} // namespace EGG

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
    friend Host::Context;

public:
    void clear();
    void init();

    [[nodiscard]] EGG::RamStream getDriverStream(Character character) const;
    [[nodiscard]] EGG::RamStream getVehicleStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getHitboxStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getBikeDispParamsStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getKartDispParamsStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getKartCameraStream(Character character) const;

    static KartParamFileManager *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static KartParamFileManager *Instance();

private:
    /// @brief A template struct that represents a parameter file containing a `T` array with
    /// length `count`.
    /// @tparam T The type of the struct that is stored in the parameter file.
    /// @details `kartParam.bin`, `bikePartsDispParam.bin`, and `kartPartsDispParam.bin` all
    /// contain an array of entries. The number of array elements is stored in the first 4 bytes
    /// of the file, and the rest of the file contains the array of structs. This struct is used
    /// to represent that data in memory and enforce that array accesses are within bounds.
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

    KartParamFileManager();
    ~KartParamFileManager() override;

    [[nodiscard]] bool validate() const;

    [[nodiscard]] static std::span<const u8> Load(const char *filename);

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
