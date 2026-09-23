#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents a walking Goomba
/// @details Goombas walk along a rail. Once they reach the end of the rail, they reverse direction.
class ObjectKuribo final : public ObjectCollidable, private StateManager {
public:
    /// @addr{0x806DB184}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Initializes the Goomba's acceleration based on param setting 2 divided by `100.0f`.
    /// Also initializes the animation rate based on param setting 3 divided by `100.0f`.
    ObjectKuribo(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          StateManager(this, STATE_ENTRIES),
          m_accel(static_cast<f32>(params.setting(1)) / 100.0f),
          m_animRate(static_cast<f32>(params.setting(2)) / 100.0f) {}

    /// @addr{0x806DB3A0}
    /// @brief Default virtual destructor
    ~ObjectKuribo() override = default;

    void init() override;

    /// @addr{0x806DB5B0}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the animation timer mod the total animation duration, then evaluates the
    /// Goomba's state machine to run state-specific logic depending on whether the Goomba is
    /// changing direction or walking along the rail. Finally, increments @ref m_currFrame.
    void calc() override {
        calcAnimTimer();
        StateManager::calc();
        ++m_currFrame;
    }

    /// @addr{0x806DD2C8}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags().setBit(eLoadFlags::Calc, eLoadFlags::Draw);
    }

    /// @addr{0x806dd278}
    /// @copybrief ObjectBase::loadAnims()
    /// @details Loads the Goomba's walking animations, `walk_l` and `walk_r`.
    void loadAnims() override {
        std::array<const char *, 2> names = {{
                "walk_l",
                "walk_r",
        }};

        std::array<Render::AnmType, 2> types = {{
                Render::AnmType::Chr,
                Render::AnmType::Chr,
        }};

        linkAnims(names, types);
    }

private:
    /// @addr{0x806DC220}
    /// @brief Called when the Goomba is idle
    /// @details The Goomba is idle for the duration specified by the current rail node setting 1.
    /// If the Goomba has been idling for this duration, transitions the Goomba to the walking
    /// state. While idle, performs a collision check with the floor and updates the Goomba's
    /// position, rotation, and transform accordingly.
    void calcIdle() {
        if (m_railInterpolator->curPoint().setting[0] < m_currentFrame) {
            m_nextStateId = 1;
        }

        checkSphereFull();
        calcRot();
        calcMatFromRotAndForward();
    }

    /// @addr{0x806DC3F8}
    /// @brief Called when Goomba is walking along the rail
    /// @details Dispatches to @ref calcAnim() to handle the Goomba's walking animation and movement
    /// along the rail.
    void calcWalk() {
        calcAnim();
    }

    void calcAnim();

    /// @addr{0x806DCC9C}
    /// @brief Smoothly interpolates the Goomba's up vector to match the floor normal beneath it
    /// @details Linearly interpolates @ref m_rot towards @ref m_floorNrm.
    void calcRot() {
        constexpr f32 INTERP_RATE = 0.1f;

        m_rot = Interpolate(INTERP_RATE, m_rot, m_floorNrm);

        if (m_rot.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            m_rot.normalise2();
        } else {
            m_rot = EGG::Vector3f::ey;
        }
    }

    void checkSphereFull();

    /// @addr{0x806DCDD0}
    /// @brief Sets the transformation matrix based on rotation and forward direction
    void calcMatFromRotAndForward() {
        setMatrixTangentTo(m_rot, m_forward);
    }

    /// @addr{0x806DD038}
    /// @brief Updates the animation timer based on the current frame and animation rate
    /// @details Calculated as \f[ m\_animTimer = \bmod(m\_currFrame \times
    /// m\_animRate,\ m\_animDuration) \f]
    void calcAnimTimer() {
        m_animTimer = ::fmodf(static_cast<f32>(m_currFrame) * m_animRate, m_animDuration);
    }

    const f32 m_accel;        ///< Acceleration applied when the Goomba is moving
    const f32 m_animRate;     ///< Animation playback rate
    EGG::Vector3f m_forward;  ///< Initial forward direction
    f32 m_animDuration;       ///< Total framecount of walk animation
    u32 m_currFrame;          ///< Number of frames elapsed since the Goomba was initialized
    f32 m_currSpeed;          ///< Current rail velocity
    EGG::Vector3f m_rot;      ///< Smoothed rotation based off of the floor normal
    EGG::Vector3f m_floorNrm; ///< Up vector of the floor beneath the Goomba
    f32 m_animTimer;          ///< m_animDuration wrapped timer, determines if Goomba should walk

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 4> STATE_ENTRIES = {{
            {StateEntry<ObjectKuribo, nullptr, &ObjectKuribo::calcIdle>(0)},
            {StateEntry<ObjectKuribo, nullptr, &ObjectKuribo::calcWalk>(1)},
            {StateEntry<ObjectKuribo, nullptr, nullptr>(2)},
            {StateEntry<ObjectKuribo, nullptr, nullptr>(3)},
    }};
};

} // namespace Kinoko::Field
