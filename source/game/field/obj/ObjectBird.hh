#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectBirdLeader;
class ObjectBirdFollower;

/// @brief Represents a group of birds.
/// @details ObjectBird holds an @ref ObjectBirdLeader and a variable number of @ref
/// ObjectBirdFollower objects whose positions are offset from the leader's position.
class ObjectBird final : public ObjectCollidable {
public:
    ObjectBird(const System::MapdataGeoObj &params);
    ~ObjectBird() override;

    void calc() override;

    /// @addr{0x8077CCF4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8077CCF0}
    /// @copybrief ObjectBase::loadGraphics()
    /// @details This is a no-op in the base game.
    void loadGraphics() override {}

    /// @addr{0x8077CCE8}
    /// @copybrief ObjectBase::createCollision()
    /// @details This is a no-op in the base game.
    void createCollision() override {}

    /// @addr{0x8077CCE0}
    /// @copybrief ObjectBase::loadRail()
    /// @details This is a no-op in the base game.
    void loadRail() override {}

    [[nodiscard]] const ObjectBirdLeader *leader() const {
        return m_leader;
    }

    /// @brief Exposes the follower birds so that @ref ObjectBirdFollower can access them
    /// @return A const ref to the collection of follower birds.
    [[nodiscard]] const auto &followers() const {
        return m_followers;
    }

protected:
    ObjectBirdLeader *m_leader; ///< The main bird in the flock; other birds follow this leader
    owning_span<ObjectBirdFollower *> m_followers; ///< The other birds that follow the leader
};

/// @brief The main bird within an @ref ObjectBird. Other birds follow this leader.
class ObjectBirdLeader : public ObjectCollidable {
public:
    ObjectBirdLeader(const System::MapdataGeoObj &params, ObjectBird *bird);

    /// @addr{0x8077CE48}
    /// @brief Default virtual destructor
    ~ObjectBirdLeader() override = default;

    void init() override;

    /// @addr{0x8077C504}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the rail interpolator and sets the leader's position accordingly.
    void calc() override {
        m_railInterpolator->calc();
        setPos(m_railInterpolator->curPos());
    }

    /// @addr{0x8077CCD4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    void loadAnims() override;

    /// @copybrief ObjectBase::createCollision()
    /// @details Not overridden in the base game, but collision mode 0 will cause our assert to
    /// fail.
    void createCollision() override {}

protected:
    ObjectBird *m_bird; ///< The parent @ref ObjectBird instance that this leader belongs to
};

/// @brief Represents all but one of the birds in an @ref ObjectBird group.
/// @details These birds initialize their position based off of the @ref ObjectBirdLeader. They
/// perform collision checks in their calc function to prevent them from flying through floors. We
/// have to implement this class because it can induce a collision transformation matrix update when
/// birds are in close proximity to a moving @ref ObjectKCL.
class ObjectBirdFollower final : public ObjectBirdLeader {
public:
    ObjectBirdFollower(const System::MapdataGeoObj &params, ObjectBird *bird, u32 idx);

    /// @addr{0x8077CE88}
    /// @brief Default virtual destructor
    ~ObjectBirdFollower() override = default;

    void init() override;
    void calc() override;

private:
    void calcPos();

    const u32 m_idx;          ///< Index of this follower in the flock
    EGG::Vector3f m_velocity; ///< The current velocity of the follower bird
    const f32 m_baseSpeed;    ///< The base speed of the follower bird
};

} // namespace Kinoko::Field
