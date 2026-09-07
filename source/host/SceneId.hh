#pragma once

namespace Kinoko::Host {

/// @brief Describes the type of @ref EGG::Scene
/// @details This is used by the @ref SceneCreatorDynamic::Create() factory function to determine
/// what type of scene to create.
enum class SceneId {
    Root = 0, ///< The root scene, the parent of all other scenes
    Race = 2, ///< Scene representing a race
};

} // namespace Kinoko::Host
