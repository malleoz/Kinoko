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
    /// @brief Describes the type of collision check to perform
    enum class CollisionCheckType {
        Edge,     ///< Checks if a body is colliding with any edge of the prism
        Plane,    ///< Checks if a body is colliding with a plane of the prism
        Movement, ///< Plane collisions that occur when traveling towards the face of the prism
    };

    /// @brief Represesnts a KCL collision prism
    struct KCollisionPrism {
        /// @brief Non-initializing default constructor
        KCollisionPrism() = default;

        /// @brief Initializing constructor
        /// @param height The height of the tri
        /// @param posIndex Index of the first vertex's index in @ref KColData::m_vertices
        /// @param faceNormIndex Index of the face normal in @ref KColData::m_nrms
        /// @param edge1NormIndex  Index of the first edge's normal in @ref KColData::m_nrms
        /// @param edge2NormIndex  Index of the second edge's normal in @ref KColData::m_nrms
        /// @param edge3NormIndex  Index of the third edge's normal in @ref KColData::m_nrms
        /// @param attribute  KCL attribute of the tri
        KCollisionPrism(f32 height, u16 posIndex, u16 faceNormIndex, u16 edge1NormIndex,
                u16 edge2NormIndex, u16 edge3NormIndex, u16 attribute)
            : height(height),
              pos_i(posIndex),
              fnrm_i(faceNormIndex),
              enrm1_i(edge1NormIndex),
              enrm2_i(edge2NormIndex),
              enrm3_i(edge3NormIndex),
              attribute(attribute) {}

        /// @brief Default destructor
        ~KCollisionPrism() = default;

        f32 height;    ///< The height of the tri
        u16 pos_i;     ///< Index of the first vertex's index in @ref m_vertices
        u16 fnrm_i;    ///< Index of the face normal in @ref m_nrms
        u16 enrm1_i;   ///< Index of the first edge's normal in @ref m_nrms
        u16 enrm2_i;   ///< Index of the second edge's normal in @ref m_nrms
        u16 enrm3_i;   ///< Index of the third edge's normal in @ref m_nrms
        u16 attribute; ///< KCL attribute of the tri
    };
    STATIC_ASSERT(sizeof(KCollisionPrism) == 0x10);

    KColData(const void *file);
    ~KColData();

    void narrowScopeLocal(const EGG::Vector3f &pos, f32 radius, KCLTypeMask mask);
    void narrowPolygon_EachBlock(const u16 *prismArray);

    void computeBBox();

    /// @addr{0x807C1F80}
    /// @brief Checks for a collision at a specific point. If the previous position is valid, then
    /// it filters only to prisms for which the kart is traveling in the direction of the prism's
    /// face normal.
    /// @param distOut Output parameter for the distance to the collision point
    /// @param fnrmOut Output parameter for the face normal at the collision point
    /// @param flagsOut Output parameter for the collision flags
    [[nodiscard]] bool checkPointCollision(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut) {
        return checkPoint(distOut, fnrmOut, flagsOut, std::isfinite(m_prevPos.y));
    }

    /// @addr{0x807C2410}
    /// @brief Checks for a collision with a sphere. If the previous position is valid, then
    /// it filters only to prisms for which the kart is traveling in the direction of the prism's
    /// face normal.
    /// @param distOut Output parameter for the distance to the collision point
    /// @param fnrmOut Output parameter for the face normal at the collision point
    /// @param flagsOut Output parameter for the collision flags
    [[nodiscard]] bool checkSphereCollision(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut) {
        return std::isfinite(m_prevPos.y) ? checkSphereMovement(distOut, fnrmOut, flagsOut) :
                                            checkSphere(distOut, fnrmOut, flagsOut);
    }

    [[nodiscard]] bool checkSphere(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut);
    [[nodiscard]] bool checkSphereSingle(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut);

    /// @addr{0x807C1B0C}
    /// @brief Sets members in preparation of a subsequent point collision check call
    /// @param pos The position of the point to search around
    /// @param prevPos The previous position of the point
    /// @param typeMask The type mask to filter which prisms to consider for collision
    void lookupPoint(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask typeMask) {
        m_prismIter = searchBlock(pos);
        m_pos = pos;
        m_prevPos = prevPos;
        m_movement = pos - prevPos;
        m_typeMask = typeMask;
    }

    /// @addr{0x807C1BB4}
    /// @brief Sets members in preparation of a subsequent sphere collision check call
    /// @param radius The radius of the sphere to check for collisions
    /// @param pos The position of the sphere to search around
    /// @param prevPos The previous position of the sphere
    /// @param typeMask The type mask to filter which prisms to consider for collision
    void lookupSphere(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask typeMask) {
        m_prismIter = searchBlock(pos);
        m_pos = pos;
        m_prevPos = prevPos;
        m_movement = pos - prevPos;
        m_radius = std::min(radius, m_sphereRadius);
        m_typeMask = typeMask;
    }

    void lookupSphereCached(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, u32 typeMask,
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

    /// @addr{0x807BDF54}
    /// @brief Computes a prism vertex based off of the triangle's normal vectors
    /// @param height The height of the prism
    /// @param vertex1 The first vertex of the triangle
    /// @param fnrm The face normal of the triangle
    /// @param enrm3 The edge normal opposite to the third vertex
    /// @param enrm The edge normal opposite to the second vertex
    /// @details @par Triangle Vertices Formula
    /// Given a triangle with vertices \f$\vec{A}, \vec{B}, \vec{C}\f$, face normal \f$\hat{f}\f$,
    /// and height \f$h\f$, label the edge normals by: \f{aligned}{\hat{en}_1 := e_{AB}, \,\,
    /// \hat{en}_2:= e_{AC}, \,\,\hat{en}_3:=e_{BC}}\f} We can recover \f$\vec{B},
    /// \vec{C}\f$ via: \f{aligned}{ \vec{B} = \vec{A} + \dfrac{h}{(\hat{en}_2 \times \hat{f})
    /// \cdot
    /// \hat{en}_3}\left(\hat{en}_2 \times \hat{f}\right), \, \, \vec{C} = \vec{A} +
    /// \dfrac{h}{(\hat{en}_1 \times \hat{f}) \cdot \hat{en}_3}(\hat{en}_1 \times \hat{f}) \, .
    /// }\f}
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
    [[nodiscard]] bool checkSphereCollision(const KCollisionPrism &prism, f32 *distOut,
            EGG::Vector3f *fnrmOut, u16 *flagsOut);

    [[nodiscard]] bool checkPointCollision(const KCollisionPrism &prism, f32 *distOut,
            EGG::Vector3f *fnrmOut, u16 *flagsOut, bool movement);
    [[nodiscard]] bool checkSphereMovement(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *attributeOut);
    [[nodiscard]] bool checkPoint(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *attributeOut,
            bool movement);

    const void *m_posData;      ///< Pointer to the KCL file section containing vertex positions
    const void *m_nrmData;      ///< Pointer to the KCL file section containing normal vectors
    const void *m_prismData;    ///< Pointer to the KCL file section containing the prism data
    const void *m_blockData;    ///< Pointer to the KCL file section containing the octree
    f32 m_prismThickness;       ///< The depth of all prisms along their normal vector
    EGG::Vector3f m_areaMinPos; ///< Smallest possible coordinate in the octree
    u32 m_areaXWidthMask;       ///< The x dimension of the octree's bounding box. @see searchBlock.
    u32 m_areaYWidthMask;       ///< The y dimension of the octree's bounding box. @see searchBlock.
    u32 m_areaZWidthMask;       ///< The z dimension of the octree's bounding box. @see searchBlock.
    u32 m_blockWidthShift;      ///< Used to initialize octree navigation. @see searchBlock.
    u32 m_areaXBlocksShift;     ///< Used to initialize octree navigation. @see searchBlock.
    u32 m_areaXYBlocksShift;    ///< Used to initialize octree navigation. @see searchBlock.
    f32 m_sphereRadius;         ///< Clamps the sphere we check collision against. @see searchBlock.
    EGG::Vector3f m_pos;        ///< The point's/sphere's position for collision queries
    EGG::Vector3f m_prevPos;    ///< The point's/sphere's previous position for collision queries
    EGG::Vector3f m_movement;   ///< The difference between @ref m_pos and @ref m_prevPos
    f32 m_radius;               ///< The radius of the sphere to query collisions for
    KCLTypeMask m_typeMask;     ///< The KCL types to filter the collision query to
    const u16 *m_prismIter;     ///< Iterator pointing to the current prism in the octree traversal
    EGG::BoundBox3f m_bbox;     ///< The 3-dimensional bounding box of all prisms in the KCL file
    std::array<u16, 256> m_prismCache; ///< Cache of prism indices to avoid expensive octree lookups
    u16 *m_prismCacheIter;     ///< Pointer to the current prism index in the cache traversal
    EGG::Vector3f m_cachedPos; ///< Position of the point/sphere corresponding to the current cache
    f32 m_cachedRadius;        ///< Radius of the sphere corresponding to the current cache

    /// @brief One-indexed byte-swapped array of all @ref KCollisionPrism objects in the KCL file
    /// @details Optimizes for time by avoiding unnecessary byteswapping. The Wii doesn't have this
    /// problem because big endian is always assumed.
    owning_span<KCollisionPrism> m_prisms;

    /// @brief Byte-swapped array of all vertex normals in the KCL file
    /// @details Optimizes for time by avoiding unnecessary byteswapping. The Wii doesn't have this
    /// problem because big endian is always assumed.
    owning_span<EGG::Vector3f> m_nrms;

    /// @brief Byte-swapped array of all vertex positions in the KCL file
    /// @details Optimizes for time by avoiding unnecessary byteswapping. The Wii doesn't have this
    /// problem because big endian is always assumed.
    owning_span<EGG::Vector3f> m_vertices;
};

} // namespace Kinoko::Field
