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

    void reset();
    void updateBBox(const EGG::Vector3f &offset);
    void transformInfo(CollisionInfoPartial &rhs, const EGG::Matrix34f &mtx);
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

    void updateBBox(const EGG::Vector3f &offset);
    void updateFloor(f32 dist, const EGG::Vector3f &fnrm);
    void updateWall(f32 dist, const EGG::Vector3f &fnrm);
    void reset();
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
        f32 height;    ///< The height of the tri
        u16 pos_i;     ///< Index of the first vertex's index in @ref m_vertices
        u16 fnrm_i;    ///< Index of the face normal in @ref m_nrms
        u16 enrm1_i;   ///< Index of the first edge's normal in @ref m_nrms
        u16 enrm2_i;   ///< Index of the second edge's normal in @ref m_nrms
        u16 enrm3_i;   ///< Index of the third edge's normal in @ref m_nrms
        u16 attribute; ///< KCL attribute of the tri

        /// @brief Non-initializing default constructor
        KCollisionPrism() = default;

        /// @brief Default destructor
        ~KCollisionPrism() = default;

        /// @brief Initializes the prism by reading its data from the provided stream
        /// @param stream The stream from which to read the prism data
        void read(EGG::RamStream &stream) {
            height = stream.read_f32();
            pos_i = stream.read_u16();
            fnrm_i = stream.read_u16();
            enrm1_i = stream.read_u16();
            enrm2_i = stream.read_u16();
            enrm3_i = stream.read_u16();
            attribute = stream.read_u16();
        }
    };
    STATIC_ASSERT(sizeof(KCollisionPrism) == 0x10);

    KColData(const void *file);
    ~KColData();

    void narrowScopeLocal(const EGG::Vector3f &pos, f32 radius, KCLTypeMask mask);

    [[nodiscard]] bool checkPointCollision(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut);
    [[nodiscard]] bool checkSphereCollision(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut);

    void lookupPoint(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask typeMask);
    void lookupSphere(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask typeMask);
    void lookupSphereCached(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            u32 typeMask);

    /// @beginGetters

    [[nodiscard]] const EGG::BoundBox3f &bbox() const;
    [[nodiscard]] u16 prismCache(u32 idx) const;
    [[nodiscard]] std::span<const KCollisionPrism> prisms() const;
    [[nodiscard]] std::span<const EGG::Vector3f> nrms() const;
    [[nodiscard]] std::span<const EGG::Vector3f> vertices() const;

    /// @endGetters

private:
    void preloadPrisms();
    void preloadNormals();
    void preloadVertices();
    void computeBBox();

    void narrowPolygon_EachBlock(const u16 *prismArray);
    [[nodiscard]] const u16 *searchBlock(const EGG::Vector3f &pos);
    [[nodiscard]] bool checkSphereSingle(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut);
    [[nodiscard]] bool checkPoint(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *attributeOut,
            bool movement);
    [[nodiscard]] bool checkPointCollision(const KCollisionPrism &prism, f32 *distOut,
            EGG::Vector3f *fnrmOut, u16 *flagsOut, bool movement);

    template <CollisionCheckType Type>
        requires(Type == CollisionCheckType::Plane || Type == CollisionCheckType::Movement)
    [[nodiscard]] bool checkSphereImpl(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut);

    template <CollisionCheckType Type>
    [[nodiscard]] bool checkSphereCollision(const KCollisionPrism &prism, f32 *distOut,
            EGG::Vector3f *fnrmOut, u16 *flagsOut);

    const void *m_posData;      ///< Pointer to the KCL file section containing vertex positions
    const void *m_nrmData;      ///< Pointer to the KCL file section containing normal vectors
    const void *m_prismData;    ///< Pointer to the KCL file section containing the prism data
    const void *m_blockData;    ///< Pointer to the KCL file section containing the octree
    f32 m_prismThickness;       ///< The depth of all prisms along their normal vector
    EGG::Vector3f m_areaMinPos; ///< Smallest possible coordinate in the octree
    u32 m_areaXWidthMask;     ///< The x dimension of the octree's bounding box. @see searchBlock()
    u32 m_areaYWidthMask;     ///< The y dimension of the octree's bounding box. @see searchBlock()
    u32 m_areaZWidthMask;     ///< The z dimension of the octree's bounding box. @see searchBlock()
    u32 m_blockWidthShift;    ///< Used to initialize octree navigation. @see searchBlock()
    u32 m_areaXBlocksShift;   ///< Used to initialize octree navigation. @see searchBlock()
    u32 m_areaXYBlocksShift;  ///< Used to initialize octree navigation. @see searchBlock()
    f32 m_sphereRadius;       ///< Clamps the sphere we check collision against. @see searchBlock()
    EGG::Vector3f m_pos;      ///< The point's/sphere's position for collision queries
    EGG::Vector3f m_prevPos;  ///< The point's/sphere's previous position for collision queries
    EGG::Vector3f m_movement; ///< The difference between @ref m_pos and @ref m_prevPos
    f32 m_radius;             ///< The radius of the sphere to query collisions for
    KCLTypeMask m_typeMask;   ///< The KCL types to filter the collision query to
    const u16 *m_prismIter;   ///< Iterator pointing to the current prism in the octree traversal
    EGG::BoundBox3f m_bbox;   ///< The 3-dimensional bounding box of all prisms in the KCL file
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

    [[nodiscard]] static EGG::Vector3f GetVertex(f32 height, const EGG::Vector3f &vertex1,
            const EGG::Vector3f &fnrm, const EGG::Vector3f &enrm3, const EGG::Vector3f &enrm);
};

} // namespace Kinoko::Field
