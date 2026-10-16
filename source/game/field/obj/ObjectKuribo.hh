#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents a walking Goomba
/// @details Goombas walk along a rail. Once they reach the end of the rail, they reverse direction.
class ObjectKuribo final : public ObjectCollidable, private StateManager {
public:
    ObjectKuribo(const System::MapdataGeoObj &params);
    ~ObjectKuribo() override;

    void init() override;
    void calc() override;
    void loadAnims() override;
    [[nodiscard]] LoadFlags loadFlags() const override;

private:
    void calcIdle();
    void calcWalk();

    void calcAnim();
    void calcRot();
    void checkSphereFull();
    void calcMatFromRotAndForward();
    void calcAnimTimer();

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
