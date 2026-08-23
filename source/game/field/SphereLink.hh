#pragma once

#include <egg/math/Vector.hh>

namespace Kinoko::Field {

/// @brief One link in a chain where each link's position is limited by the other links
/// @details Used by @ref HanachanChainManager to ensure that each body segment stays connected to
/// neighboring segments.
class SphereLink {
public:
    SphereLink();
    ~SphereLink();

    /// @addr{0x806F1194}
    /// @brief Initializes the link's length and resets its internal state
    void initLinkLen(f32 length) {
        m_linkLen = length;
        init();
    }

    /// @brief Resets the link's internal state
    void init() {
        m_pos.setZero();
        m_vel.setZero();
        m_springForce.setZero();
        m_smoothedUp = EGG::Vector3f::ey;
        m_touchingGround = true;
    }

    void calcStiffness();
    void calc();
    void calcConstraints(f32 scale);
    void checkCollision();

    /// @brief Calculates the new position of the link based on its velocity and spring force, and
    /// resets the spring force
    void calcPos() {
        m_vel += m_springForce - GRAVITY;
        m_vel *= 0.9f;
        m_pos += m_vel;
        m_springForce.setZero();
    }

    [[nodiscard]] bool isLeader() const {
        return !m_prev;
    }

    /// @beginSetters
    void setPrev(SphereLink *prev) {
        m_prev = prev;
    }

    void setNext(SphereLink *next) {
        m_next = next;
    }

    void setPos(const EGG::Vector3f &pos) {
        m_pos = pos;
    }

    void setVel(const EGG::Vector3f &vel) {
        m_vel = vel;
    }

    void addSpringForce(const EGG::Vector3f &force) {
        m_springForce += force;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] f32 linkLen() const {
        return m_linkLen;
    }

    [[nodiscard]] const EGG::Vector3f &pos() const {
        return m_pos;
    }

    [[nodiscard]] const EGG::Vector3f &up() const {
        return m_smoothedUp;
    }
    /// @endGetters

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
};

} // namespace Kinoko::Field
