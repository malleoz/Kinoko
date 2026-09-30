#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectBirdLeader;
class ObjectBirdFollower;

/// @brief Represents a group of birds.
/// @details ObjectBird holds an @ref ObjectBirdLeader and a variable number of @ref
/// ObjectBirdFollower objects whose positions are offset from the leader's position.
/// @note This object must be implemented in Kinoko to prevent certain ghost replays from desyncing
/// on Toad's Factory. @ref calc() performs a collision check between the birds and the environment
/// to prevent them from flying through floors. Because of this, birds flying near @ref ObjectCrane
/// instances can cause the crane's transformation matrix to update.
class ObjectBird final : public ObjectCollidable {
public:
    ObjectBird(const System::MapdataGeoObj &params);

    /// @addr{0x8077CDC8}
    /// @brief Default virtual destructor
    ~ObjectBird() override = default;

    void calc() override;

    /// @addr{0x8077CCF4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that the object is calculated every frame.
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

    /// @beginGetters

    /// @brief Gets a pointer to the leader of the flock of birds
    /// @return A const pointer to the @ref ObjectBirdLeader of the flock.
    [[nodiscard]] const ObjectBirdLeader *leader() const {
        return m_leader;
    }

    /// @brief Exposes the follower birds so that @ref ObjectBirdFollower can access them
    /// @return A const ref to the collection of follower birds.
    [[nodiscard]] const auto &followers() const {
        return m_followers;
    }

    /// @endGetters

protected:
    ObjectBirdLeader *m_leader; ///< The main bird in the flock; other birds follow this leader
    owning_span<ObjectBirdFollower *> m_followers; ///< The other birds that follow the leader
};

/// @brief The main bird within an @ref ObjectBird. Other birds follow this leader.
class ObjectBirdLeader : public ObjectCollidable {
public:
    /// @addr{0x8077C2F4}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    /// @param bird The @ref ObjectBird instance that this leader belongs to
    /// @details Sets @ref m_bird based on the provided @ref ObjectBird pointer.
    ObjectBirdLeader(const System::MapdataGeoObj &params, const ObjectBird *bird)
        : ObjectCollidable(params),
          m_bird(bird) {}

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
    /// @return Returns @ref eLoadFlags::Calc, so that the object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8077CC78}
    /// @copybrief ObjectBase::loadAnims()
    /// @details Loads the `flying` animation for the bird leader.
    void loadAnims() override {
        std::array<const char *, 1> names = {{
                "flying",
        }};

        std::array<Render::AnmType, 1> types = {{
                Render::AnmType::Chr,
        }};

        linkAnims(names, types);
    }

    /// @copybrief ObjectBase::createCollision()
    /// @details Not overridden in the base game, but collision mode 0 will cause our assert to
    /// fail in @ref ObjectCollidable::createCollision().
    void createCollision() override {}

protected:
    const ObjectBird *const m_bird; ///< The @ref ObjectBird instance that this leader belongs to
};

/// @brief Represents all but one of the birds in an @ref ObjectBird group.
/// @details These birds initialize their position based off of the @ref ObjectBirdLeader. They
/// perform collision checks in their calc function to prevent them from flying through floors. We
/// have to implement this class because it can induce a collision transformation matrix update when
/// birds are in close proximity to a moving @ref ObjectKCL.
class ObjectBirdFollower final : public ObjectBirdLeader {
public:
    /// @addr{0x8077C580}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    /// @param bird The @ref ObjectBird instance that this follower belongs to
    /// @param idx The index of this follower in the flock
    /// @details Sets the bird's @ref m_idx to the provided `idx` and the bird's @ref m_baseSpeed
    /// based off param setting 1.
    ObjectBirdFollower(const System::MapdataGeoObj &params, const ObjectBird *bird, u32 idx)
        : ObjectBirdLeader(params, bird),
          m_idx(idx),
          m_baseSpeed(static_cast<f32>(params.setting(0))) {}

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
