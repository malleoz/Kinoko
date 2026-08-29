#pragma once

#include "host/SceneId.hh"

#include <egg/core/SceneCreator.hh>

#include <game/scene/RaceScene.hh>
#include <game/scene/RootScene.hh>

/// @brief Represents the host application.
namespace Kinoko::Host {

class SceneCreatorDynamic final : public EGG::SceneCreator {
public:
    [[nodiscard]] EGG::Scene *create(int sceneId) const override {
        return create(static_cast<SceneId>(sceneId));
    }

    void destroy(int sceneId) const override {
        destroy(static_cast<SceneId>(sceneId));
    }

private:
    /// @addr{0x8054AA64}
    [[nodiscard]] EGG::Scene *create(SceneId sceneId) const {
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
    void destroy(SceneId sceneId) const {
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
