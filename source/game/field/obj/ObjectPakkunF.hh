#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents the piranhas on GCN Mario Circuit.
/// @details These piranhas often exist above a pipe, but these piranha objects are separate from
/// the pipe object, which is a primitive @ref ObjectCollidable represented with
/// @ref ObjectId::PakkunDokan. Piranhas cycle between a waiting state and an attacking state. When
/// they attack, their collision transformation matrix is updated to reflect the piranha chomping
/// forward and downward.
class ObjectPakkunF final : public ObjectCollidable {
public:
    ObjectPakkunF(const System::MapdataGeoObj &params);
    ~ObjectPakkunF() override;

    /// @addr{0x807743E4}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_state = State::Wait;
        m_waitFrames = m_waitDuration;

        calcTransform();
    }

    void calc() override;

    /// @addr{0x807754F4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    void loadAnims() override;
    void calcCollisionTransform() override;

    /// @addr{0x80775458}
    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The collision radius of the piranha, `1000.0f`.
    /// @details Extends the collision radius to ensure GJK collision checks occur from far enough
    /// away when the piranha chomps forward.
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 1000.0f;
    }

private:
    /// @brief Distinguishes between whether the piranha is idle or chomping
    enum class State {
        Wait = 0,   ///< The piranha is idle
        Attack = 1, ///< The piranha is chomping
    };

    /// @addr{0x80774A00}
    /// @brief Runs every frame when the piranha is idle
    /// @details When the the wait timer reaches 0, the piranha will enter the attack state.
    void calcWait() {
        if (--m_waitFrames == 0) {
            enterAttack();
        }
    }

    /// @addr{0x80774A84}
    /// @brief Runs every frame while the piranha is attacking
    void calcAttack() {
        ++m_currAttackFrame;
        calcAttackTimer();
    }

    void enterAttack();

    /// @addr{0x80774C1C}
    /// @brief Updates the attack timer
    void calcAttackTimer() {
        if (--m_attackFrames == 0) {
            enterWait();
        }
    }

    /// @addr{0x80774C80}
    /// @brief Transitions the piranha from the attack state back to the wait state
    void enterWait() {
        m_state = State::Wait;
        m_waitFrames = m_waitDuration;
    }

    State m_state;         ///< Tracks whether the piranha is idle or chomping
    s32 m_waitFrames;      ///< How long until the piranha starts chomping
    s32 m_attackFrames;    ///< How long until the piranha stops chomping
    u32 m_currAttackFrame; /// How long the piranhas has been attacking for

    /// @brief Total time the piranha will be idle for
    /// @details Not in the base game, but caching this prevents having to dereference m_mapObj
    /// multiple times.
    const s32 m_waitDuration;
};

} // namespace Kinoko::Field
