#include "MapdataStartPoint.hh"

#include "game/system/CourseMap.hh"

namespace Kinoko::System {

// We have to define these early so they're available for findKartStartPoint
static constexpr s8 X_TRANSLATION_TABLE[MAX_PLAYERS][MAX_PLAYERS] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {-5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {-10, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {-10, 5, -5, 10, 0, 0, 0, 0, 0, 0, 0, 0},
        {-10, 0, 10, -5, 5, 0, 0, 0, 0, 0, 0, 0},
        {-10, -2, 6, -6, 2, 10, 0, 0, 0, 0, 0, 0},
        {-5, 5, -10, 0, 10, -5, 5, 0, 0, 0, 0, 0},
        {-10, 0, 10, -5, 5, -10, 0, 10, 0, 0, 0, 0},
        {-10, -2, 6, -6, 2, 10, -10, -2, 6, 0, 0, 0},
        {-10, 0, 10, -5, 5, -10, 0, 10, -5, 5, 0, 0},
        {-10, -2, 6, -6, 2, 10, -10, -2, 6, -6, 2, 0},
        {-10, -2, 6, -6, 2, 10, -10, -2, 6, -6, 2, 10},
};

static constexpr s8 Z_TRANSLATION_TABLE[MAX_PLAYERS][MAX_PLAYERS] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 1, 1, 1, 2, 2, 0, 0, 0, 0, 0},
        {0, 0, 0, 1, 1, 2, 2, 2, 0, 0, 0, 0},
        {0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0},
        {0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 0, 0},
        {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 0},
        {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3},
};

/// @brief Constructor
/// @param data Pointer to the raw starting point data
MapdataStartPoint::MapdataStartPoint(const SData *data) : m_rawData(data) {
    EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
    read(stream);
}

/// @brief Reads the starting point data from the given stream
/// @param stream The stream to read the starting point data from
void MapdataStartPoint::read(EGG::Stream &stream) {
    m_position.read(stream);
    m_rotation.read(stream);
    if (CourseMap::Instance()->version() > 1830) {
        m_playerIndex = stream.read_s16();
    } else {
        m_playerIndex = 0;
    }
}

/// @addr{0x80514368}
/// @brief Calculates the starting position and rotation for a kart based on the kart's ranking and
/// player count
/// @param pos Output parameter for the calculated starting position
/// @param rot Output parameter for the calculated starting rotation
/// @param placement The ranking or placement of the kart
/// @param playerCount The total number of players in the race
void MapdataStartPoint::findKartStartPoint(EGG::Vector3f &pos, EGG::Vector3f &rot, u8 placement,
        u8 playerCount) {
    EGG::Quatf rotation = EGG::Quatf::FromRPY(m_rotation * DEG2RAD);
    EGG::Vector3f backward = rotation.rotateVector(-EGG::Vector3f::ez);
    EGG::Vector3f left = rotation.rotateVector(-EGG::Vector3f::ex);

    CourseMap *courseMap = CourseMap::Instance();
    const MapdataStageInfo *stageInfo = courseMap->getStageInfo();
    ASSERT(stageInfo);
    int translationDirection = stageInfo->polePosition() == 1 ? -1 : 1;

    f32 cos = EGG::Mathf::CosFIdx(courseMap->skewAngle() * DEG2FIDX);
    f32 sin = EGG::Mathf::SinFIdx(courseMap->skewAngle() * DEG2FIDX) * translationDirection;

    int xTranslation = translationDirection * X_TRANSLATION_TABLE[playerCount - 1][0];
    f32 xScalar = sin *
            (courseMap->lateralSpacing() * (static_cast<f32>(xTranslation) + 10.0f) / 10.0f) / cos;
    EGG::Vector3f xTmp = -backward * xScalar;

    int zTranslation = Z_TRANSLATION_TABLE[playerCount - 1][placement];
    f32 zScalar = courseMap->longitudinalOffset() * static_cast<f32>(zTranslation / 2) +
            courseMap->longitudinalSpacing() * static_cast<f32>(zTranslation) +
            courseMap->longitudinalWideOffset() * static_cast<f32>((zTranslation + 1) / 2);
    EGG::Vector3f zTmp = backward * zScalar;

    EGG::Vector3f tmp0 = xTmp + zTmp;
    EGG::Vector3f tmp1 = left * courseMap->lateralSpacing();
    EGG::Vector3f tmp2 = tmp0 - tmp1;
    EGG::Vector3f tmpPos = tmp2 + m_position;

    EGG::Vector3f vCos = left * cos;
    EGG::Vector3f vSin = backward * sin;
    EGG::Vector3f vRes = vCos + vSin;

    int tmpTranslation = translationDirection * X_TRANSLATION_TABLE[playerCount - 1][placement];
    f32 tmpScalar = courseMap->lateralSpacing() * (static_cast<f32>(tmpTranslation) + 10.0f) /
            (cos * 10.0f);
    EGG::Vector3f tmpRes = vRes * tmpScalar;

    pos = tmpPos + tmpRes;
    rot = m_rotation;
}

/// @addr{0x80514258}
/// @brief Constructor
/// @param header Pointer to the section header of the KTPT section in the course KMP
MapdataStartPointAccessor::MapdataStartPointAccessor(const MapSectionHeader *header)
    : MapdataAccessorBase<MapdataStartPoint, MapdataStartPoint::SData>(header) {
    if (CourseMap::Instance()->version() > 1830) {
        init(reinterpret_cast<const MapdataStartPoint::SData *>(m_sectionHeader + 1),
                parse<u16>(m_sectionHeader->count));
    } else {
        init(reinterpret_cast<const MapdataStartPoint::SData *>(
                     reinterpret_cast<const u8 *>(m_sectionHeader + 4)),
                1);
    }
}

/// @brief Default virtual destructor
MapdataStartPointAccessor::~MapdataStartPointAccessor() = default;

} // namespace Kinoko::System
