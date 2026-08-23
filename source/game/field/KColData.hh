#pragma once

#include "game/field/KCollisionTypes.hh"

#include <egg/math/BoundBox.hh>
#include <egg/math/Matrix.hh>

// Credit: em-eight/mkw
// Credit: stblr/Hanachan

namespace Kinoko::Field {

/// @brief Stores partial information pertaining to a collision
struct CollisionInfoPartial {
    EGG::BoundBox3f bbox;     ///< Bounding box of "push out" vectors
    EGG::Vector3f tangentOff; ///< The net "push out" vector

    /// @brief Expands the bounding box to include the provided position vector
    void update(const EGG::Vector3f &offset) {
        bbox.min = bbox.min.minimize(offset);
        bbox.max = bbox.max.maximize(offset);
    }
};

/// @brief Stores information pertaining to a collision
/// @details Tracks the distance for the closest floor, wall, and moving floor collision during a
/// collision query so that it can update its internal state to reflect the closest collision for
/// each category.
struct CollisionInfo {
    EGG::BoundBox3f bbox;       ///< Bounding box of "push out" vectors
    EGG::Vector3f tangentOff;   ///< The net "push out" vector
    EGG::Vector3f floorNrm;     ///< Colliding floor's up vector
    EGG::Vector3f wallNrm;      ///< Colliding wall face's normal vector
    EGG::Vector3f roadVelocity; ///< Optional velocity induced by the colliding object
    f32 floorDist;              ///< Distance from the colliding floor tri
    f32 wallDist;               ///< Distance from the colliding wall tri
    f32 movingFloorDist;        ///< Distance from the colliding moving floor tri
    f32 perpendicularity;       ///< Measures how much two colliding wall normals diverge

    /// @brief Updates the floor collision info if the provided distance is closer than the
    /// currently tracked floor collision
    /// @param dist Distance from the colliding floor
    /// @param fnrm Colliding floor's up vector
    void updateFloor(f32 dist, const EGG::Vector3f &fnrm) {
        if (dist > floorDist) {
            floorDist = dist;
            floorNrm = fnrm;
        }
    }

    /// @brief Updates the wall collision info if the provided distance is closer than the
    /// currently tracked wall collision
    /// @param dist Distance from the colliding wall
    /// @param fnrm Colliding wall's up vector
    void updateWall(f32 dist, const EGG::Vector3f &fnrm) {
        if (dist > wallDist) {
            wallDist = dist;
            wallNrm = fnrm;
        }
    }

    /// @brief Initializes members to clear all collision info
    void reset() {
        bbox.setZero();
        movingFloorDist = -std::numeric_limits<f32>::min();
        wallDist = -std::numeric_limits<f32>::min();
        floorDist = -std::numeric_limits<f32>::min();
        perpendicularity = 0.0f;
    }

    void update(f32 now_dist, const EGG::Vector3f &offset, const EGG::Vector3f &fnrm,
            u32 kclAttributeTypeBit);
    void transformInfo(CollisionInfo &rhs, const EGG::Matrix34f &mtx, const EGG::Vector3f &v);
};

/// @brief Performs lookups for KCL triangles
class KColData {
public:
    enum class CollisionCheckType {
        Edge,
        Plane,
        Movement,
    };

    struct KCollisionPrism {
        KCollisionPrism();
        KCollisionPrism(f32 height, u16 posIndex, u16 faceNormIndex, u16 edge1NormIndex,
                u16 edge2NormIndex, u16 edge3NormIndex, u16 attribute);

        f32 height;
        u16 pos_i;
        u16 fnrm_i;
        u16 enrm1_i;
        u16 enrm2_i;
        u16 enrm3_i;
        u16 attribute;
    };
    STATIC_ASSERT(sizeof(KCollisionPrism) == 0x10);

    KColData(const void *file);
    ~KColData();

    void narrowScopeLocal(const EGG::Vector3f &pos, f32 radius, KCLTypeMask mask);
    void narrowPolygon_EachBlock(const u16 *prismArray);

    void computeBBox();
    /// @addr{0x807C1F80}
    [[nodiscard]] bool checkPointCollision(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut) {
        return std::isfinite(m_prevPos.y) ? checkPointMovement(distOut, fnrmOut, flagsOut) :
                                            checkPoint(distOut, fnrmOut, flagsOut);
    }

    /// @addr{0x807C2410}
    [[nodiscard]] bool checkSphereCollision(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut) {
        return std::isfinite(m_prevPos.y) ? checkSphereMovement(distOut, fnrmOut, flagsOut) :
                                            checkSphere(distOut, fnrmOut, flagsOut);
    }

    [[nodiscard]] bool checkSphere(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut);
    [[nodiscard]] bool checkSphereSingle(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut);

    /// @addr{0x807C1B0C}
    /// @brief Sets members in preparation of a subsequent point collision check call
    void lookupPoint(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask typeMask) {
        m_prismIter = searchBlock(pos);
        m_pos = pos;
        m_prevPos = prevPos;
        m_movement = pos - prevPos;
        m_typeMask = typeMask;
    }

    /// @addr{0x807C1BB4}
    /// @brief Sets members in preparation of a subsequent sphere collision check call
    void lookupSphere(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask typeMask) {
        m_prismIter = searchBlock(pos);
        m_pos = pos;
        m_prevPos = prevPos;
        m_movement = pos - prevPos;
        m_radius = std::min(radius, m_sphereRadius);
        m_typeMask = typeMask;
    }

    void lookupSphereCached(const EGG::Vector3f &p1, const EGG::Vector3f &p2, u32 typeMask,
            f32 radius);

    [[nodiscard]] const u16 *searchBlock(const EGG::Vector3f &pos);

    /// @beginGetters
    [[nodiscard]] const EGG::BoundBox3f &bbox() const {
        return m_bbox;
    }

    [[nodiscard]] u16 prismCache(u32 idx) const {
        return m_prismCache[idx];
    }

    [[nodiscard]] std::span<const KCollisionPrism> prisms() const {
        return m_prisms.view();
    }

    [[nodiscard]] std::span<const EGG::Vector3f> nrms() const {
        return m_nrms.view();
    }

    [[nodiscard]] std::span<const EGG::Vector3f> vertices() const {
        return m_vertices.view();
    }
    /// @endGetters

    /// @brief Computes a prism vertex based off of the triangle's normal vectors
    /// @addr{0x807BDF54}
    /// @par Triangle Vertices Formula
    /// Given a triangle with vertices \f$\vec{A}, \vec{B}, \vec{C}\f$, face normal \f$\hat{f}\f$,
    /// and height \f$h\f$, label the edge normals by: \begin{aligned}\hat{en}_1 := e_{AB}, \,\,
    /// \hat{en}_2:= e_{AC}, \,\,\hat{en}_3:=e_{BC} \end{aligned} We can recover \f$\vec{B},
    /// \vec{C}\f$ via: \begin{aligned} \vec{B} = \vec{A} + \dfrac{h}{(\hat{en}_2 \times \hat{f})
    /// \cdot
    /// \hat{en}_3}\left(\hat{en}_2 \times \hat{f}\right), \, \, \vec{C} = \vec{A} +
    /// \dfrac{h}{(\hat{en}_1 \times \hat{f}) \cdot \hat{en}_3}(\hat{en}_1 \times \hat{f}) \, .
    /// \end{aligned}
    [[nodiscard]] EGG::Vector3f GetVertex(f32 height, const EGG::Vector3f &vertex1,
            const EGG::Vector3f &fnrm, const EGG::Vector3f &enrm3, const EGG::Vector3f &enrm) {
        EGG::Vector3f cross = fnrm.cross(enrm);
        f32 dp = cross.ps_dot(enrm3);
        cross *= (height / dp);

        return cross + vertex1;
    }

private:
    void preloadPrisms();
    void preloadNormals();
    void preloadVertices();

    template <CollisionCheckType Type>
    [[nodiscard]] bool checkCollision(const KCollisionPrism &prism, f32 *distOut,
            EGG::Vector3f *fnrmOut, u16 *flagsOut);

    [[nodiscard]] bool checkPointCollision(const KCollisionPrism &prism, f32 *distOut,
            EGG::Vector3f *fnrmOut, u16 *flagsOut, bool movement);
    [[nodiscard]] bool checkSphereMovement(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *attributeOut);
    [[nodiscard]] bool checkPointMovement(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *attributeOut);
    [[nodiscard]] bool checkPoint(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *attributeOut);

    const void *m_posData;
    const void *m_nrmData;
    const void *m_prismData;
    const void *m_blockData;
    f32 m_prismThickness;
    EGG::Vector3f m_areaMinPos;
    u32 m_areaXWidthMask;    ///< The x dimension of the octree's bounding box. @see searchBlock.
    u32 m_areaYWidthMask;    ///< The y dimension of the octree's bounding box. @see searchBlock.
    u32 m_areaZWidthMask;    ///< The z dimension of the octree's bounding box. @see searchBlock.
    u32 m_blockWidthShift;   ///< Used to initialize octree navigation. @see searchBlock.
    u32 m_areaXBlocksShift;  ///< Used to initialize octree navigation. @see searchBlock.
    u32 m_areaXYBlocksShift; ///< Used to initialize octree navigation. @see searchBlock.
    f32 m_sphereRadius;      ///< Clamps the sphere we check collision against. @see searchBlock.
    EGG::Vector3f m_pos;
    EGG::Vector3f m_prevPos;
    EGG::Vector3f m_movement;
    f32 m_radius;
    KCLTypeMask m_typeMask;
    const u16 *m_prismIter;
    EGG::BoundBox3f m_bbox;
    std::array<u16, 256> m_prismCache;
    u16 *m_prismCacheTop;
    u16 *m_cachedPrismArray;
    EGG::Vector3f m_cachedPos;
    f32 m_cachedRadius;

    /// @brief Optimizes for time by avoiding unnecessary byteswapping.
    /// The Wii doesn't have this problem because big endian is always assumed.
    owning_span<KCollisionPrism> m_prisms;
    owning_span<EGG::Vector3f> m_nrms;
    owning_span<EGG::Vector3f> m_vertices;
};

} // namespace Kinoko::Field
