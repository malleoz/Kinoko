#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectCarTGE;

/// @brief Watcher class that checks if any car has squashed the player
/// @details This squash cooldown is used by @ref ObjectCarTGE to determine if the player can be
/// squashed again.
class ObjectHighwayManager : public ObjectCollidable {
public:
    ObjectHighwayManager();
    ~ObjectHighwayManager() override;

    void init() override;
    void calc() override;

    /// @addr{0x806D5C6C}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806D5C68}
    /// @brief Does nothing since this is just a watcher class
    void loadGraphics() override {}

    /// @addr{0x806D5C60}
    /// @brief Does nothing since this is just a watcher class
    void createCollision() override {}

    /// @addr{0x806D5C64}
    /// @brief Does nothing since this is just a watcher class
    void loadRail() override {}

    [[nodiscard]] u32 squashTimer() const {
        return m_squashTimer;
    }

private:
    void calcSquash();

    owning_span<ObjectCarTGE *> m_cars; ///< Pointers to all car objects
    u32 m_squashTimer;                  ///< Normally an array, one for each player

    static constexpr u32 SQUASH_MAX = 600; ///< Squash timer cap
};

} // namespace Kinoko::Field
