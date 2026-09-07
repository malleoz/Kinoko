#include "ObjectBase.hh"

#include "game/field/ObjectDirector.hh"

#include "game/system/CourseMap.hh"

namespace Kinoko::Field {

/// @addr{0x8081F828}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectBase::ObjectBase(const System::MapdataGeoObj &params)
    : m_drawMdl(nullptr), m_resFile(nullptr), m_id(static_cast<ObjectId>(params.id())),
      m_railInterpolator(nullptr), m_mapObj(&params), m_pos(params.pos()), m_scale(params.scale()),
      m_rot(params.rot() * DEG2RAD), m_rotUpdated(true), m_transform(EGG::Matrix34f::ident) {
    m_flags.setBit(eFlags::Position, eFlags::Rotation, eFlags::Scale);
}

/// @addr{0x8081FB04}
/// @brief Overloaded constructor that constructs the object associated with the provided name
/// @param name The name of the object
/// @param pos The initial position of the object
/// @param rot The initial rotation of the object
/// @param scale The initial scale of the object
ObjectBase::ObjectBase(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
        const EGG::Vector3f &scale)
    : m_drawMdl(nullptr), m_resFile(nullptr), m_railInterpolator(nullptr), m_mapObj(nullptr),
      m_pos(pos), m_scale(scale), m_rot(rot), m_rotUpdated(true),
      m_transform(EGG::Matrix34f::ident) {
    m_flags.setBit(eFlags::Position, eFlags::Rotation, eFlags::Scale);
    m_id = ObjectDirector::Instance()->flowTable().getIdFromName(name);
}

/// @addr{0x8067E3C4}
/// @brief Virtual destructor that deletes the @ref Abstract::g3d::ResFile, @ref Render::DrawMdl,
/// and @ref RailInterpolator instances associated with the object
ObjectBase::~ObjectBase() {
    EGG::egg_delete(m_resFile);
    EGG::egg_delete(m_drawMdl);
    EGG::egg_delete(m_railInterpolator);
}

/// @addr{0x80680730}
/// @brief Fetches the name of the resource file (.brres) associated with the object, if any
/// @return The name of the resource file, or "-" if none is associated
const char *ObjectBase::getResources() const {
    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    const auto *collisionSet = flowTable.set(flowTable.slot(id()));
    ASSERT(collisionSet);
    return collisionSet->resources;
}

/// @addr{0x8081FD10}
/// @brief Loads the resources associated with the object, if any
void ObjectBase::loadGraphics() {
    const char *name = getResources();
    if (strcmp(name, "-") == 0) {
        return;
    }

    char filename[128];
    snprintf(filename, sizeof(filename), "%s.brres", name);

    auto *resMgr = System::ResourceManager::Instance();
    std::span<const u8> resFile = resMgr->getFile(filename, System::ArchiveId::Course);
    if (!resFile.empty()) {
        m_resFile = EGG::egg_new<Abstract::g3d::ResFile>(resFile.data());
        m_drawMdl = EGG::egg_new<Render::DrawMdl>();
    }
}

/// @addr{0x80820980}
/// @brief Loads the rail interpolator for the object, if any
void ObjectBase::loadRail() {
    if (!m_mapObj) {
        return;
    }

    s16 pathId = m_mapObj->pathId();

    if (pathId == -1) {
        return;
    }

    auto *point = System::CourseMap::Instance()->getPointInfo(pathId);
    f32 speed = static_cast<f32>(m_mapObj->setting(0));

    if (point->setting(0) == 0) {
        m_railInterpolator = EGG::egg_new<RailLinearInterpolator>(speed, pathId);
    } else {
        m_railInterpolator = EGG::egg_new<RailSmoothInterpolator>(speed, pathId);
    }
}

/// @addr{0x80680784}
/// @brief Fetches the name of the object
/// @return The name of the object
[[nodiscard]] const char *ObjectBase::getName() const {
    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    const auto *collisionSet = flowTable.set(flowTable.slot(id()));
    ASSERT(collisionSet);
    return collisionSet->name;
}

/// @addr{0x806806DC}
/// @brief Fetches the name of the KCL resource associated with the object
/// @return The name of the KCL resource associated with the object
const char *ObjectBase::getKclName() const {
    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    const auto *collisionSet = flowTable.set(flowTable.slot(id()));
    ASSERT(collisionSet);
    return collisionSet->resources;
}

/// @addr{0x80821640}
/// @brief Updates the transform matrix of the object based on its position and rotation
void ObjectBase::calcTransform() {
    if (m_flags.onBit(eFlags::Rotation)) {
        m_transform.makeRT(m_rot, m_pos);
        m_flags.resetBit(eFlags::Rotation, eFlags::Position);
    } else if (m_flags.onBit(eFlags::Position)) {
        m_transform.setBase(3, m_pos);
        m_flags.setBit(eFlags::Matrix);
    }
}

/// @brief Refreshes the object's rotation vector based on its transform matrix
/// @details This is used when the object's transform matrix is modified directly, such as via
/// @ref setMatrixTangentTo().
void ObjectBase::calcRotLock() {
    if (!m_rotUpdated) {
        m_rotUpdated = true;
        m_rot = m_transform.calcRPY();
    }
}

/// @addr{0x80820EB8}
/// @brief Links animations from the object's resource file to the DrawMdl
/// @param names The names of the animations to link
/// @param types The types of the animations to link
void ObjectBase::linkAnims(const std::span<const char *> &names,
        const std::span<Render::AnmType> types) {
    if (!m_drawMdl) {
        return;
    }

    ASSERT(names.size() == types.size());

    for (size_t i = 0; i < names.size(); ++i) {
        m_drawMdl->linkAnims(i, m_resFile, names[i], types[i]);
    }
}

/// @addr{0x80821910}
/// @brief Sets the object's transformation matrix based on an up vector and a tangent vector
/// @param up The up vector to align the matrix with
/// @param tangent The tangent vector to align the matrix with
void ObjectBase::setMatrixTangentTo(const EGG::Vector3f &up, const EGG::Vector3f &tangent) {
    m_rotUpdated = false;
    m_flags.setBit(eFlags::Matrix);
    SetRotTangentHorizontal(m_transform, up, tangent);
    m_transform.setBase(3, m_pos);
}

/// @addr{0x808218B0}
/// @brief Sets the transformation matrix based on an orthonormal basis and the object's position
/// @param v The vector to construct an orthonormal basis from
void ObjectBase::setMatrixFromOrthonormalBasisAndPos(const EGG::Vector3f &v) {
    m_flags.setBit(eFlags::Matrix);
    m_transform = OrthonormalBasis(v);
    m_transform.setBase(3, m_pos);
}

/// @addr{0x806B38A8}
/// @brief Calculates on what side of line segment ab point lies.
/// @param point The point to check against the line segment
/// @param a The starting point of the line segment
/// @param b The ending point of the line segment
/// @return A positive value if point is on the left side of the line segment, negative if on the
/// right side, and 0 if on the line segment.
f32 ObjectBase::CheckPointAgainstLineSegment(const EGG::Vector3f &point, const EGG::Vector3f &a,
        const EGG::Vector3f &b) {
    return (b.x - a.x) * (point.z - a.z) - (point.x - a.x) * (b.z - a.z);
}

/// @addr{0x806B3900}
/// @brief Rotates a vector around the Y-axis and returns the XZ-plane portion of the vector.
/// @param angle The angle to rotate the vector by
/// @param v The vector to rotate
/// @return The rotated vector in the XZ-plane
EGG::Vector3f ObjectBase::RotateXZByYaw(f32 angle, const EGG::Vector3f &v) {
    f32 y = EGG::Mathf::SinFIdx(RAD2FIDX * (0.5f * angle));
    f32 w = EGG::Mathf::CosFIdx(RAD2FIDX * (0.5f * angle));
    EGG::Quatf quat = EGG::Quatf(w, 0.0f, y, 0.0f);
    return quat.rotateVector(EGG::Vector3f(v.x, 0.0f, v.z));
}

/// @addr{0x806B3AC4}
/// @brief Rotates a vector around an arbitrary axis by a given angle
/// @param angle The angle to rotate the vector by
/// @param axis The axis to rotate the vector around
/// @param v1 The vector to rotate
/// @return The rotated vector
EGG::Vector3f ObjectBase::RotateAxisAngle(f32 angle, const EGG::Vector3f &axis,
        const EGG::Vector3f &v1) {
    EGG::Matrix34f mat;
    mat.setBase(3, EGG::Vector3f::zero);
    mat.setAxisRotation(angle, axis);
    return mat.ps_multVector(v1);
}

/// @addr{0x806B41E0}
/// @brief Sets the rotation of a matrix based on an up vector and a tangent vector
/// @param mat The matrix to set the rotation for
/// @param up The up vector to align the matrix with
/// @param tangent The tangent vector to align the matrix with
void ObjectBase::SetRotTangentHorizontal(EGG::Matrix34f &mat, const EGG::Vector3f &up,
        const EGG::Vector3f &tangent) {
    EGG::Vector3f vec = tangent - up * tangent.dot(up);
    vec.normalise2();

    mat.setBase(0, up.cross(vec));
    mat.setBase(1, up);
    mat.setBase(2, vec);
}

/// @addr{0x806B3CA4}
/// @brief Creates an orthonormal basis from a given vector
/// @param v The vector to construct the orthonormal basis from
/// @return The resulting orthonormal basis as a matrix
EGG::Matrix34f ObjectBase::OrthonormalBasis(const EGG::Vector3f &v) {
    EGG::Vector3f z = v;

    if (EGG::Mathf::abs(z.y) < 0.001f) {
        z.y = 0.001f;
    }

    EGG::Vector3f h = EGG::Vector3f(v.x, 0.0f, v.z);
    h.normalise2();

    EGG::Vector3f x = (z.y > 0.0f) ? -h.cross(z) : h.cross(z);
    x.normalise2();

    EGG::Matrix34f mat;
    mat.setBase(3, EGG::Vector3f::zero);
    mat.setBase(0, x);
    mat.setBase(1, z.cross(x));
    mat.setBase(2, z);

    return mat;
}

/// @addr{0x806B46A4}
/// @brief Creates an orthonormal basis from a rail interpolator
/// @param railInterpolator The rail interpolator to construct the orthonormal basis from
/// @return The resulting orthonormal basis as a matrix
EGG::Matrix34f ObjectBase::RailOrthonormalBasis(const RailInterpolator &railInterpolator) {
    EGG::Matrix34f mat = OrthonormalBasis(railInterpolator.curTangentDir());
    mat.setBase(3, railInterpolator.curPos());
    return mat;
}

/// @addr{0x807DE934}
/// @brief Adjusts a vector based on sideways and forward scalars, ensuring a minimum magnitude
/// @param sidewaysScalar The scalar to apply to the sideways component of the vector
/// @param forwardScalar The scalar to apply to the forward component of the vector
/// @param minSpeed The minimum allowed magnitude for the forward component
/// @param src The vector to adjust
/// @param forward The forward direction vector
/// @return The scaled vector
EGG::Vector3f ObjectBase::AdjustVecForward(f32 sidewaysScalar, f32 forwardScalar, f32 minSpeed,
        const EGG::Vector3f &src, EGG::Vector3f forward) {
    if (forward.y > 0.0f) {
        forward.y = 0.0f;
        auto [mag, tmp] = forward.ps_normalized();

        if (mag <= 0.0f) {
            return src;
        }

        forward = tmp;
    }

    EGG::Vector3f proj = forward * src.ps_dot(forward);
    EGG::Vector3f sideways = (src - proj) * sidewaysScalar;

    EGG::Vector3f newForward = proj * -forwardScalar;
    if (newForward.squaredLength() < minSpeed * minSpeed) {
        newForward = forward * minSpeed;
    }

    return sideways + newForward;
}

} // namespace Kinoko::Field
