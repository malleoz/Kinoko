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

    void init();
    void calc();

    /// @beginSetters

    void setPos(size_t idx, const EGG::Vector3f &pos);
    void setVel(size_t idx, const EGG::Vector3f &v);
    void addSpringForce(size_t idx, const EGG::Vector3f &v);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] const EGG::Vector3f &pos(size_t idx) const;
    [[nodiscard]] const EGG::Vector3f &up(size_t idx) const;

    /// @endGetters

private:
    void calcConstraints();

    owning_span<SphereLink> m_links; ///< Array of links representing the Wiggler's body segments
};

/// @brief Base class for one of the spherical body segments of a Wiggler
class ObjectHanachanPart : public ObjectCollidable {
    /// @brief Grants the main ObjectHanachan class access to the part's state
    friend class ObjectHanachan;

public:
    ObjectHanachanPart(const System::MapdataGeoObj &params);
    ObjectHanachanPart(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    ~ObjectHanachanPart() override;

    void loadRail() override;
    [[nodiscard]] LoadFlags loadFlags() const override;
    [[nodiscard]] const char *getKclName() const override;

private:
    void calcTransformFromUpAndTangent(const EGG::Vector3f &pos, const EGG::Vector3f &up,
            const EGG::Vector3f &tangent);
};

/// @brief Represents the head of a Wiggler, which is the leading segment of the body chain
/// @details The head segment has a larger collision sphere than the body segments
class ObjectHanachanHead final : public ObjectHanachanPart {
public:
    ObjectHanachanHead(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    ~ObjectHanachanHead() override;

    void createCollision() override;
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

    void calcCollisionTransform() override;
    [[nodiscard]] const char *getKclName() const override;

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
    void calc() override;
    void loadGraphics() override;
    void createCollision() override;
    [[nodiscard]] LoadFlags loadFlags() const override;

private:
    /// @brief Tracks whether the Wiggler is following its rail correctly or has deviated
    enum class RailAlignment {
        Aligned = -1,        ///< The Wiggler is closely following the rail
        Unknown = 0,         ///< Uninitialized
        MisalignedLeft = 1,  ///< The rail is deviating to the left from the Wiggler
        MisalignedRight = 2, ///< The rail is deviating to the right from the Wiggler
    };

    void enterWalk();
    void calcWalk();
    void calcWait();

    void onSegmentEnd();
    void initRail();
    void calcRail();
    void initBody();
    void calcBody();
    void initChain();
    void clearChain();
    void calcRailAlignmentMotion();
    void calcSway();
    void calcDefaultLateralMotion();
    void calcFastLateralMotion(s32 frame);
    void calcLateralMotion(f32 amplitude, f32 period, f32 wavelength, s16 frame);
    void setRailVel();
    [[nodiscard]] RailAlignment calcRailAlignment() const;
    [[nodiscard]] bool shouldStartMoving() const;

    /// @beginGetters

    [[nodiscard]] ObjectHanachanHead *&headPart();
    [[nodiscard]] std::span<ObjectHanachanPart *> bodyParts();

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
