#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The base class for penguins on N64 Sherbet Land.
/// @details Penguins walk along a rail, rotating with smoothed interpolation to face the rail
/// tangent. In the base game, the cave penguins derive from this class, but they do not override
/// any physics-dependent virtual functions. For Kinoko, we simply construct these penguins as a
/// base class instance. This class also defines an enum class which describes the state of motion
/// of the penguin, which is used by the derived class ObjectPenguinS to determine when the penguin
/// is sliding or walking.
class ObjectPenguin : public ObjectCollidable {
public:
    ObjectPenguin(const System::MapdataGeoObj &params);
    ~ObjectPenguin() override;

    /// @addr{0x807756B0}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_railInterpolator->init(0.0f, 0);
        m_state = State::Walk;
        m_basis = EGG::Vector3f::ez;
    }

    /// @addr{0x80775764}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        if (m_state == State::Walk) {
            calcWalk();
        }
    }

    /// @addr{0x80777324}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @brief Runs every frame that the penguin is walking
    /// @addr{0x807757A0}
    virtual void calcWalk() {
        m_railInterpolator->calc();
        calcPos();
        calcRot();
    }

    /// @addr{0x80775B1C}
    void calcRot() {
        constexpr f32 INTERP_RATE = 0.2f;

        m_basis = Interpolate(INTERP_RATE, m_basis, m_railInterpolator->curTangentDir());
        m_basis.normalise();
        setMatrixFromOrthonormalBasisAndPos(m_basis);
    }

    /// @addr{0x80775C2C}
    /// @brief Sets the penguin's position to the rail interpolator's position
    void calcPos() {
        setPos(m_railInterpolator->curPos());
    }

    /// @addr{0x8077588C}
    virtual void enterWalk() {
        m_state = State::Walk;
        calcTransform();
        setRotNoFlag(transform().base(2));
    }

protected:
    /// @brief The state of motion of the penguin
    enum class State {
        Walk = 0,       ///< Walking
        Dive = 1,       ///< Diving mid-air onto belly
        Slider = 2,     ///< Sliding full speed on belly
        SliderSlow = 3, ///< Sliding but slowing down
        StandUp = 4,    ///< Standing up from a slide
    };

    State m_state;         ///< The state of motion of the penguin
    EGG::Vector3f m_basis; ///< Orientation basis / forward direction
};

/// @brief Represents the penguins on N64 Sherbet Land that slide on their stomach.
/// @details The duration of each @ref State is determined by the animation length of the
/// corresponding CHR animation.
class ObjectPenguinS final : public ObjectPenguin {
public:
    ObjectPenguinS(const System::MapdataGeoObj &params);
    ~ObjectPenguinS() override;

    /// @addr{0x807760B0}
    /// @copybrief ObjectBase::init()
    void init() override {
        initAnmTimer();
        m_railInterpolator->init(0.0f, 0);
        m_basis = m_railInterpolator->curTangentDir();
        m_state = State::Walk;
    }

    void calc() override;

    void loadAnims() override;

    /// @addr{0x8077637C}
    void calcWalk() override {
        calcRail();
        calcPos();
        calcRot();
    }

    /// @addr{0x80776AF0}
    void enterWalk() override {
        m_state = State::Walk;
    }

private:
    /// @addr{0x80776188}
    /// @brief Initializes the animation timer
    void initAnmTimer() {
        m_anmTimer = 0;
    }

    /// @addr{0x807763D0}
    /// @brief Runs every frame the penguin is diving mid-air onto its belly
    void calcDive() {
        calcWalk();
    }

    void calcSlider();
    void calcStandUp();
    void calcRail();

    /// @addr{0x80776B7C}
    /// @brief Transitions the penguin into the dive state
    void enterDive() {
        m_state = State::Dive;
    }

    /// @addr{0x80776B94}
    /// @brief Transitions the penguin into the slider state
    void enterSlider() {
        auto *anmMgr = m_drawMdl->anmMgr();
        anmMgr->playAnim(0.0f, 1.0f, 1);
        m_state = State::Slider;
        m_anmTimer = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();
    }

    /// @addr{0x80776C58}
    /// @brief Transitions the penguin into the slider slow state
    void enterSliderSlow() {
        m_state = State::SliderSlow;
    }

    /// @addr{0x80776CDC}
    /// @brief Transitions the penguin into the stand up state
    void enterStandUp() {
        auto *anmMgr = m_drawMdl->anmMgr();
        anmMgr->playAnim(0.0f, 1.0f, 3);
        m_state = State::StandUp;
        m_anmTimer = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();
    }

    s32 m_anmTimer; ///< Frames remaining for the currently plating animation
};

} // namespace Kinoko::Field
