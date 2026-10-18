#include "MapdataArea.hh"

#include "game/system/CourseMap.hh"

#include <egg/math/Quat.hh>

namespace Kinoko::System {

/// @addr{0x80516050}
/// @brief Constructor that parses the raw area data
/// @param data Pointer to the raw area data
/// @param index Index of the entry in the AREA section of the KMP
/// @details Creates a @ref EGG::RamStream to read the raw area data, taking into account the KMP
/// file format version which may or may not contain rail indices in the `AREA` section. Initializes
/// @ref m_boundingRadiusSq, @ref m_ellipseRatio, and @ref m_ellipseRadiusSq to zero. Initializes
/// the dimension vectors to zero as well.
MapdataAreaBase::MapdataAreaBase(const SData *data, s16 index) : m_rawData(data), m_index(index) {
    EGG::RamStream stream = EGG::RamStream(data,
            CourseMap::Instance()->hasRailIdx() ? sizeof(SData) : sizeof(SData) - 4);
    read(stream);

    m_boundingRadiusSq = 0.0f;
    m_ellipseRatio = 0.0f;
    m_ellipseRadiusSq = 0.0f;
    m_dimensions = EGG::Vector3f::zero;
    m_right = EGG::Vector3f::zero;
    m_up = EGG::Vector3f::zero;
    m_forward = EGG::Vector3f::zero;
}

/// @brief Default destructor
MapdataAreaBase::~MapdataAreaBase() = default;

/// @brief Parses the raw area data from the provided stream
/// @param stream The stream from which to read the raw area data
/// @note Earlier revisions of the KMP file format do not include the rail ID field. Even though all
/// vanilla course KMP files have revision 2520, we add this check for posterity's sake.
void MapdataAreaBase::read(EGG::RamStream &stream) {
    stream.skip(1);
    m_type = static_cast<Type>(stream.read_s8());
    stream.skip(1);
    m_priority = stream.read_u8();
    m_position.read(stream);
    m_rotation.read(stream);
    m_scale.read(stream);

    for (auto &param : m_params) {
        param = stream.read_s16();
    }

    if (CourseMap::Instance()->hasRailIdx()) {
        m_railId = stream.read_s8();
    }
}

/// @addr{0x80516168}
/// @brief Fetches the @ref MapdataPointInfo for the associated rail, if any
/// @return Pointer to the @ref MapdataPointInfo for the associated rail, or `nullptr` if none
/// exists
/// @note Earlier revisions of the KMP file format do not include the rail ID field. Even though all
/// vanilla course KMP files have revision 2520, we add this check for posterity's sake.
const MapdataPointInfo *MapdataAreaBase::getPointInfo() const {
    auto *courseMap = CourseMap::Instance();
    if (courseMap->version() < 2200) {
        return nullptr;
    }

    return courseMap->getPointInfo(m_railId);
}

/// @brief Gets the type of area (e.g. @ref Type::MovingRoad)
/// @return The type of the area
MapdataAreaBase::Type MapdataAreaBase::type() const {
    return m_type;
}

/// @brief Gets the priority of the area
/// @return The priority of the area
u8 MapdataAreaBase::priority() const {
    return m_priority;
}

/// @brief Fetches the area setting at the specified index
/// @param i The index of the setting to fetch
/// @return The value of the specified setting
s16 MapdataAreaBase::param(size_t i) const {
    ASSERT(i < m_params.size());
    return m_params[i];
}

/// @brief Gets the index of the entry in the AREA section of the KMP
/// @return The index of the entry in the AREA section of the KMP
s16 MapdataAreaBase::index() const {
    return m_index;
}

/// @addr{0x80516220}
/// @brief Constructor
/// @param data Pointer to the raw area data
/// @param index Index of the entry in the AREA section of the KMP
/// @details Initializes @ref m_dimensions to reflect a box whose length in each dimension is
/// `10000.0f` units times @ref m_scale. Initializes @ref m_ellipseRatio and @ref m_ellipseRadiusSq
/// to zero since these members are not used for box-shaped areas. Computes @ref m_boundingRadiusSq
/// by getting the squared length of @ref m_dimensions. Initializes the world orientation vectors
/// based on the area's @ref m_rotation.
MapdataAreaBox::MapdataAreaBox(const SData *data, s16 index) : MapdataAreaBase(data, index) {
    m_dimensions.x = 0.5f * (10000.0f * m_scale.x);
    m_dimensions.y = 10000.0f * m_scale.y;
    m_dimensions.z = 0.5f * (10000.0f * m_scale.z);

    m_ellipseRatio = 0.0f;
    m_ellipseRadiusSq = 0.0f;
    m_boundingRadiusSq = m_dimensions.squaredLength();

    EGG::Quatf rotation = EGG::Quatf::FromRPY(DEG2RAD * m_rotation.x, DEG2RAD * m_rotation.y,
            DEG2RAD * m_rotation.z);

    m_right = rotation.rotateVector(EGG::Vector3f::ex);
    m_up = rotation.rotateVector(EGG::Vector3f::ey);
    m_forward = rotation.rotateVector(EGG::Vector3f::ez);
}

/// @brief Default virtual destructor
MapdataAreaBox::~MapdataAreaBox() = default;

/// @addr{0x805163F4}
/// @copydoc MapdataAreaBase::testImpl()
/// @details Computes the relative position of @ref pos with respect to @ref m_position. If the
/// relative position lies outside the box's dimensions along any axis, the function returns
/// `false`. Otherwise, it returns `true`.
bool MapdataAreaBox::testImpl(const EGG::Vector3f &pos) const {
    EGG::Vector3f relPos = pos - m_position;

    f32 u = relPos.dot(m_up);
    if (u > m_dimensions.y || u < 0.0f) {
        return false;
    }

    f32 r = relPos.dot(m_right);
    if (r > m_dimensions.x || r < -m_dimensions.x) {
        return false;
    }

    f32 f = relPos.dot(m_forward);
    if (f > m_dimensions.z || f < -m_dimensions.z) {
        return false;
    }

    return true;
}

/// @addr{0x805164FC}
/// @brief Constructor
/// @param data Pointer to the raw area data
/// @param index Index of the entry in the AREA section of the KMP
/// @details Initializes @ref m_dimensions to `5000.0f` units times @ref m_scale. Computes @ref
/// m_ellipseRadiusSq and @ref m_boundingRadiusSq based on the dimensions. Initializes @ref
/// m_ellipseRatio based on the ratio between the X and Z-axis scales. Computes the world
/// orientation vectors based on the area's @ref m_rotation.
MapdataAreaCylinder::MapdataAreaCylinder(const SData *data, s16 index)
    : MapdataAreaBase(data, index) {
    m_dimensions = m_scale * 5000.0f;
    m_ellipseRadiusSq = m_dimensions.x * m_dimensions.x;
    m_boundingRadiusSq = m_dimensions.x * m_dimensions.x + m_dimensions.z * m_dimensions.z;
    m_ellipseRatio = m_scale.x / m_scale.z;

    EGG::Quatf rotation = EGG::Quatf::FromRPY(DEG2RAD * m_rotation.x, DEG2RAD * m_rotation.y,
            DEG2RAD * m_rotation.z);

    m_right = rotation.rotateVector(EGG::Vector3f::ex);
    m_up = rotation.rotateVector(EGG::Vector3f::ey);
    m_forward = rotation.rotateVector(EGG::Vector3f::ez);
}

/// @brief Default virtual destructor
MapdataAreaCylinder::~MapdataAreaCylinder() = default;

/// @addr{0x80516688}
/// @copydoc MapdataAreaBase::testImpl()
/// @details Computes the relative position of @ref pos with respect to @ref m_position. If the
/// relative position lies outside the cylinder's height or elliptical cross-section, the function
/// returns `false`. Otherwise, it returns `true`.
bool MapdataAreaCylinder::testImpl(const EGG::Vector3f &pos) const {
    EGG::Vector3f relPos = pos - m_position;

    if (relPos.dot(m_up) > m_dimensions.y) {
        return false;
    }

    f32 f = m_ellipseRatio * relPos.dot(m_forward);
    f32 r = relPos.dot(m_right);
    if (r * r + f * f < m_ellipseRadiusSq) {
        return false;
    }

    return true;
}

/// @addr{0x80515E50}
/// @brief Constructor
/// @param header Pointer to the header of the AREA section
/// @details Constructs all entries and sorts them by priority to @ref m_sortedEntries
MapdataAreaAccessor::MapdataAreaAccessor(const MapSectionHeader *header)
    : MapdataAccessorBase<MapdataAreaBase, MapdataAreaBase::SData>(header) {
    init(reinterpret_cast<const MapdataAreaBase::SData *>(m_sectionHeader + 1),
            parse<u16>(m_sectionHeader->count));

    m_sortedEntries = owning_span<MapdataAreaBase *>(size());
}

/// @addr{0x80518BDC}
/// @brief Default virtual destructor
MapdataAreaAccessor::~MapdataAreaAccessor() = default;

/// @brief Constructs all AREA entries based off the raw AREA section data
/// @param start Pointer to the start of the raw data
/// @param count The number of entries in the AREA section
/// @details Iterates over the raw `AREA` section data, keeping in mind that the rail index is not
/// present in KMP file format versions prior to 2200. For each entry, constructs the appropriate
/// `MapdataAreaBase` derived object based on the @ref MapdataAreaBase::Shape.
void MapdataAreaAccessor::init(const MapdataAreaBase::SData *start, u16 count) {
    if (count != 0) {
        m_entries.reserve(count);
    }

    for (u16 i = 0; i < count; ++i) {
        // Iterators will be wrong for versions < 2200, so we need to factor out the new members
        const MapdataAreaBase::SData *data = CourseMap::Instance()->hasRailIdx() ?
                &start[i] :
                reinterpret_cast<const MapdataAreaBase::SData *>(
                        reinterpret_cast<uintptr_t>(start) +
                        i * (sizeof(MapdataAreaBase::SData) - 4));

        // Reading this is safe because 8-bit integers are immune to endianness issues
        MapdataAreaBase::Shape shape = static_cast<MapdataAreaBase::Shape>(data->shape);

        switch (shape) {
        case MapdataAreaBase::Shape::Box:
            m_entries.push_back(EGG::egg_new<MapdataAreaBox>(data, i));
            break;
        case MapdataAreaBase::Shape::Cylinder:
            m_entries.push_back(EGG::egg_new<MapdataAreaCylinder>(data, i));
            break;
        default:
            PANIC("Invalid area shape!");
            break;
        }
    }
}

/// @addr{0x80515F8C}
/// @brief Sorts the `AREA` entries by priority in descending order
/// @details Performs insertion sort such that entries are ordered by priority in descending order.
/// The result is stored in @ref m_sortedEntries.
void MapdataAreaAccessor::sort() {
    for (size_t i = 0; i < size(); ++i) {
        m_sortedEntries[i] = get(i);
    }

    for (size_t i = 1; i < size(); ++i) {
        size_t j = i;
        for (; j > 0 && m_sortedEntries[j - 1]->priority() < m_sortedEntries[i]->priority(); --j) {
            m_sortedEntries[j] = m_sortedEntries[j - 1];
        }
    }
}

/// @brief Gets the sorted entry at the specified index
/// @param i The index of the sorted entry to retrieve
/// @return A pointer to the sorted entry at the specified index, or `nullptr` if the
/// index is out of bounds
/// @pre Requires that @ref m_sortedEntries has already been initialized.
MapdataAreaBase *MapdataAreaAccessor::getSorted(u16 i) const {
    ASSERT(!m_sortedEntries.empty());
    return i < m_sortedEntries.size() ? m_sortedEntries[i] : nullptr;
}

} // namespace Kinoko::System
