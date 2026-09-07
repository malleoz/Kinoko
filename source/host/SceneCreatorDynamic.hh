#pragma once

#include "host/SceneId.hh"

#include <egg/core/SceneCreator.hh>

#include <game/scene/RaceScene.hh>
#include <game/scene/RootScene.hh>

/// @brief Represents the host application.
namespace Kinoko::Host {

/// @brief Factory function which creates and destroys scenes based on their @ref SceneId
class SceneCreatorDynamic final : public EGG::SceneCreator {
public:
    /// @copydoc EGG::SceneCreator::create()
    [[nodiscard]] EGG::Scene *create(int sceneId) const override {
        return Create(static_cast<SceneId>(sceneId));
    }

    /// @copydoc EGG::SceneCreator::destroy()
    void destroy(int sceneId) const override {
        Destroy(static_cast<SceneId>(sceneId));
    }

private:
    /// @addr{0x8054AA64}
    /// @brief Factory function which creates the scene corresponding to the given @ref SceneId
    /// @param sceneId The @ref SceneId of the scene to create
    /// @return A pointer to the newly created scene
    [[nodiscard]] static EGG::Scene *Create(SceneId sceneId) {
        switch (sceneId) {
        case SceneId::Root:
            return EGG::egg_new<Scene::RootScene>();
        case SceneId::Race:
            return EGG::egg_new<Scene::RaceScene>();
        default:
            PANIC("Unreachable scene creation!");
        }
    }

    /// @addr{0x8054AB28}
    /// @brief Destroys the scene corresponding to the given @ref SceneId
    /// @param sceneId The @ref SceneId of the scene to destroy
    /// @details The base game doesn't actually do anything in this function, so we don't either.
    static void Destroy(SceneId sceneId) {
        switch (sceneId) {
        case SceneId::Root:
        case SceneId::Race:
            // The base game doesn't do anything, so we don't either
            break;
        default:
            PANIC("Unreachable scene deletion!");
        }
    }
};

} // namespace Kinoko::Host
