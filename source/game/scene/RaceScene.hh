#pragma once

#include "game/scene/GameScene.hh"

namespace Kinoko::Scene {

/// @brief Represents an instance of a race
class RaceScene final : public GameScene {
public:
    RaceScene();
    ~RaceScene() override;

    void createEngines() override;
    void initEngines() override;
    void calcEngines() override;
    void destroyEngines() override;
    void configure() override;

    /// @addr{0x80554A94}
    /// @copybrief GameScene::onReinit
    void onReinit() override {
        configure();
    }
};

} // namespace Kinoko::Scene
