#pragma once

#include "game/field/ObjectCollisionSphere.hh"
#include "game/field/SphereLink.hh"
#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Class that interfaces with the chain links corresponding to Wiggler body parts.
/// @details Responsible for updating chain link objects and enforcing constraints when the chain is
/// taut. @ref ObjectHanachan uses this class to retrieve chain link positions so it can update the
/// Wiggler body parts accordingly.
class HanachanChainManager {
public:
    HanachanChainManager(const std::span<const f32> &linkDistances);
    ~HanachanChainManager();

    /// @addr{0x806F3370}
    /// @brief Initializes all SphereLink objects in the chain
    void init() {
        for (auto &link : m_links) {
            link.init();
        }
    }

    void calc();

    /// @beginSetters

    /// @addr{0x806F43E8}
    void setPos(size_t idx, const EGG::Vector3f &pos) {
        ASSERT(idx < m_links.size());
        m_links[idx].setPos(pos);
    }

    /// @addr{0x806F451C}
    void setVel(size_t idx, const EGG::Vector3f &v) {
        ASSERT(idx < m_links.size());
        m_links[idx].setVel(v);
    }

    /// @addr{0x806F45A4}
    void addSpringForce(size_t idx, const EGG::Vector3f &v) {
        ASSERT(idx < m_links.size());
        m_links[idx].addSpringForce(v);
    }
    /// @endSetters

    /// @beginGetters

    /// @addr{0x806F47B0}
    [[nodiscard]] const EGG::Vector3f &pos(size_t idx) const {
        ASSERT(idx < m_links.size());
        return m_links[idx].pos();
    }

    /// @addr{0x806F481C}
    [[nodiscard]] const EGG::Vector3f &up(size_t idx) const {
        ASSERT(idx < m_links.size());
        return m_links[idx].up();
    }
    /// @endGetters

private:
    /// @addr{0x806F5290}
    /// @brief Calculates the constraints for all SphereLink objects in the chain, ensuring that
    /// they do not stretch beyond their maximum link length
    void calcConstraints() {
        for (size_t i = 1; i < m_links.size(); ++i) {
            m_links[i].calcConstraints(1.0f);
        }
    }

    owning_span<SphereLink> m_links; ///< Array of links representing the Wiggler's body segments
};

/// @brief Base class for one of the spherical body segments of a Wiggler
class ObjectHanachanPart : public ObjectCollidable {
    /// @brief Grants the main ObjectHanachan class access to the part's state
    friend class ObjectHanachan;

public:
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectHanachanPart(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @brief Constructor
    /// @param name The name of the object
    /// @param pos The initial position of the object
    /// @param rot The initial rotation of the object
    /// @param scale The initial scale of the object
    ObjectHanachanPart(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale)
        : ObjectCollidable(name, pos, rot, scale) {}

    /// @addr{0x806C7E68}
    /// @brief Default virtual destructor
    ~ObjectHanachanPart() override = default;

    /// @addr{0x806CCAD0}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806CCB88}
    [[nodiscard]] const char *getKclName() const override {
        return "hanachan";
    }

    /// @addr{0x806CCACC}
    /// @copybrief ObjectBase::loadRail()
    /// @details no-op because the head and body segments are managed by the main @ref
    /// ObjectHanachan class.
    void loadRail() override {}

private:
    /// @addr{0x806CA5E4}
    /// @brief Sets the transform matrix of the object based on its position, up vector, and tangent
    void calcTransformFromUpAndTangent(const EGG::Vector3f &pos, const EGG::Vector3f &up,
            const EGG::Vector3f &tangent) {
        EGG::Matrix34f mat;
        SetRotTangentHorizontal(mat, up, tangent);
        mat.setBase(3, pos);
        setTransform(mat);
    }
};

/// @brief Represents the head of a Wiggler, which is the leading segment of the body chain
/// @details The head segment has a larger collision sphere than the body segments
class ObjectHanachanHead final : public ObjectHanachanPart {
public:
    ObjectHanachanHead(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    ~ObjectHanachanHead() override;

    /// @addr{0x806C818C}
    /// @copybrief ObjectBase::createCollision()
    void createCollision() override {
        m_collision = EGG::egg_new<ObjectCollisionSphere>(150.0f, collisionCenter());
    }

    void calcCollisionTransform() override;

private:
    EGG::Vector3f m_lastPos; ///< Position from last frame, used to calculate speed
};

/// @brief Represents one of the body segments of a Wiggler that trails behind the head segment
class ObjectHanachanBody final : public ObjectHanachanPart {
    /// @brief Grants the main ObjectHanachan class access to the body segment's state
    friend class ObjectHanachan;

public:
    ObjectHanachanBody(const System::MapdataGeoObj &params, const char *mdlName);
    ObjectHanachanBody(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale, const char *mdlName);
    ~ObjectHanachanBody() override;

    /// @addr{0x806CCB80}
    [[nodiscard]] const char *getKclName() const override {
        return m_mdlName;
    }

    void calcCollisionTransform() override;

protected:
    const char *m_mdlName;   ///< KCL name that corresponds to this body segment
    bool m_lastSegment;      ///< True if this is the last segment in the Wiggler's body chain
    EGG::Vector3f m_lastPos; ///< Position from last frame, used to calculate speed

private:
    /// @brief Static array of model names for the Wiggler's body segments, used to determine which
    /// model to load for each segment
    static constexpr std::array<const char *, 4> MDL_NAMES = {
            "hanachan_body1",
            "hanachan_body2",
            "hanachan_body3",
            "hanachan_body4",
    };
};

/// @brief Represents a Wiggler, which is comprised of multiple @ref ObjectHanachanPart segments
class ObjectHanachan final : public ObjectCollidable, private StateManager {
public:
    ObjectHanachan(const System::MapdataGeoObj &params);
    ~ObjectHanachan() override;

    void init() override;

    /// @addr{0x806C9860}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        calcRail();
        StateManager::calc();
        calcBody();
    }

    /// @addr{0x806CC9FC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806CC9F8}
    /// @copybrief ObjectBase::loadGraphics()
    /// @details This is a no-op in the base game.
    void loadGraphics() override {}

    /// @addr{0x806CC9F4}
    /// @copybrief ObjectBase::createCollision()
    /// @details Does not creat collision since the body parts have their own collision spheres
    void createCollision() override {}

private:
    /// @brief Tracks whether the Wiggler is following its rail correctly or has deviated
    enum class RailAlignment {
        Aligned = -1,
        Unknown = 0, ///< Uninitialized
        MisalignedLeft = 1,
        MisalignedRight = 2,
    };

    void enterStateStub() {}

    /// @addr{0x806C9BC0}
    /// @brief Runs once when the Wiggler begins walking
    void enterWalk() {
        setRailVel();
        m_swayAmplitude = INIT_SWAY_AMPLITUDE;
        m_still = false;
        m_leftMisalignFrame = 0;
    }

    void calcWalk();
    void calcWait();

    /// @addr{0x806CA24C}
    /// @brief Called when the Wiggler reaches the end of a rail segment
    /// @details If the rail point's first setting is non-zero, then the Wiggler will wait at that
    /// point for that number of frames
    void onSegmentEnd() {
        u16 setting = m_railInterpolator->curPoint().setting[0];
        if (setting != 0) {
            m_still = true;
            m_stillDuration = setting;
        }
    }

    /// @addr{0x806CA6CC}
    /// @brief Initializes the rail and its velocity
    void initRail() {
        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setCurrVel(m_walkSpeed);
    }

    /// @addr{0x806CA27C}
    /// @brief Caches the last frame's rail tangent and updates the rail interpolator
    void calcRail() {
        m_prevRailTangent = m_railInterpolator->curTangentDir();

        if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd) {
            onSegmentEnd();
        }
    }

    void calcBody();
    void initBody();

    /// @addr{0x806CA9AC}
    /// @brief Initializes the positions of the chain link objects based on the initial parts'
    /// positions
    void initChain() {
        m_chain.init();

        for (size_t i = 0; i < m_parts.size(); ++i) {
            m_chain.setPos(i, m_parts[i]->pos());
        }
    }

    /// @addr{0x806CAAD0}
    /// @brief Resets the chain link positions and clears their velocity and spring force
    void clearChain() {
        m_chain.setPos(0, m_railInterpolator->curPos());
        m_chain.setVel(0, EGG::Vector3f::zero);
        m_chain.addSpringForce(0, EGG::Vector3f::ey * SphereLink::GRAVITY);
    }

    void calcRailAlignmentMotion();
    void calcSway();

    /// @addr{0x806CAC74}
    /// @brief Applies a lateral motion when walking and standing still
    void calcDefaultLateralMotion() {
        constexpr f32 PERIOD = 50.0f;
        constexpr f32 WAVELENGTH = 4000.0f;

        calcLateralMotion(m_swayAmplitude, PERIOD, WAVELENGTH, m_currentFrame);
    }

    /// @addr{0x806CAC94}
    /// @brief Applies a large lateral motion when the Wiggler is misaligned from the rail
    void calcFastLateralMotion(s32 frame) {
        constexpr f32 AMPLITUDE = 25.0f;
        constexpr f32 PERIOD = 300.0f;
        constexpr f32 WAVELENGTH = 5500.0f;

        calcLateralMotion(AMPLITUDE, PERIOD, WAVELENGTH, static_cast<s16>(frame));
    }

    void calcLateralMotion(f32 amplitude, f32 period, f32 wavelength, s16 frame);
    [[nodiscard]] RailAlignment calcRailAlignment() const;

    /// @addr{0x806CAF9C}
    [[nodiscard]] bool shouldStartMoving() const {
        return m_currentFrame > m_stillDuration;
    }

    /// @addr{0x806CBE2C}
    /// @brief Sets the rail interpolator's velocity to the Wiggler's movement speed
    void setRailVel() {
        m_railInterpolator->setCurrVel(m_walkSpeed);
    }

    [[nodiscard]] ObjectHanachanHead *&headPart() {
        return reinterpret_cast<ObjectHanachanHead *&>(m_parts[0]);
    }

    [[nodiscard]] std::span<ObjectHanachanPart *> bodyParts() {
        return std::span(m_parts.begin() + 1, m_parts.size() - 1);
    }

    std::array<ObjectHanachanPart *, 7> m_parts; ///< Array of pointers to the body segments
    HanachanChainManager m_chain; ///< Manager that enforces stretch constraints of the chain link
    const f32 m_walkSpeed;        ///< The speed at which the Wiggler moves along its rail
    std::array<float, 7> m_partDisplacement; ///< Total distance between a given part and the head
    u16 m_stillDuration;     ///< How long the Wiggler waits at the current rail point
    f32 m_swayAmplitude;     ///< Amplitude of lateral sway motion when walking or standing still
    bool m_still;            ///< True if the Wiggler is standing still, false if it is walking
    u16 m_leftMisalignFrame; ///< Frame at which the wiggler became left-misaligned from the rail
    EGG::Vector3f m_prevRailTangent; ///< Previous frame's rail tangent, used to detect sharp turns
    RailAlignment m_railAlignment;   ///< Describes if the Wiggler has deviated from its rail
    RailAlignment m_prevRailAlignment; ///< Used to detect when the Wiggler becomes misaligned

    /// @brief The distance between each body part link in the chain
    static constexpr std::array<f32, 6> BODY_PART_DISTANCES = {{
            360.0f,
            510.0f,
            510.0f,
            480.0f,
            510.0f,
            510.0f,
    }};

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            {StateEntry<ObjectHanachan, &ObjectHanachan::enterWalk, &ObjectHanachan::calcWalk>(0)},
            {StateEntry<ObjectHanachan, &ObjectHanachan::enterStateStub, &ObjectHanachan::calcWait>(
                    1)},
    }};

    /// @brief Initial amplitude of lateral sway motion when walking or standing still
    static constexpr f32 INIT_SWAY_AMPLITUDE = 15.0f;
};

} // namespace Kinoko::Field
