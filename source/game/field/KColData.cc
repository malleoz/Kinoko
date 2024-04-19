#include "KColData.hh"

#include <egg/geom/Sphere.hh>
#include <egg/math/Math.hh>

#include <cmath>

// Credit: em-eight/mkw
// Credit: stblr/Hanachan

namespace Field {

/// @addr{0x807BDC5C}
KColData::KColData(const void *file) {
    auto addOffset = [](const void *file, u32 offset) -> const void * {
        return reinterpret_cast<const void *>(reinterpret_cast<const u8 *>(file) + offset);
    };
    u8 *unsafeData = reinterpret_cast<u8 *>(const_cast<void *>(file));
    EGG::RamStream stream = EGG::RamStream(unsafeData, sizeof(KColHeader));

    u32 posOffset = stream.read_u32();
    u32 nrmOffset = stream.read_u32();
    u32 prismOffset = stream.read_u32();
    u32 blockOffset = stream.read_u32();

    m_posData = addOffset(file, posOffset);
    m_nrmData = addOffset(file, nrmOffset);
    m_prismData = addOffset(file, prismOffset);
    m_blockData = addOffset(file, blockOffset);

    m_prismThickness = stream.read_f32();
    m_areaMinPos.read(stream);
    m_areaXWidthMask = stream.read_u32();
    m_areaYWidthMask = stream.read_u32();
    m_areaZWidthMask = stream.read_u32();
    m_blockWidthShift = stream.read_u32();
    m_areaXBlocksShift = stream.read_u32();
    m_areaXYBlocksShift = stream.read_u32();
    m_sphereRadius = stream.read_f32();

    m_pos.setZero();
    m_prevPos.setZero();
    m_movement.setZero();
    m_radius = 0.0f;
    m_prismIter = nullptr;
    m_cachedPrismArray = m_prismCache.data() - 1;
    m_prisms = reinterpret_cast<const KCollisionPrism *>(
            addOffset(m_prismData, sizeof(KCollisionPrism)));
    m_prismCount =
            (reinterpret_cast<uintptr_t>(m_blockData) - reinterpret_cast<uintptr_t>(m_prisms + 1)) /
            sizeof(KCollisionPrism);

    computeBBox();
}

/// @addr{0x807C24C0}
void KColData::narrowScopeLocal(const EGG::Vector3f &pos, f32 radius, KCLTypeMask mask) {
    m_prismCacheTop = m_prismCache.data();
    m_pos = pos;
    m_radius = radius;
    m_typeMask = mask;
    m_cachedPos = pos;
    m_cachedRadius = radius;

    if (radius <= m_sphereRadius) {
        narrowPolygon_EachBlock(searchBlock(pos));
    }

    *m_prismCacheTop = 0;
}

/// @addr{0x807C243C}
void KColData::narrowPolygon_EachBlock(const u16 *prismArray) {
    m_prismIter = prismArray;

    while (checkSphereSingle(nullptr, nullptr, nullptr)) {
        /// We assume the cache has same endianness as the archive file,
        /// so do not parse out the prism index and directly store it in the cache.
        *(m_prismCacheTop++) = *m_prismIter;

        if (m_prismCacheTop == m_prismCache.end()) {
            --m_prismCacheTop;
            return;
        }
    }
}

/// @brief Calculates a EGG::BoundBox3f that describes the boundary of the track's KCL
/// @addr{0x807BDDFC}
void KColData::computeBBox() {
    m_bbox.max.set(-999999.0f);
    m_bbox.min.set(999999.0f);

    for (size_t i = 0; i < m_prismCount; i++) {
        const KCollisionPrism prism = getPrism(i);

        const EGG::Vector3f fnrm = getNrm(prism.fnrm_i);
        const EGG::Vector3f enrm1 = getNrm(prism.enrm1_i);
        const EGG::Vector3f enrm2 = getNrm(prism.enrm2_i);
        const EGG::Vector3f enrm3 = getNrm(prism.enrm3_i);
        const EGG::Vector3f vtx1 = getPos(prism.pos_i);

        const EGG::Vector3f vtx2 = GetVertex(prism.height, vtx1, fnrm, enrm3, enrm1);
        const EGG::Vector3f vtx3 = GetVertex(prism.height, vtx1, fnrm, enrm3, enrm2);

        m_bbox.min = m_bbox.min.minimize(vtx1);
        m_bbox.min = m_bbox.min.minimize(vtx2);
        m_bbox.min = m_bbox.min.minimize(vtx3);
        m_bbox.max = m_bbox.max.maximize(vtx1);
        m_bbox.max = m_bbox.max.maximize(vtx2);
        m_bbox.max = m_bbox.max.maximize(vtx3);
    }
}

/// @addr{0x807C2410}
bool KColData::checkSphereCollision(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut) {
    return std::isfinite(m_prevPos.y) ? checkSphereMovement(distOut, fnrmOut, flagsOut) :
                                        checkSphere(distOut, fnrmOut, flagsOut);
}

/// @brief Iterates the list of looked-up triangles to see if we are colliding
/// @addr{0x807C1514}
/// @param distOut If colliding, returns the distance between the player and the triangle
/// @param fnrmOut If colliding, returns the floor normal of the triangle
/// @param flagsOut If colliding, returns the KCL attributes for that triangle
/// @return whether or not the player is colliding with the triangle
bool KColData::checkSphere(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut) {
    // If there's no list of triangles to check, there's no collision
    if (!m_prismIter) {
        return false;
    }

    // Check collision for all triangles, and continuously call the function until we're out
    while (*++m_prismIter != 0) {
        const KCollisionPrism prism = getPrism(parse<u16>(*m_prismIter));
        if (checkCollision(prism, distOut, fnrmOut, flagsOut, CollisionCheckType::Plane)) {
            return true;
        }
    }

    // We're out of triangles to check - another list must be prepared for subsequent calls
    m_prismIter = nullptr;
    return false;
}

/// @addr{0x807C0F00}
bool KColData::checkSphereSingle(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *flagsOut) {
    if (!m_prismIter) {
        return false;
    }

    while (*++m_prismIter != 0) {
        if (m_prismCacheTop != m_prismCache.begin()) {
            u16 *puVar10 = m_prismCacheTop - 1;
            while (*m_prismIter != *puVar10) {
                if (puVar10-- < m_prismCache.begin()) {
                    break;
                }
            }

            if (puVar10 >= m_prismCache.begin()) {
                continue;
            }
        }

        const KCollisionPrism prism = getPrism(parse<u16>(*m_prismIter));
        if (checkCollision(prism, distOut, fnrmOut, flagsOut, CollisionCheckType::Edge)) {
            return true;
        }
    }

    m_prismIter = nullptr;
    return false;
}

/// @brief Sets members in preparation of a subsequent collision check call
/// @addr{0x807C1BB4}
void KColData::lookupSphere(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask typeMask) {
    m_prismIter = searchBlock(pos);
    m_pos = pos;
    m_prevPos = prevPos;
    m_movement = pos - prevPos;
    m_radius = std::min(radius, m_sphereRadius);
    m_typeMask = typeMask;
}

/// @addr{0x807C1DE8}
void KColData::lookupSphereCached(const EGG::Vector3f &p1, const EGG::Vector3f &p2, u32 typeMask,
        f32 radius) {
    EGG::Sphere3f sphere1(p1, radius);
    EGG::Sphere3f sphere2(m_cachedPos, m_cachedRadius);

    if (!sphere1.isInsideOtherSphere(sphere2)) {
        m_prismIter = searchBlock(p1);
        m_radius = std::min(m_sphereRadius, radius);
    } else {
        m_radius = radius;
        m_prismIter = m_cachedPrismArray;
    }

    m_pos = p1;
    m_prevPos = p2;
    m_movement = p1 - p2;
    m_typeMask = typeMask;
}

/// @brief Finds the data block corresponding to the provided position
/// @addr{0x807BE030}
/// @param point The player's position
/// @return the address of the leaf node containing the input point.
const u16 *KColData::searchBlock(const EGG::Vector3f &point) {
    // Calculate the x, y, and z offsets of the point from the minimum
    // corner of the tree's bounding box.
    const int x = point.x - m_areaMinPos.x;
    const int y = point.y - m_areaMinPos.y;
    const int z = point.z - m_areaMinPos.z;

    // Check if the point is outside the tree's bounding box in the x, y,
    // or z dimensions. If it is, return 0.
    if (x & m_areaXWidthMask || y & m_areaYWidthMask || z & m_areaZWidthMask) {
        return nullptr;
    }

    // Initialize the current tree node to the root node of the tree.
    u32 shift = m_blockWidthShift;
    const u8 *curBlock = reinterpret_cast<const u8 *>(m_blockData);
    s32 offset;

    // Traverse the tree to find the leaf node containing the input point.
    u32 index = 4 *
            (((u32)z >> shift) << m_areaXYBlocksShift | ((u32)y >> shift) << m_areaXBlocksShift |
                    (u32)x >> shift);

    while (true) {
        // Get the offset of the current node's child node.
        offset = parse<u32>(*reinterpret_cast<const u32 *>(curBlock + index));

        // If the offset is negative, the current node is a leaf node.
        if ((offset & 0x80000000) != 0) {
            break;
        }

        // If the offset is non-negative, update the current node to be
        // the child node and continue traversing the tree.
        shift--;
        curBlock += offset;

        u32 x_shift = ((1 * ((u32)x >> shift)) & 1);
        u32 y_shift = ((2 * ((u32)y >> shift)) & 2);
        u32 z_shift = ((4 * ((u32)z >> shift)) & 4);

        index = 4 * (x_shift | y_shift | z_shift);
    }

    // We have to remove the MSB since it's solely used to identify leaves.
    return reinterpret_cast<const u16 *>(curBlock + (offset & ~0x80000000));
}

EGG::Vector3f KColData::getPos(u16 posIdx) const {
    const EGG::Vector3f *vec = &reinterpret_cast<const EGG::Vector3f *>(m_posData)[posIdx];
    u8 *unsafeData = reinterpret_cast<u8 *>(const_cast<EGG::Vector3f *>(vec));
    EGG::RamStream stream = EGG::RamStream(unsafeData, sizeof(EGG::Vector3f));
    EGG::Vector3f pos;
    pos.read(stream);
    return pos;
}

EGG::Vector3f KColData::getNrm(u16 nrmIdx) const {
    const EGG::Vector3f *vec = &reinterpret_cast<const EGG::Vector3f *>(m_nrmData)[nrmIdx];
    u8 *unsafeData = reinterpret_cast<u8 *>(const_cast<EGG::Vector3f *>(vec));
    EGG::RamStream stream = EGG::RamStream(unsafeData, sizeof(EGG::Vector3f));
    EGG::Vector3f nrm;
    nrm.read(stream);
    return nrm;
}

KColData::KCollisionPrism KColData::getPrism(u16 prismIdx) const {
    const KCollisionPrism *prism =
            &reinterpret_cast<const KCollisionPrism *>(m_prismData)[prismIdx];
    u8 *unsafeData = reinterpret_cast<u8 *>(const_cast<KCollisionPrism *>(prism));
    EGG::RamStream stream = EGG::RamStream(unsafeData, sizeof(KCollisionPrism));

    f32 height = stream.read_f32();
    u16 posIndex = stream.read_u16();
    u16 faceNormIndex = stream.read_u16();
    u16 edge1NormIndex = stream.read_u16();
    u16 edge2NormIndex = stream.read_u16();
    u16 edge3NormIndex = stream.read_u16();
    u16 attribute = stream.read_u16();

    return KCollisionPrism(height, posIndex, faceNormIndex, edge1NormIndex, edge2NormIndex,
            edge3NormIndex, attribute);
}

u16 KColData::prismCache(u32 idx) const {
    return m_prismCache[idx];
}

/// @brief Computes a prism vertex based off of the triangle's normal vectors
/// @addr{0x807BDF54}
/// @par Triangle Vertices Formula
/// Given a triangle with vertices \f$\vec{A}, \vec{B}, \vec{C}\f$, face normal \f$\vec{f}\f$, and
/// height \f$h\f$, label the edge normals by: \begin{aligned}\vec{en}_1 := e_{AB}, \,\,
/// \vec{en}_2:= e_{AC}, \,\,\vec{en}_3:=e_{BC} \end{aligned} We can recover \f$\vec{B}, \vec{C}\f$
/// via: \begin{aligned} \vec{B} = \vec{A} + \dfrac{h}{(\vec{en}_2 \times \vec{f}) \cdot
/// \vec{en}_3}\left(\vec{en}_2 \times \vec{f}\right), \, \, \vec{C} = \vec{A} +
/// \dfrac{h}{(\vec{en}_1 \times \vec{f}) \cdot \vec{en}_3}(\vec{en}_1 \times \vec{f}) \, .
/// \end{aligned}
EGG::Vector3f KColData::GetVertex(f32 height, const EGG::Vector3f &vertex1,
        const EGG::Vector3f &fnrm, const EGG::Vector3f &enrm3, const EGG::Vector3f &enrm) {
    EGG::Vector3f cross = fnrm.cross(enrm);
    f32 dp = cross.ps_dot(enrm3);
    cross *= (height / dp);

    return cross + vertex1;
}

/// @brief This is a combination of the three collision checks in the base game.
/// @details The checks vary only by a few if-statements, related to whether we are checking for:
/// 1. A collision with at least the triangle edge (0x807C0F00)
/// 2. A collision with the triangle plane (0x807C1514)
/// 3. A collision such that we are inside the triangle (0x807C0884)
bool KColData::checkCollision(const KCollisionPrism &prism, f32 *distOut, EGG::Vector3f *fnrmOut,
        u16 *flagsOut, CollisionCheckType type) {
    // Responsible for updating the output params
    auto out = [&](f32 dist) {
        if (distOut) {
            *distOut = dist;
        }
        if (fnrmOut) {
            *fnrmOut = getNrm(prism.fnrm_i);
        }
        if (flagsOut) {
            *flagsOut = prism.attribute;
        }
        return true;
    };

    // The flag check occurs earlier than in the base game here. We don't want to do math if the tri
    // we're checking doesn't have matching flags.
    u32 attributeMask = KCL_ATTRIBUTE_TYPE_BIT(prism.attribute);
    if (!(attributeMask & m_typeMask)) {
        return false;
    }

    const EGG::Vector3f relativePos = m_pos - getPos(prism.pos_i);

    // Edge normals point outside the triangle
    const EGG::Vector3f enrm1 = getNrm(prism.enrm1_i);
    f32 dist_ca = relativePos.ps_dot(enrm1);
    if (m_radius <= dist_ca) {
        return false;
    }

    const EGG::Vector3f enrm2 = getNrm(prism.enrm2_i);
    f32 dist_ab = relativePos.ps_dot(enrm2);
    if (m_radius <= dist_ab) {
        return false;
    }

    const EGG::Vector3f enrm3 = getNrm(prism.enrm3_i);
    f32 dist_bc = relativePos.ps_dot(enrm3) - prism.height;
    if (m_radius <= dist_bc) {
        return false;
    }

    const EGG::Vector3f fnrm = getNrm(prism.fnrm_i);
    f32 plane_dist = relativePos.ps_dot(fnrm);
    f32 dist_in_plane = m_radius - plane_dist;
    if (dist_in_plane <= 0.0f) {
        return false;
    }

    f32 typeDistance = m_prismThickness;
    if (type == CollisionCheckType::Edge) {
        typeDistance += m_radius;
    }

    if (dist_in_plane >= typeDistance) {
        return false;
    }

    if (type == CollisionCheckType::Movement) {
        if (attributeMask & KCL_TYPE_DIRECTIONAL && m_movement.dot(fnrm) > 0.0f) {
            return false;
        }
    }

    // Originally part of the edge searching, but moved out for simplicity
    // If these are all zero, then we're inside the triangle
    if (dist_ab <= 0.0f && dist_bc <= 0.0f && dist_ca <= 0.0f) {
        if (type == CollisionCheckType::Movement) {
            EGG::Vector3f lastPos = relativePos - m_movement;
            // We're only colliding if we are moving towards the face
            if (plane_dist < 0.0f && lastPos.dot(fnrm) < 0.0f) {
                return false;
            }
        }
        return out(dist_in_plane);
    }

    EGG::Vector3f edge_nor, other_edge_nor;
    f32 edge_dist, other_edge_dist;
    // > means further, < means closer, = means same distance
    if (dist_ab >= dist_ca && dist_ab > dist_bc) {
        // AB is the furthest edge
        edge_nor = enrm2;
        edge_dist = dist_ab;
        if (dist_ca >= dist_bc) {
            // CA is the second furthest edge
            other_edge_nor = enrm1;
            other_edge_dist = dist_ca;
        } else {
            // BC is the second furthest edge
            other_edge_nor = enrm3;
            other_edge_dist = dist_bc;
        }
    } else if (dist_bc >= dist_ca) {
        // BC is the furthest edge
        edge_nor = enrm3;
        edge_dist = dist_bc;
        if (dist_ab >= dist_ca) {
            // AB is the second furthest edge
            other_edge_nor = enrm2;
            other_edge_dist = dist_ab;
        } else {
            // CA is the second furthest edge
            other_edge_nor = enrm1;
            other_edge_dist = dist_ca;
        }
    } else {
        // CA is the furthest edge
        edge_nor = enrm1;
        edge_dist = dist_ca;
        if (dist_bc >= dist_ab) {
            // BC is the second furthest edge
            other_edge_nor = enrm3;
            other_edge_dist = dist_bc;
        } else {
            // AB is the second furthest edge
            other_edge_nor = enrm2;
            other_edge_dist = dist_ab;
        }
    }

    f32 cos = edge_nor.ps_dot(other_edge_nor);
    f32 sq_dist;
    if (cos * edge_dist > other_edge_dist) {
        if (type == CollisionCheckType::Plane) {
            if (edge_dist > plane_dist) {
                return false;
            }
        }
        sq_dist = m_radius * m_radius - edge_dist * edge_dist;
    } else {
        f32 sq_sin = cos * cos - 1.0f;
        f32 t = (cos * edge_dist - other_edge_dist) / sq_sin;
        f32 s = edge_dist - t * cos;
        const EGG::Vector3f corner_pos = edge_nor * s + other_edge_nor * t;

        if (type == CollisionCheckType::Plane) {
            if (corner_pos.dot() > plane_dist * plane_dist) {
                return false;
            }
        }

        sq_dist = m_radius * m_radius - corner_pos.dot();
    }

    if (sq_dist < plane_dist * plane_dist || sq_dist <= 0.0f) {
        return false;
    }

    f32 dist = EGG::Mathf::sqrt(sq_dist) - plane_dist;

    if (type == CollisionCheckType::Movement) {
        EGG::Vector3f lastPos = relativePos - m_movement;
        // We're only colliding if we are moving towards the face
        if (lastPos.dot(fnrm) < 0.0f) {
            return false;
        }
    }

    return out(dist);
}

/// @brief Iterates the local data block to check for directional collision
/// @addr{0x807C0884}
/// @param distOut If colliding, returns the distance between the player and the tri
/// @param fnrmOut If colliding, returns the floor normal of the triangle
/// @param attributeOut If colliding, returns the KCL attributes for that triangle
/// @return Whether or not a collision has occurred
bool KColData::checkSphereMovement(f32 *distOut, EGG::Vector3f *fnrmOut, u16 *attributeOut) {
    // If there's no list of triangles to check, there's no collision
    if (!m_prismIter) {
        return false;
    }

    // Check collision for all triangles, and continuously call the function until we're out
    while (*++m_prismIter != 0) {
        const KCollisionPrism prism = getPrism(parse<u16>(*m_prismIter));
        if (checkCollision(prism, distOut, fnrmOut, attributeOut, CollisionCheckType::Movement)) {
            return true;
        }
    }

    // We're out of triangles to check - another list must be prepared for subsequent calls
    m_prismIter = nullptr;
    return false;
}

KColData::KCollisionPrism::KCollisionPrism() = default;

KColData::KCollisionPrism::KCollisionPrism(f32 height, u16 posIndex, u16 faceNormIndex,
        u16 edge1NormIndex, u16 edge2NormIndex, u16 edge3NormIndex, u16 attribute)
    : height(height), pos_i(posIndex), fnrm_i(faceNormIndex), enrm1_i(edge1NormIndex),
      enrm2_i(edge2NormIndex), enrm3_i(edge3NormIndex), attribute(attribute) {}

} // namespace Field
