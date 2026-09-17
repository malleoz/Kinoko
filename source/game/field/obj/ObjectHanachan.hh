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

    /// @addr{0x806F31F4}
    /// @brief Default destructor
    ~HanachanChainManager() = default;

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
    /// @brief Sets the position of the specified Wiggler body segment
    /// @param idx The index of the body segment
    /// @param pos The new position of the body segment
    void setPos(size_t idx, const EGG::Vector3f &pos) {
        ASSERT(idx < m_links.size());
        m_links[idx].setPos(pos);
    }

    /// @addr{0x806F451C}
    /// @brief Sets the velocity of the specified Wiggler body segment
    /// @param idx The index of the body segment
    /// @param v The new velocity of the body segment
    void setVel(size_t idx, const EGG::Vector3f &v) {
        ASSERT(idx < m_links.size());
        m_links[idx].setVel(v);
    }

    /// @addr{0x806F45A4}
    /// @brief Adds a spring force to the specified Wiggler body segment
    /// @param idx The index of the body segment
    /// @param v The spring force to add to the body segment
    void addSpringForce(size_t idx, const EGG::Vector3f &v) {
        ASSERT(idx < m_links.size());
        m_links[idx].addSpringForce(v);
    }
    
    /// @endSetters

    /// @beginGetters

    /// @addr{0x806F47B0}
    /// @brief Retrieves the position of the specified Wiggler body segment
    /// @param idx The index of the body segment
    /// @return The position of the body segment
    [[nodiscard]] const EGG::Vector3f &pos(size_t idx) const {
        ASSERT(idx < m_links.size());
        return m_links[idx].pos();
    }

    /// @addr{0x806F481C}
    /// @brief Retrieves the up vector of the specified Wiggler body segment
    /// @param idx The index of the body segment
    /// @return The up vector of the body segment
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
    /// @copydoc ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    ObjectHanachanPart(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @copydoc ObjectCollidable::ObjectCollidable(const char *, const EGG::Vector3f &,
    /// const EGG::Vector3f &, const EGG::Vector3f &)
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
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the Wiggler head (`hanachan`)
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
    /// @brief Sets the transform matrix of the object based on its position, up vector, and
    /// tangent
    /// @param pos The position of the object
    /// @param up The up vector of the object
    /// @param tangent The tangent vector of the object
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
    /// @addr{0x806C7D74}
    /// @copydoc ObjectHanachanPart::ObjectHanachanPart(const char *, const EGG::Vector3f &,
    /// const EGG::Vector3f &, const EGG::Vector3f &)
    /// @details Initializes @ref m_lastPos to the zero vector.
    ObjectHanachanHead(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale)
        : ObjectHanachanPart(name, pos, rot, scale),
          m_lastPos(EGG::Vector3f::zero) {}

    /// @addr{0x806CCB94}
    /// @brief Default virtual destructor
    ~ObjectHanachanHead() override = default;

    /// @addr{0x806C818C}
    /// @copybrief ObjectBase::createCollision()
    /// @details Creates a collision sphere for the head segment with a radius of `150` units,
    /// centered at the collision center.
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
    /// @brief Overloaded constructor
    /// @param params The parameters used to initialize the object
    /// @param mdlName The name of the model corresponding to this body segment
    /// @details Caches the model name to @ref m_mdlName. Initializes @ref m_lastSegment to `false`
    /// and @ref m_lastPos to the zero vector.
    ObjectHanachanBody(const System::MapdataGeoObj &params, const char *mdlName)
        : ObjectHanachanPart(params),
          m_mdlName(mdlName),
          m_lastSegment(false),
          m_lastPos(EGG::Vector3f::zero) {}

    /// @brief Overloaded constructor
    /// @param name The name of the object
    /// @param pos The initial position of the object
    /// @param rot The initial rotation of the object
    /// @param scale The initial scale of the object
    /// @param mdlName The name of the model corresponding to this body segment
    /// @details Caches the model name to @ref m_mdlName. Initializes @ref m_lastSegment to `false`
    /// and @ref m_lastPos to the zero vector.
    ObjectHanachanBody(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale, const char *mdlName)
        : ObjectHanachanPart(name, pos, rot, scale),
          m_mdlName(mdlName),
          m_lastSegment(false),
          m_lastPos(EGG::Vector3f::zero) {}

    /// @addr{0x806CCAD8}
    /// @brief Default virtual destructor
    ~ObjectHanachanBody() override = default;

    /// @addr{0x806CCB80}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the Wiggler body segment (`hanachan_body1`, `hanachan_body2`,
    /// `hanachan_body3`, or `hanachan_body4`)
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

    /// @addr{0x806C9598}
    /// @brief Default virtual destructor
    ~ObjectHanachan() override = default;

    void init() override;

    /// @addr{0x806C9860}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the rail interpolator, evaluates the Wiggler's state machine, and updates
    /// the Wiggler's body segments' positions.
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
        Aligned = -1,        ///< The Wiggler is closely following the rail
        Unknown = 0,         ///< Uninitialized
        MisalignedLeft = 1,  ///< The rail is deviating to the left from the Wiggler
        MisalignedRight = 2, ///< The rail is deviating to the right from the Wiggler
    };

    /// @addr{0x806C9BC0}
    /// @brief Runs once when the Wiggler begins walking
    /// @details Sets the rail's velocity to @ref m_walkSpeed, sets @ref m_swayAmplitude to @ref
    /// INIT_SWAY_AMPLITUDE, sets @ref m_still to `false`, and resets @ref m_leftMisalignFrame to
    /// zero.
    void enterWalk() {
        setRailVel();
        m_swayAmplitude = INIT_SWAY_AMPLITUDE;
        m_still = false;
        m_leftMisalignFrame = 0;
    }

    void calcWalk();

    /// @addr{0x806C9F98}
    /// @brief Runs every frame when the Wiggler is standing still
    /// @detail If the Wiggler has been standing still for @ref m_stillDuration frames, then
    /// transitions the Wiggler to the walking state. Otherwise, stops all body segments from
    /// moving. Calculates the lateral sway of all body segments and updates their positions
    /// accordingly.
    void calcWait() {
        if (shouldStartMoving()) {
            m_nextStateId = 0;
        }

        clearChain();
        calcSway();
        m_chain.calc();
    }

    /// @addr{0x806CA24C}
    /// @brief Called when the Wiggler reaches the end of a rail segment
    /// @details If the current rail point's first setting is non-zero, then the Wiggler will wait
    /// at that point for that number of frames
    void onSegmentEnd() {
        u16 setting = m_railInterpolator->curPoint().setting[0];
        if (setting != 0) {
            m_still = true;
            m_stillDuration = setting;
        }
    }

    /// @addr{0x806CA6CC}
    /// @brief Initializes the rail and its velocity
    /// @details Initializes the rail interpolator to its starting position and sets its speed to
    /// @ref m_walkSpeed.
    void initRail() {
        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setSpeed(m_walkSpeed);
    }

    /// @addr{0x806CA27C}
    /// @brief Caches the last frame's rail tangent and updates the rail interpolator
    /// @details Stores the current rail tangent direction in @ref m_prevRailTangent and then
    /// updates the rail interpolator. If the end of the current rail segment is reached, calls
    /// @ref onSegmentEnd().
    void calcRail() {
        m_prevRailTangent = m_railInterpolator->curTangentDir();

        if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd) {
            onSegmentEnd();
        }
    }

    void calcBody();

    /// @addr{0x806CA72C}
    /// @brief Initializes the positions of the Wiggler's body parts based on the initial rail
    /// position
    /// @details Sets the head's position to the current rail interpolator position. For each body
    /// segment, sets its position based on the previous segment's position and the current rail
    /// tangent direction, and aligns its matrix tangent to the rail.
    void initBody() {
        headPart()->setPos(m_railInterpolator->curPos());

        const EGG::Vector3f &curTanDir = m_railInterpolator->curTangentDir();
        for (size_t i = 1; i < m_parts.size(); ++i) {
            m_parts[i]->setPos(m_parts[i - 1]->pos() - curTanDir * BODY_PART_DISTANCES[i - 1]);
            m_parts[i]->setMatrixTangentTo(EGG::Vector3f::ey, curTanDir);
        }
    }

    /// @addr{0x806CA9AC}
    /// @brief Initializes the positions of the chain link objects based on the parts' initial
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

    /// @addr{0x806CB67C}
    /// @brief Updates the sway amplitude and calculates the lateral sway motion
    /// @details Decays the sway amplitude by `0.25f`, and clamps it to zero if it becomes smaller
    /// than `1.0f`. Finally, applies a sinusoidal sway to the body segments using the updated
    /// amplitude.
    void calcSway() {
        constexpr f32 SWAY_AMPLITUDE_DELTA = 0.25f;

        if (m_swayAmplitude >= 0.0f) {
            m_swayAmplitude -= SWAY_AMPLITUDE_DELTA;
        } else {
            m_swayAmplitude += SWAY_AMPLITUDE_DELTA;
        }

        if (EGG::Mathf::abs(m_swayAmplitude) <= 1.0f) {
            m_swayAmplitude = 0.0f;
        }

        calcDefaultLateralMotion();
    }

    /// @addr{0x806CAC74}
    /// @brief Applies a lateral motion when walking and standing still
    /// @details The lateral motion has an amplitude of @ref m_swayAmplitude, a period of `50.0f`, a
    /// wavelength of `4000.0f`, and a phase of @ref m_currentFrame.
    void calcDefaultLateralMotion() {
        constexpr f32 PERIOD = 50.0f;
        constexpr f32 WAVELENGTH = 4000.0f;

        calcLateralMotion(m_swayAmplitude, PERIOD, WAVELENGTH, m_currentFrame);
    }

    /// @addr{0x806CAC94}
    /// @brief Applies a large lateral motion when the Wiggler is misaligned from the rail
    /// @param frame The current frame used to calculate the lateral motion
    /// @details The lateral motion has an amplitude of `25.0f`, a period of `300.0f`, a wavelength
    /// of `5500.0f`, and a phase of the provided `frame`.
    void calcFastLateralMotion(s32 frame) {
        constexpr f32 AMPLITUDE = 25.0f;
        constexpr f32 PERIOD = 300.0f;
        constexpr f32 WAVELENGTH = 5500.0f;

        calcLateralMotion(AMPLITUDE, PERIOD, WAVELENGTH, static_cast<s16>(frame));
    }

    void calcLateralMotion(f32 amplitude, f32 period, f32 wavelength, s16 frame);
    [[nodiscard]] RailAlignment calcRailAlignment() const;

    /// @addr{0x806CAF9C}
    /// @brief Determines if the Wiggler should start moving based on the current frame
    /// @return `true` if the Wiggler should start moving, `false` otherwise
    [[nodiscard]] bool shouldStartMoving() const {
        return m_currentFrame > m_stillDuration;
    }

    /// @addr{0x806CBE2C}
    /// @brief Sets the rail interpolator's velocity to the Wiggler's movement speed
    void setRailVel() {
        m_railInterpolator->setSpeed(m_walkSpeed);
    }

    /// @beginGetters

    /// @brief Gets the head part of the Wiggler via a `reinterpret_cast`
    /// @return A pointer to the head part of the Wiggler
    [[nodiscard]] ObjectHanachanHead *&headPart() {
        return reinterpret_cast<ObjectHanachanHead *&>(m_parts[0]);
    }

    /// @brief Gets a span of all body parts except the head of the Wiggler
    /// @return A span containing pointers to all body parts except the head
    [[nodiscard]] std::span<ObjectHanachanPart *> bodyParts() {
        return std::span(m_parts.begin() + 1, m_parts.size() - 1);
    }
    
    /// @endGetters

    static constexpr size_t PART_COUNT = 7;

    std::array<ObjectHanachanPart *, PART_COUNT> m_parts; ///< Array of body segment pointers
    HanachanChainManager m_chain; ///< Manager that enforces stretch constraints of the chain link
    const f32 m_walkSpeed;        ///< The speed at which the Wiggler moves along its rail
    std::array<float, PART_COUNT> m_partDisplacement; ///< Distance between a part and the head
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
            {StateEntry<ObjectHanachan, nullptr, &ObjectHanachan::calcWait>(1)},
    }};

    /// @brief Initial amplitude of lateral sway motion when walking or standing still
    static constexpr f32 INIT_SWAY_AMPLITUDE = 15.0f;
};

} // namespace Kinoko::Field
