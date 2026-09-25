#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectProjectileLauncher.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief Represents the sun on DS Desert Hills that launches @ref ObjectFireSnake projectiles
/// @details Follows a rail, stopping for a duration specified by the rail point. @ref
/// ObjectSunManager interfaces with this class via @ref launchPointIdx() to determine if a @ref
/// ObjectFireSnake projectile should be launched.
class ObjectSunDS final : public ObjectProjectileLauncher, private StateManager {
public:
    /// @addr{0x806DDDD8}
    /// @copybrief ObjectProjectileLauncher::ObjectProjectileLauncher(const System::MapdataGeoObj &)
    /// @details Sets @ref m_revolutionSpeed based on param setting 1 and @ref m_startFrame based on
    /// param setting 2.
    ObjectSunDS(const System::MapdataGeoObj &params)
        : ObjectProjectileLauncher(params),
          StateManager(this, STATE_ENTRIES),
          m_revolutionSpeed(static_cast<f32>(params.setting(0))),
          m_startFrame(static_cast<s32>(params.setting(1))) {}

    /// @addr{0x806DDF68}
    /// @brief Default virtual destructor
    ~ObjectSunDS() override = default;

    /// @addr{0x806DDFD4}
    /// @copybrief ObjectBase::init()
    /// @details Initializes @ref m_stillDuration to zero.
    void init() override {
        m_stillDuration = 0;
    }

    /// @addr{0x806DE03C}
    /// @copybrief ObjectBase::calc()
    /// @details Early returns if @ref m_startFrame frames have not elapsed in the race yet. Updates
    /// the sun's rail position, checking to see if the sun should stop. Evaluates the sun's state
    /// machine. Finally, updates the sun's position based on the rail interpolator's current
    /// position.
    void calc() override {
        if (System::RaceManager::Instance()->timer() < static_cast<u32>(m_startFrame)) {
            return;
        }

        calcRail();
        StateManager::calc();
        calcPos();
    }

    /// @addr{0x806DE614}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806DE598}
    /// @copybrief ObjectProjectileLauncher::launchPointIdx()
    /// @return The index of the rail point from which to launch a @ref ObjectFireSnake, or -1 if no
    /// @ref ObjectFireSnake should be launched this frame.
    /// @details If the sun is currently in the revolving state, or if the sun has not been
    /// stationary for 30 frames, then returns `-1`. Otherwise, returns the rail interpolator's
    /// current point index.
    [[nodiscard]] s16 launchPointIdx() const override {
        constexpr u16 THROW_DELAY = 30;

        if (m_currentStateId != 0 || THROW_DELAY != m_currentFrame) {
            return -1;
        }

        return m_railInterpolator->curPointIdx();
    }

private:
    /// @addr{0x806DE1C8}
    /// @brief Runs once when the sun has reached a point along its rail designated as a stop point
    /// @details Sets the rail interpolator's speed to zero.
    void enterStill() {
        m_railInterpolator->setSpeed(0.0f);
    }

    /// @addr{0x806DE408}
    /// @brief Runs once when the sun begins revolving around the course again
    /// @details Sets the rail interpolator's speed to @ref m_revolutionSpeed
    void enterRevolving() {
        m_railInterpolator->setSpeed(m_revolutionSpeed);
    }

    /// @addr{0x806DE1E4}
    /// @brief Runs every frame that the sun is stationary
    /// @details If the sun has been stationary for @ref m_stillDuration frames, then transitions
    /// the sun to the revolving state.
    void calcStill() {
        if (m_currentFrame >= m_stillDuration) {
            m_nextStateId = 1;
        }
    }

    /// @addr{0x806DE458}
    /// @brief Updates the rail interpolator and checks to see if the sun should stop
    /// @details Updates the rail interpolator. If the interpolator has reached the end of a rail
    /// segment, then snaps the rail interpolator to the start of the new rail segment and calls
    /// @ref checkStop() to see if the sun should transition to the still state.
    void calcRail() {
        if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd) {
            m_railInterpolator->setT(0.0f);
            checkStop();
        }
    }

    /// @addr{0x806DE4E4}
    /// @brief Updates the sun's position based on the current rail interpolation
    void calcPos() {
        setPos(m_railInterpolator->curPos());
    }

    /// @addr{0x806DE568}
    /// @brief Checks if the current rail point is a stop point
    /// @details Stop points are identified based on the rail node's first setting being non-zero.
    /// This setting specifies the number of frames the sun should remain stationary for. If this
    /// setting is non-zero, sets @ref m_stillDuration based on the setting's value and transitions
    /// the sun to the still state.
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

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            {StateEntry<ObjectSunDS, &ObjectSunDS::enterStill, &ObjectSunDS::calcStill>(0)},
            {StateEntry<ObjectSunDS, &ObjectSunDS::enterRevolving, nullptr>(1)},
    }};
};

} // namespace Kinoko::Field
