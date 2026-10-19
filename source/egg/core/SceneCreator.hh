#pragma once

namespace Kinoko::EGG {

class Scene;

/// @brief Interface for creating and destroying scenes.
/// @details For Kinoko, the interface is used by the @ref SceneManager. The only class that
/// inherits from this interface is @ref Host::SceneCreatorDynamic.
class SceneCreator {
public:
    /// @brief Responsible for creating a new scene instance
    /// @param sceneID The ID of the scene to be created
    /// @return A pointer to the created scene instance
    virtual Scene *create(int sceneID) const = 0;

    /// @brief Responsible for destroying a scene instance
    /// @param sceneID The ID of the scene to be destroyed
    virtual void destroy(int sceneID) const = 0;
};

} // namespace Kinoko::EGG
