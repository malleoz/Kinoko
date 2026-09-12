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
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectSunDS(const System::MapdataGeoObj &params)
        : ObjectProjectileLauncher(params),
          StateManager(this, STATE_ENTRIES),
          m_revolutionSpeed(static_cast<f32>(params.setting(0))),
          m_startFrame(static_cast<s32>(params.setting(1))) {}

    /// @addr{0x806DDF68}
    /// @brief Default virtual destructor
    ~ObjectSunDS() = default;

    /// @addr{0x806DDFD4}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_stillDuration = 0;
    }

    /// @addr{0x806DE03C}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the sun's position along its rail. If the sun reaches a stop point, it
    /// transitions to the still state.
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
    /// @copydoc ObjectProjectileLauncher::launchPointIdx()
    [[nodiscard]] s16 launchPointIdx() override {
        constexpr u16 THROW_DELAY = 30;

        if (m_currentStateId != 0 || THROW_DELAY != m_currentFrame) {
            return -1;
        }

        return m_railInterpolator->curPointIdx();
    }

private:
    /// @addr{0x806DE1C8}
    /// @brief Runs once when the sun has reached a point along its rail designated as a stop point
    void enterStill() {
        m_railInterpolator->setSpeed(0.0f);
    }

    /// @addr{0x806DE408}
    /// @brief Runs once when the sun begins revolving around the course again
    void enterRevolving() {
        m_railInterpolator->setSpeed(m_revolutionSpeed);
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

    /// @addr{0x806DE458}
    /// @brief Updates the sun's position along its rail and handles stop points
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
            {StateEntry<ObjectSunDS, &ObjectSunDS::enterRevolving, &ObjectSunDS::calcRevolving>(1)},
    }};
};

} // namespace Kinoko::Field
