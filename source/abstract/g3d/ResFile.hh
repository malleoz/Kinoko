#pragma once

#include "abstract/g3d/ResAnmChr.hh"
#include "abstract/g3d/ResDic.hh"

// Credit: kiwi515/ogws

namespace Kinoko::Abstract::g3d {

/// @brief Represents a binary resource file (`.brres`) which contains object models, textures, and
/// animations.
/// @details All resource files start with a header of length `0x10`. The header is followed by a
/// root section (represented as @ref TopLevelDictData) which uses a @ref ResDic to point to
/// different folders of the `.brres` file.
class ResFile {
public:
    /// @brief The "root" section of the file
    struct TopLevelDictData {
        ResBlockHeaderData header; ///< The header of the root section
        ResDic::Data topLevel;     ///< The top-level dictionary of the root section
    };

    /// @brief The combined header and top-level dictionary of the file
    struct Data {
        ResFileHeaderData header; ///< The header of the resource file
        TopLevelDictData dict;    ///< The root section of the resource file
    };

    /// @brief Constructor
    /// @param data Pointer to the binary data of the resource file
    /// @details Sets @ref m_data to `data`.
    ResFile(const void *data) : m_data(reinterpret_cast<const Data *>(data)) {}

    /// @brief Default destructor
    ~ResFile() = default;

    /// @addr{0x8004C780}
    /// @brief Retrieves the AnmChr section from the binary file.
    /// @param pName The name of the AnmChr resource to retrieve
    /// @return A @ref ResAnmChr object representing the requested AnmChr resource
    /// @details Looks up the "AnmChr(NW4R)" folder in the top-level dictionary and retrieves the
    /// resource with the specified name.
    /// @warning Panics if the "AnmChr(NW4R)" folder or the specified resource is not found.
    [[nodiscard]] ResAnmChr resAnmChr(const char *pName) const {
        ResDic dic = ResDic(&m_data->dict.topLevel);
        void *dicData = dic["AnmChr(NW4R)"];
        ASSERT(dicData);
        void *anmChrData = ResDic(dicData)[pName];
        ASSERT(anmChrData);
        return ResAnmChr(anmChrData);
    }

private:
    const Data *const m_data; ///< Pointer to the binary data of the resource file
};

} // namespace Kinoko::Abstract::g3d
