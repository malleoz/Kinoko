#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents a walking Goomba
class ObjectKuribo : public ObjectCollidable, public StateManager {
public:
    ObjectKuribo(const System::MapdataGeoObj &params);
    ~ObjectKuribo() override;

    void init() override;
    void calc() override;

    /// @addr{0x806DD2C8}
    [[nodiscard]] u32 loadFlags() const override {
        return 3;
    }

    void loadAnims() override;

private:
    void enterStateStub() {}

    void calcStateStub() {}

    void calcReroute();

    /// @addr{0x806DC3F8}
    /// @brief Called when Goomba is walking along the rail
    void calcWalk() {
        calcAnim();
    }

    void calcAnim();
    void calcRot();
    void checkSphereFull();

    const f32 m_accel;        ///< Acceleration applied when the Goomba is moving
    const f32 m_animRate;     ///< Animation playback rate
    EGG::Vector3f m_forward;  ///< Initial forward direction
    f32 m_animDuration;       ///< Total framecount of walk animation
    u32 m_currFrame;          ///< Number of frames elapsed since the Goomba was initialized
    f32 m_currSpeed;          ///< Current rail velocity
    EGG::Vector3f m_rot;      ///< Smoothed rotation based off of the floor normal
    EGG::Vector3f m_floorNrm; ///< Up vector of the floor beneath the Goomba
    f32 m_animTimer;          ///< m_animDuration wrapped timer, determines if Goomba should walk

    static constexpr std::array<StateManagerEntry, 4> STATE_ENTRIES = {{
            {StateEntry<ObjectKuribo, &ObjectKuribo::enterStateStub, &ObjectKuribo::calcReroute>(
                    0)},
            {StateEntry<ObjectKuribo, &ObjectKuribo::enterStateStub, &ObjectKuribo::calcWalk>(1)},
            {StateEntry<ObjectKuribo, &ObjectKuribo::enterStateStub, &ObjectKuribo::calcStateStub>(
                    2)},
            {StateEntry<ObjectKuribo, &ObjectKuribo::enterStateStub, &ObjectKuribo::calcStateStub>(
                    3)},
    }};
};

} // namespace Kinoko::Field
