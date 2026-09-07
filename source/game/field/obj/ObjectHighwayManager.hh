#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectCarTGE;

/// @brief Watcher class that checks if any car has squashed the player
/// @details This squash cooldown is used by @ref ObjectCarTGE to determine if the player can be
/// squashed again.
class ObjectHighwayManager final : public ObjectCollidable {
public:
    ObjectHighwayManager();
    ~ObjectHighwayManager() override;

    /// @addr{0x806D332C}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_squashTimer = SQUASH_MAX;
    }

    /// @addr{0x806D345C}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        calcSquash();
    }

    /// @addr{0x806D5C6C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806D5C68}
    /// @copybrief ObjectBase::loadGraphics()
    /// @details Does nothing since this is just a watcher class
    void loadGraphics() override {}

    /// @addr{0x806D5C60}
    /// @copybrief ObjectBase::createCollision()
    /// @details Does nothing since this is just a watcher class
    void createCollision() override {}

    /// @addr{0x806D5C64}
    /// @copybrief ObjectBase::loadRail()
    /// @details Does nothing since this is just a watcher class
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
