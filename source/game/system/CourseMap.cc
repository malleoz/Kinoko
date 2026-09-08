#include "CourseMap.hh"

namespace Kinoko::System {

/// @addr{0x805127EC}
/// @brief Initializes the course map by loading the course KMP file and parsing its sections
/// @details Also computes the starting position spacing.
void CourseMap::init() {
    std::span<const u8> buffer = LoadFile("course.kmp");
    m_header = EGG::egg_new<MapdataFileAccessor>(
            reinterpret_cast<const MapdataFileAccessor::SData *>(buffer.data()));

    constexpr u32 AREA_SIGNATURE = Signature("AREA");
    constexpr u32 CANNON_POINT_SIGNATURE = Signature("CNPT");
    constexpr u32 CHECK_PATH_SIGNATURE = Signature("CKPH");
    constexpr u32 CHECK_POINT_SIGNATURE = Signature("CKPT");
    constexpr u32 GEO_OBJ_SIGNATURE = Signature("GOBJ");
    constexpr u32 JUGEM_POINT_SIGNATURE = Signature("JGPT");
    constexpr u32 START_POINT_SIGNATURE = Signature("KTPT");
    constexpr u32 POINT_INFO_SIGNATURE = Signature("POTI");
    constexpr u32 STAGE_INFO_SIGNATURE = Signature("STGI");

    m_startPoint = parseMapdata<MapdataStartPointAccessor>(START_POINT_SIGNATURE);
    m_checkPath = parseMapdata<MapdataCheckPathAccessor>(CHECK_PATH_SIGNATURE);
    m_checkPoint = parseMapdata<MapdataCheckPointAccessor>(CHECK_POINT_SIGNATURE);
    m_geoObj = parseMapdata<MapdataGeoObjAccessor>(GEO_OBJ_SIGNATURE);
    m_pointInfo = parseMapdata<MapdataPointInfoAccessor>(POINT_INFO_SIGNATURE);
    m_area = parseMapdata<MapdataAreaAccessor>(AREA_SIGNATURE);
    m_jugemPoint = parseMapdata<MapdataJugemPointAccessor>(JUGEM_POINT_SIGNATURE);
    m_cannonPoint = parseMapdata<MapdataCannonPointAccessor>(CANNON_POINT_SIGNATURE);
    m_stageInfo = parseMapdata<MapdataStageInfoAccessor>(STAGE_INFO_SIGNATURE);

    ASSERT(m_area);
    m_area->sort();

    MapdataStageInfo *stageInfo = getStageInfo();
    constexpr u8 TRANSLATION_MODE_NARROW = 1;
    if (stageInfo && stageInfo->translationMode() == TRANSLATION_MODE_NARROW) {
        m_skewAngle = 25.0f;
        m_longitudinalOffset = 250.0f;
        m_longitudinalWideOffset = 0.0f;
    } else {
        m_skewAngle = 30.0f;
        m_longitudinalOffset = 400.0f;
        m_longitudinalWideOffset = 100.0f;
    }

    m_lateralSpacing = 800.0f;
    m_longitudinalSpacing = 1200.0f;
}

/// @addr{0x80511500}
/// @brief Finds the sector (checkpoint) that contains the given position
/// @param pos The position to check
/// @param checkpointIdx The index of the checkpoint to start the search from
/// @param distanceRatio Output parameter for the distance ratio within the sector
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::findSector(const EGG::Vector3f &pos, u16 checkpointIdx, f32 &distanceRatio) {
    clearSectorChecked();

    MapdataCheckPoint *checkpoint = getCheckPoint(checkpointIdx);
    s16 id = -1;

    MapdataCheckPoint::SectorOccupancy occupancy =
            checkpoint->checkSectorAndDistanceRatio(pos, distanceRatio);
    checkpoint->setSearched();

    switch (occupancy) {
    // The player is fully inside the current checkpoint, so just set to current checkpoint
    case MapdataCheckPoint::SectorOccupancy::InsideSector:
        id = checkpoint->id();
        break;

    // The player is between the sides of the quad, but NOT between this checkpoint and
    // next; player is likely in the same checkpoint group
    case MapdataCheckPoint::SectorOccupancy::BetweenSides:
        id = findSectorBetweenSides(pos, checkpoint, distanceRatio);
        break;

    // The player is not between the sides of the quad (may still be between this checkpoint
    // and next); player is likely in a different checkpoint group
    case MapdataCheckPoint::SectorOccupancy::OutsideSector:
        id = findSectorOutsideSector(pos, checkpoint, distanceRatio);
        break;

    default:
        break;
    }

    return id > -1 ? id : findSectorRegional(pos, checkpoint, distanceRatio);
}

/// @addr{0x80511110}
/// @brief Recursively depth-first searches for the sector (checkpoint) that contains the position
/// @param pos The position to check
/// @param depth The current depth of the recursive search
/// @param searchBackwardsFirst Whether to search backwards first
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the distance ratio within the sector
/// @param playerIsForwards Whether the player is moving forwards
/// @return The ID of the sector containing the position, or -1 if not found
/// @details This function performs a depth-first search through the checkpoint graph, starting from
/// the given checkpoint, to find the sector that contains the specified position. It considers the
/// player's movement direction and the search order (forwards or backwards) to determine the most
/// likely sector. The search is limited by a maximum depth to prevent infinite recursion.
s16 CourseMap::findRecursiveSector(const EGG::Vector3f &pos, s16 depth, bool searchBackwardsFirst,
        MapdataCheckPoint *checkpoint, f32 &distanceRatio, bool playerIsForwards) const {
    constexpr s16 MAX_DEPTH = 6;

    if (depth >= 0 && depth > MAX_DEPTH) {
        return -1;
    }

    MapdataCheckPoint::SectorOccupancy completion =
            MapdataCheckPoint::SectorOccupancy::OutsideSector;

    if (!checkpoint->searched()) {
        completion = checkpoint->checkSectorAndDistanceRatio(pos, distanceRatio);
        checkpoint->setSearched();
    }

    // If player is inside current checkpoint, stop searching
    if (completion == MapdataCheckPoint::SectorOccupancy::InsideSector) {
        return checkpoint->id();
    }

    // Search type 0: Search forwards first, then backwards
    if (!searchBackwardsFirst) {
        // If "player is forwards" but completion < 0, force completion to 0 and return
        // current checkpoint (GHOST CHECKPOINT!)
        if (playerIsForwards && completion == MapdataCheckPoint::SectorOccupancy::BetweenSides &&
                distanceRatio < 0.0f) {
            distanceRatio = 0.0f;
            return checkpoint->id();
        }

        // Stop if current checkpoint is a KCP
        if (checkpoint->type() >= 0) {
            return -1;
        }

        // If player is between the sides of the quad but NOT between this checkpoint and next, AND
        // completion > 0, then "player is forwards"
        bool forward = completion == MapdataCheckPoint::SectorOccupancy::BetweenSides &&
                distanceRatio > 0.0f;

        // Search forwards, including checkpoints already searched
        s16 id = searchNextCheckpoint(pos, depth, checkpoint, distanceRatio, forward, false);

        // If that fails, search backwards, excluding checkpoints already searched
        return id == -1 ?
                searchPrevCheckpoint(pos, depth, checkpoint, distanceRatio, forward, true) :
                id;
    }

    // Search type 1: Search backwards first, then forwards

    // If "player is backwards" flag is true but completion > 1, force completion to 1 and return
    // current checkpoint (GHOST CHECKPOINT!)
    if (playerIsForwards && completion == MapdataCheckPoint::SectorOccupancy::BetweenSides &&
            distanceRatio > 1.0f) {
        distanceRatio = 1.0f;
        return checkpoint->id();
    }

    // Stop if current checkpoint is a KCP (skipped for online players, but they aren't supported)
    if (checkpoint->type() >= 0) {
        return -1;
    }

    // If player is between the sides of the quad but NOT between this checkpoint and next, AND
    // completion < 0, then set "player is backwards" flag
    bool forward =
            completion == MapdataCheckPoint::SectorOccupancy::BetweenSides && distanceRatio < 0.0f;

    // Search backwards, including checkpoints already searched
    s16 id = searchPrevCheckpoint(pos, depth, checkpoint, distanceRatio, forward, false);

    // If that fails, search forwards, excluding checkpoints already searched
    return id == -1 ? searchNextCheckpoint(pos, depth, checkpoint, distanceRatio, forward, true) :
                      id;
}

/// @addr{0x80516808}
/// @brief Gets the ID of the current area that contains the given position, if any
/// @param i The index of the area to check first
/// @param pos The position to check
/// @param type The type of area to search for
/// @return The ID of the area containing the position, or -1 if not found
s16 CourseMap::getCurrentAreaID(s16 i, const EGG::Vector3f &pos, MapdataAreaBase::Type type) const {
    // Check if we're colliding with the provided area ID
    if (i >= 0) {
        const MapdataAreaBase *area = getArea(i);
        if (area->type() == type && area->test(pos)) {
            return i;
        }
    }

    // Search all areas of the same type
    for (i = 0; i < getAreaCount(); ++i) {
        const MapdataAreaBase *area = getAreaSorted(i);
        if (area->type() == type && area->test(pos)) {
            return area->index();
        }
    }

    return -1;
}

/// @addr{0x8051276C}
/// @brief Private constructor
CourseMap::CourseMap()
    : m_header(nullptr),
      m_startPoint(nullptr),
      m_stageInfo(nullptr),
      m_skewAngle(0.0f),
      m_lateralSpacing(0.0f),
      m_longitudinalSpacing(0.0f),
      m_longitudinalOffset(0.0f),
      m_longitudinalWideOffset(0.0f) {}

/// @addr{0x805127AC}
/// @brief Private destructor that destroys the accessors to the `course.kmp` sections
CourseMap::~CourseMap() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("CourseMap instance not explicitly handled!");
    }

    EGG::egg_delete(m_header);
    EGG::egg_delete(m_startPoint);
    EGG::egg_delete(m_checkPath);
    EGG::egg_delete(m_checkPoint);
    EGG::egg_delete(m_pointInfo);
    EGG::egg_delete(m_geoObj);
    EGG::egg_delete(m_area);
    EGG::egg_delete(m_jugemPoint);
    EGG::egg_delete(m_cannonPoint);
    EGG::egg_delete(m_stageInfo);
}

/// @brief Searches for the sector containing the given position when the player is @ref
/// MapdataCheckPoint::SectorOccupancy::BetweenSides of the current checkpoint's quad
/// @param pos The position to check
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::findSectorBetweenSides(const EGG::Vector3f &pos, MapdataCheckPoint *checkpoint,
        f32 &distanceRatio) {
    s16 id = -1;

    // Search order varies depending on whether player is closer to the next or previous checkpoint.
    if (distanceRatio > 0.5f) {
        // Step 1: Starting at current checkpoint, search forwards
        id = searchNextCheckpoint(pos, 0, checkpoint, distanceRatio, false, false);

        if (id != -1) {
            return id;
        }

        // Step 2: If step 1 fails, start at next checkpoint(s) and search backwards
        for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
            MapdataCheckPoint *next = checkpoint->nextPoint(i);

            for (size_t j = 0; j < next->prevCount(); ++j) {
                MapdataCheckPoint *prev = next->prevPoint(j);

                if (prev == checkpoint) {
                    continue;
                }

                id = findRecursiveSector(pos, 1, true, prev, distanceRatio, false);
                if (id != -1) {
                    return id;
                }
            }
        }

        // Step 3: If step 2 fails, start at previous checkpoint(s) and search forwards
        for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
            MapdataCheckPoint *prev = checkpoint->prevPoint(i);

            for (size_t j = 0; j < prev->nextCount(); ++j) {
                MapdataCheckPoint *next = prev->nextPoint(j);

                if (next == checkpoint) {
                    continue;
                }

                id = findRecursiveSector(pos, 1, false, next, distanceRatio, false);
                if (id != -1) {
                    return id;
                }
            }
        }

        // Step 4: If step 3 fails, start at current checkpoint and search backwards
        return searchPrevCheckpoint(pos, 0, checkpoint, distanceRatio, false, false);
    } else {
        // Step 1: Starting at current checkpoint, search backwards
        id = searchPrevCheckpoint(pos, 0, checkpoint, distanceRatio, false, false);

        if (id != -1) {
            return id;
        }

        // Step 2: If step 1 fails, start at prev checkpoint(s) and search forwards
        for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
            MapdataCheckPoint *prev = checkpoint->prevPoint(i);

            for (size_t j = 0; j < prev->nextCount(); ++j) {
                MapdataCheckPoint *next = prev->nextPoint(j);

                if (next == checkpoint) {
                    continue;
                }

                id = findRecursiveSector(pos, 1, false, next, distanceRatio, false);

                if (id != -1) {
                    return id;
                }
            }
        }

        // Step 3: If step 2 fails, start at next checkpoint(s) and search backwards
        for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
            MapdataCheckPoint *next = checkpoint->nextPoint(i);

            for (size_t j = 0; j < next->prevCount(); ++j) {
                MapdataCheckPoint *prev = next->prevPoint(j);

                if (prev == checkpoint) {
                    continue;
                }

                id = findRecursiveSector(pos, 1, true, prev, distanceRatio, false);

                if (id != -1) {
                    return id;
                }
            }
        }

        // Step 4: If step 3 fails, start at current checkpoint and search forwards
        return searchNextCheckpoint(pos, 0, checkpoint, distanceRatio, false, false);
    }

    return id;
}

/// @brief Searches for the sector containing the given position when the player is @ref
/// MapdataCheckPoint::SectorOccupancy::OutsideSector the current checkpoint's quad
/// @param pos The position to check
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::findSectorOutsideSector(const EGG::Vector3f &pos, MapdataCheckPoint *checkpoint,
        f32 &distanceRatio) {
    s16 id = -1;

    // Step 1: Starting at next checkpoint(s), search backwards
    for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
        MapdataCheckPoint *next = checkpoint->nextPoint(i);

        for (size_t j = 0; j < next->prevCount(); ++j) {
            MapdataCheckPoint *prev = next->prevPoint(j);

            if (prev == checkpoint) {
                continue;
            }

            id = findRecursiveSector(pos, 1, true, prev, distanceRatio, false);

            if (id != -1) {
                return id;
            }
        }
    }

    // Step 2: If step 1 fails, start at prev checkpoint(s) and search forwards
    for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
        MapdataCheckPoint *prev = checkpoint->prevPoint(i);

        for (size_t j = 0; j < prev->nextCount(); ++j) {
            MapdataCheckPoint *next = prev->nextPoint(j);

            if (next == checkpoint) {
                continue;
            }

            id = findRecursiveSector(pos, 1, false, next, distanceRatio, false);

            if (id != -1) {
                return id;
            }
        }
    }

    // Step 3: If step 2 fails, start at next checkpoint(s) and search forwards
    for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
        id = findRecursiveSector(pos, 1, false, checkpoint->nextPoint(i), distanceRatio, false);

        if (id != -1) {
            return id;
        }
    }

    // Step 4: If step 3 fails, start at prev checkpoint(s) and search backwards
    for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
        id = findRecursiveSector(pos, 1, true, checkpoint->prevPoint(i), distanceRatio, false);

        if (id != -1) {
            return id;
        }
    }

    return id;
}

/// @brief Last resort fallback which removes the recursion limit and searches all checkpoints
/// @param pos The position to check
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::findSectorRegional(const EGG::Vector3f &pos, MapdataCheckPoint *checkpoint,
        f32 &distanceRatio) {
    s16 id = -1;

    // Step 1: Search all next checkpoints until player or key checkpoint is found
    for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
        id = findRecursiveSector(pos, -1, false, checkpoint->nextPoint(i), distanceRatio, false);

        if (id != -1) {
            return id;
        }
    }

    // Step 2: Search all previous checkpoints until player or key checkpoint is found
    for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
        id = findRecursiveSector(pos, -1, true, checkpoint->prevPoint(i), distanceRatio, false);

        if (id != -1) {
            return id;
        }
    }

    return id;
}

/// @addr{0x80510F58}
/// @brief Recurses through the next checkpoints to find the sector containing the given position
/// @param pos The position to check
/// @param depth The current depth of the recursive search
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @param playerIsForwards Whether the player is moving forwards
/// @param useCache Whether to use the cached search results
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::searchNextCheckpoint(const EGG::Vector3f &pos, s16 depth,
        const MapdataCheckPoint *checkpoint, f32 &distanceRatio, bool playerIsForwards,
        bool useCache) const {
    s16 id = -1;
    depth = depth >= 0 ? depth + 1 : -1;

    for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
        MapdataCheckPoint *next = checkpoint->nextPoint(i);

        if (!useCache || !next->searched()) {
            id = findRecursiveSector(pos, depth, false, next, distanceRatio, playerIsForwards);

            if (id != -1) {
                return id;
            }
        }
    }

    return id;
}

/// @addr{0x80511034}
/// @brief Recurses through the previous checkpoints to find the sector containing the given
/// position
/// @param pos The position to check
/// @param depth The current depth of the recursive search
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @param playerIsForwards Whether the player is moving forwards
/// @param useCache Whether to use the cached search results
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::searchPrevCheckpoint(const EGG::Vector3f &pos, s16 depth,
        const MapdataCheckPoint *checkpoint, f32 &distanceRatio, bool playerIsForwards,
        bool useCache) const {
    s16 id = -1;
    depth = depth >= 0 ? depth + 1 : -1;

    for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
        MapdataCheckPoint *prev = checkpoint->prevPoint(i);

        if (!useCache || !prev->searched()) {
            id = findRecursiveSector(pos, depth, true, prev, distanceRatio, playerIsForwards);

            if (id != -1) {
                return id;
            }
        }
    }

    return id;
}

CourseMap *CourseMap::s_instance = nullptr;

} // namespace Kinoko::System
