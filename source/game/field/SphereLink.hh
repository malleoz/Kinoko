#pragma once

#include <egg/math/Vector.hh>

namespace Kinoko::Field {

/// @brief Represents one link in a chain where each link's position is limited by the other links
/// @details Used by @ref HanachanChainManager to ensure that each body segment stays connected to
/// neighboring segments.
class SphereLink {
public:
    SphereLink();
    ~SphereLink();

    void initLinkLen(f32 length);
    void init();
    void calcStiffness();
    void calc();
    void calcConstraints(f32 scale);
    void checkCollision();
    void calcPos();

    /// @beginSetters

    void setPrev(SphereLink *prev);
    void setNext(SphereLink *next);
    void setPos(const EGG::Vector3f &pos);
    void setVel(const EGG::Vector3f &vel);
    void addSpringForce(const EGG::Vector3f &force);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] bool isLeader() const;
    [[nodiscard]] f32 linkLen() const;
    [[nodiscard]] const EGG::Vector3f &pos() const;
    [[nodiscard]] const EGG::Vector3f &up() const;

    /// @endGetters

    /// @brief The gravity vector applied to the link's velocity every frame
    static constexpr EGG::Vector3f GRAVITY = EGG::Vector3f(0.0f, 2.5f, 0.0f);

private:
    void calcSpring();

    SphereLink *m_prev;  ///< The previous link in the chain, or nullptr if this is the first link
    SphereLink *m_next;  ///< The next link in the chain, or nullptr if this is the last link
    f32 m_linkLen;       ///< The maximum distance between this link and the previous (forward) link
    EGG::Vector3f m_pos; ///< The current position of the link in world space
    EGG::Vector3f m_vel; ///< The current velocity of the link
    EGG::Vector3f m_springForce; ///< Prevents links from stretching past their m_linkLen
    EGG::Vector3f m_smoothedUp;  ///< Smoothed up vector for the link
    bool m_touchingGround;       ///< Whether the link is currently touching the ground

    /// @brief Magnitude of the spring force applied to the link when it is stretched beyond its
    /// link length
    static constexpr f32 SPRING_FORCE_MAGNITUDE = 2.0f;
};

} // namespace Kinoko::Field
