#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectVolcanoBall;

/// @brief The manager class that launch fireballs on Grumble Volcano.
/// @details Lies dormant until @ref m_initDelay frames have elapsed. Then, every @ref
/// m_cycleDuration frames, a fireball is launched. The launcher rotates through an array of
/// fireballs to launch.
class ObjectVolcanoBallLauncher final : public ObjectCollidable {
public:
    ObjectVolcanoBallLauncher(const System::MapdataGeoObj &params);
    ~ObjectVolcanoBallLauncher() override;

    void init() override;
    void calc() override;

    /// @addr{0x806E3A74}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    };

    /// @addr{0x806E3A70}
    /// @details no-op because the launcher does not have any graphics to load
    void loadGraphics() override {}

    /// @addr{0x806E3A68}
    /// @details no-op because the launcher itself does not have any collision
    void createCollision() override {}

    /// @addr{0x806E3A6C}
    /// @details no-op because the launcher itself does not follow any rail path
    void loadRail() override {}

private:
    owning_span<ObjectVolcanoBall *> m_balls; ///< Array of fireballs managed by the launcher
    const f32 m_initDelay;     ///< Frames the launcher lays dormant at the start of the race
    const f32 m_cycleDuration; ///< Frames between consecutive fireball launches
    u32 m_currBallIdx;         ///< Index of the next fireball to be launched
    bool m_active;             ///< False when the volcano is dormant, true after @ref m_initDelay
};

} // namespace Kinoko::Field
