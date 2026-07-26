#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectProjectileLauncher.hh"

namespace Kinoko::Field {

/// @brief Represents the sun on DS Desert Hills that launches @ref ObjectFireSnake projectiles
/// @details Follows a rail, stopping for a duration specified by the rail point. @ref
/// ObjectSunManager interfaces with this class via @ref launchPointIdx() to determine if a @ref
/// ObjectFireSnake projectile should be launched.
class ObjectSunDS : public ObjectProjectileLauncher, public StateManager {
public:
    ObjectSunDS(const System::MapdataGeoObj &params);
    ~ObjectSunDS() override;

    /// @addr{0x806DDFD4}
    void init() override {
        m_stillDuration = 0;
    }

    void calc() override;

    /// @addr{0x806DE614}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    [[nodiscard]] s16 launchPointIdx() override;

private:
    /// @addr{0x806DE1C8}
    /// @brief Runs once when the sun has reached a point along its rail designated as a stop point
    void enterStill() {
        m_railInterpolator->setCurrVel(0.0f);
    }

    /// @addr{0x806DE408}
    /// @brief Runs once when the sun begins revolving around the course again
    void enterRevolving() {
        m_railInterpolator->setCurrVel(m_revolutionSpeed);
    }

    /// @addr{0x806DE1E4}
    /// @brief Runs every frame that the sun is stationary
    void calcStill() {
        if (m_currentFrame >= m_stillDuration) {
            m_nextStateId = 1;
        }
    }

    /// @addr{0x806DE454}
    void calcRevolving() {}

    void calcRail();

    /// @addr{0x806DE4E4}
    /// @brief Updates the sun's position based on the current rail interpolation
    void calcPos() {
        setPos(m_railInterpolator->curPos());
    }

    /// @addr{0x806DE568}
    /// @brief Checks if the current rail point is a stop point
    void checkStop() {
        u16 setting = m_railInterpolator->curPoint().setting[0];
        if (setting != 0) {
            m_stillDuration = setting;
            m_nextStateId = 0;
        }
    }

    const f32 m_revolutionSpeed; ///< Speed of sun revolving around the course
    const s32 m_startFrame;      ///< The sun is inactive until this frame
    u32 m_stillDuration;         ///< How long the sun remains stationary for before moving again

    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            {StateEntry<ObjectSunDS, &ObjectSunDS::enterStill, &ObjectSunDS::calcStill>(0)},
            {StateEntry<ObjectSunDS, &ObjectSunDS::enterRevolving, &ObjectSunDS::calcRevolving>(1)},
    }};
};

} // namespace Kinoko::Field
