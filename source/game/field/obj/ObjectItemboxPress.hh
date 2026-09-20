#pragma once

#include "game/field/obj/ObjectCollidable.hh"
#include "game/field/obj/ObjectPress.hh"

namespace Kinoko::Field {

class ObjectPressSenko;

/// @brief Represents an item box or block that spawns from a @ref ObjectItemboxLine
/// @details Actually inherits from ObjectItembox, but we don't need to implement this functionality
/// in Kinoko. Manages stomper state for the first left/right set of Toad's Factory stompers.
class ObjectItemboxBlock final : public ObjectCollidable {
public:
    /// @addr{0x8076D9E4}
    /// @copydoc ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    ObjectItemboxBlock(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x8076DA48}
    /// @brief Default virtual destructor
    ~ObjectItemboxBlock() override = default;

    /// @addr{0x8076DA88}
    /// @copybrief ObjectBase::init()
    /// @details Sets the initial state of the itembox/block to @ref State::Despawned.
    void init() override {
        m_state = State::Despawned;
    }

    /// @addr{0x8076DAF4}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the position of the itembox/block along the rail if it is currently
    /// spawned.
    void calc() override {
        constexpr f32 HEIGHT_OFFSET = 180.0f;

        switch (m_state) {
        case State::Itembox:
        case State::Block: {
            calcRail();
            const auto &railPos = m_railInterpolator->curPos();
            setPos(EGG::Vector3f(railPos.x, railPos.y + HEIGHT_OFFSET, railPos.z));
        } break;
        default:
            break;
        }
    }

    /// @addr{0x8076E9E4}
    /// @copybrief ObjectBase::id()
    /// @return The ID of an itembox, `ObjectId::Itembox`.
    [[nodiscard]] ObjectId id() const override {
        return ObjectId::Itembox;
    }

    /// @addr{0x8076E9C4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8076E9CC}
    /// @copybrief ObjectBase::getResources()
    /// @details Returns the resource name for the itembox.
    /// @return The resource name for the itembox (`itembox`).
    [[nodiscard]] const char *getResources() const override {
        return "itembox";
    }

    /// @addr{0x8076E9D8}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the itembox (`itembox`).
    [[nodiscard]] const char *getKclName() const override {
        return "itembox";
    }

    /// @addr{0x8076DA98}
    /// @brief Spawns the itembox/block at the start of the rail
    /// @details Initializes the rail interpolator to the start of the rail and sets the per-point
    /// velocities flag to true.
    void spawn() {
        m_state = State::Block;
        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setPerPointVelocities(true);
    }

    /// @beginSetters

    /// @brief Assigns the @ref ObjectPressSenko object associated with this itembox/block.
    /// @param senko Pointer to the @ref ObjectPressSenko object to associate with this
    /// itembox/block.
    void setSenko(ObjectPressSenko *senko) {
        m_senko = senko;
    }

    /// @endSetters

private:
    /// @brief Describes the current state of the itembox/block along the rail
    enum class State {
        Despawned = 0, ///< The itembox/block is not spawned
        Itembox = 1,   ///< The object has been crushed and turned into an itembox
        Block = 2,     ///< The object is spawned and is a block
    };

    /// @addr{0x8076DF44}
    /// @brief Updates the rail interpolator and stomper state
    /// @details Updates the rail interpolator. If the itembox/block has reached the end of a rail
    /// segment and the rail node has setting 2 set to 1, the associated stomper is activated.
    /// Otherwise, if the itembox/block has reached the end of the rail, it is despawned.
    void calcRail() {
        auto result = m_railInterpolator->calc();
        if (result == RailInterpolator::Status::SegmentEnd) {
            if (m_railInterpolator->curPoint().setting[1] == 1) {
                m_senko->beginStomp();
            }
        } else if (result == RailInterpolator::Status::ChangingDirection) {
            m_state = State::Despawned;
        }
    }

    State m_state;             ///< Current state of the itembox/block
    ObjectPressSenko *m_senko; ///< Pointer to the associated stomper object
};

} // namespace Kinoko::Field
