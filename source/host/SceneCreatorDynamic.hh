#pragma once

#include <egg/core/SceneCreator.hh>

/// @brief Represents the host application.
namespace Kinoko::Host {

enum class SceneId;

/// @brief Factory function which creates and destroys scenes based on their @ref SceneId
class SceneCreatorDynamic final : public EGG::SceneCreator {
public:
    [[nodiscard]] EGG::Scene *create(int sceneId) const override;
    void destroy(int sceneId) const override;

private:
    [[nodiscard]] static EGG::Scene *Create(SceneId sceneId);
    static void Destroy(SceneId sceneId);
};

} // namespace Kinoko::Host
