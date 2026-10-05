#pragma once

#include "game/scene/GameScene.hh"

namespace Kinoko::Scene {

/// @brief Represents the scene for an instance of a race
class RaceScene final : public GameScene {
public:
    /// @addr{0x80553B88}
    /// @brief Constructor
    /// @details Sets the @ref m_heap name to `"RaceSceneHeap"` for debugging/logging purposes.
    RaceScene() {
        m_heap->setName("RaceSceneHeap");
    }

    /// @addr{0x80553BD4}
    /// @brief Default virtual destructor
    ~RaceScene() override = default;

    void createEngines() override;
    void initEngines() override;
    void calcEngines() override;
    void destroyEngines() override;
    void configure() override;

    /// @addr{0x80554A94}
    /// @copybrief GameScene::onReinit
    /// @details Simply calls @ref configure().
    void onReinit() override {
        configure();
    }
};

} // namespace Kinoko::Scene
