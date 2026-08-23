#pragma once

#include "game/field/obj/ObjectCollidable.hh"
#include "game/field/obj/ObjectPress.hh"

namespace Kinoko::Field {

class ObjectPressSenko;

/// @brief Manages stomper state for the first left/right set of Toad's Factory stompers
/// @details Actually inherits from ObjectItembox, but we don't need to implement.
class ObjectItemboxPress final : public ObjectCollidable {
public:
    /// @addr{0x8076D9E4}
    ObjectItemboxPress(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x8076DA48}
    ~ObjectItemboxPress() override = default;

    /// @addr{0x8076DA88}
    void init() override {
        m_state = 0;
    }

    /// @addr{0x8076DAF4}
    void calc() override {
        constexpr f32 HEIGHT_OFFSET = 180.0f;

        switch (m_state) {
        case 1:
        case 2: {
            calcRail();
            const auto &railPos = m_railInterpolator->curPos();
            setPos(EGG::Vector3f(railPos.x, railPos.y + HEIGHT_OFFSET, railPos.z));
        } break;
        default:
            break;
        }
    }

    /// @addr{0x8076E9E4}
    [[nodiscard]] ObjectId id() const override {
        return ObjectId::Itembox;
    }

    /// @addr{0x8076E9C4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x8076E9CC}
    [[nodiscard]] const char *getResources() const override {
        return "itembox";
    }

    /// @addr{0x8076E9D8}
    [[nodiscard]] const char *getKclName() const override {
        return "itembox";
    }

    /// @addr{0x8076DA98}
    /// @brief Used by @ref ObjectItemboxLine to activate the stomper.
    void startPress() {
        m_state = 2;
        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setPerPointVelocities(true);
    }

    void setSenko(ObjectPressSenko *senko) {
        m_senko = senko;
    }

private:
    /// @addr{0x8076DF44}
    /// @brief Updates the rail interpolator and stomper state
    void calcRail() {
        auto result = m_railInterpolator->calc();
        if (result == RailInterpolator::Status::SegmentEnd) {
            if (m_railInterpolator->curPoint().setting[1] == 1) {
                m_senko->beginStomp();
            }
        } else if (result == RailInterpolator::Status::ChangingDirection) {
            m_state = 0;
        }
    }

    u32 m_state;               ///< Current state of the stomper (0 = idle, 2 = stomping)
    ObjectPressSenko *m_senko; ///< Pointer to the underlying stomper object
};

} // namespace Kinoko::Field
