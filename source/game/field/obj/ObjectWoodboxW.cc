#include "ObjectWoodboxW.hh"

#include "game/field/obj/ObjectWoodboxWSub.hh"

namespace Kinoko::Field {

/// @addr{0x8077DF24}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectWoodboxW::ObjectWoodboxW(const System::MapdataGeoObj &params)
    : ObjectCollidable(params),
      m_spawnInterval(params.setting(5)) {
    constexpr u16 DEFAULT_BOX_COUNT = 5;

    ObjectCollidable::init();

    u16 boxCount = params.setting(6);

    if (boxCount == 0) {
        boxCount = DEFAULT_BOX_COUNT;
    }

    m_boxes = owning_span<ObjectWoodboxWSub *>(boxCount);

    for (auto *&box : m_boxes) {
        box = EGG::egg_new<ObjectWoodboxWSub>(params);
        box->load();
    }
}

/// @addr{0x8077E120}
/// @brief Default virtual destructor
ObjectWoodboxW::~ObjectWoodboxW() = default;

/// @addr{0x8077E1A0}
/// @copybrief ObjectBase::init()
void ObjectWoodboxW::init() {
    ASSERT(m_mapObj);
    u32 startDelay = m_mapObj->setting(4);
    if (startDelay == 0) {
        startDelay = m_spawnInterval;
    }

    m_spawnTimer = startDelay;
    m_nextBoxIdx = 0;
}

/// @addr{0x8077E1E4}
/// @copybrief ObjectBase::calc()
void ObjectWoodboxW::calc() {
    if (--m_spawnTimer >= 1) {
        return;
    }

    m_spawnTimer = m_spawnInterval;
    m_boxes[m_nextBoxIdx]->enableCollision();
    m_nextBoxIdx = (m_nextBoxIdx + 1) % m_boxes.size();
}

} // namespace Kinoko::Field
