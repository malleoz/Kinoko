#include "ObjectBasabasa.hh"

#include "game/field/RailManager.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @brief Scoped within the TU so that ObjectBasabasa can set and ObjectBasabasaDummy can access.
static f32 s_initialX;
static f32 s_initialY;

/// @addr{0x806B5C84}
ObjectBasabasaDummy::ObjectBasabasaDummy(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), StateManager(this) {
    m_active = true;
}

/// @addr{0x806B7630}
ObjectBasabasaDummy::~ObjectBasabasaDummy() = default;

/// @addr{0x806B5E80}
void ObjectBasabasaDummy::init() {
    m_railInterpolator->init(0.0f, 0);
    m_pos = m_railInterpolator->curPos();
    m_flags |= 1;

    auto &rng = System::RaceManager::Instance()->random();
    rng.next();
    f32 y = rng.getF32(s_initialY);
    f32 x = rng.getF32(s_initialX);

    m_initialPos = EGG::Vector3f(x - s_initialX * 0.5f, y, 0.0f);

    m_nextStateId = 0;
}

/// @addr{0x806B602C}
void ObjectBasabasaDummy::calc() {
    if (m_nextStateId >= 0) {
        m_currentStateId = m_nextStateId;
        m_nextStateId = -1;
        m_currentFrame = 0;

        auto enterFunc = m_entries[m_entryIds[m_currentStateId]].onEnter;
        (this->*enterFunc)();
    } else {
        ++m_currentFrame;
    }

    auto calcFunc = m_entries[m_entryIds[m_currentStateId]].onCalc;
    (this->*calcFunc)();
}

/// @addr{0x806B6874}
Kart::Reaction ObjectBasabasaDummy::onCollision(Kart::KartObject * /*kartObj*/,
        Kart::Reaction reactionOnKart, Kart::Reaction /*reactionOnObj*/,
        EGG::Vector3f & /*hitDepth*/) {
    return reactionOnKart;
}

/// @addr{0x806B5C80}
void ObjectBasabasaDummy::enterState0() {}

/// @addr{0x806B6288}
/// @brief This is run when a bat is hit with a start or other item. We can ignore for Kinoko.
void ObjectBasabasaDummy::enterState1() {}

/// @addr{0x806B6100}
void ObjectBasabasaDummy::calcState0() {
    if (!m_active) {
        return;
    }

    if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
        m_active = false;
        unregisterCollision();
    } else {
        setMatrixLookAt(m_railInterpolator->curTangentDir());
        calcTransform();

        EGG::Matrix34f mat = m_transform;
        mat.setBase(3, EGG::Vector3f::zero);
        m_pos = m_railInterpolator->curPos() + mat.ps_multVector(m_initialPos);
        m_flags |= 1;
    }
}

/// @addr{0x806B652C}
/// @brief This is run when a bat is hit with a start or other item. We can ignore for Kinoko.
void ObjectBasabasaDummy::calcState1() {}

const std::array<StateManagerEntry<ObjectBasabasaDummy>, 2>
        StateManager<ObjectBasabasaDummy>::STATE_ENTRIES = {{
                {0, &ObjectBasabasaDummy::enterState0, &ObjectBasabasaDummy::calcState0},
                {1, &ObjectBasabasaDummy::enterState1, &ObjectBasabasaDummy::calcState1},
        }};

StateManager<ObjectBasabasaDummy>::StateManager(ObjectBasabasaDummy *obj) {
    constexpr size_t ENTRY_COUNT = 2;

    m_obj = obj;
    m_entries = std::span{STATE_ENTRIES};
    m_entryIds = std::span(new u16[ENTRY_COUNT], ENTRY_COUNT);

    // The base game initializes all entries to 0xffff, possibly to avoid an uninitialized value
    for (auto &id : m_entryIds) {
        id = 0xffff;
    }

    for (size_t i = 0; i < m_entryIds.size(); ++i) {
        m_entryIds[STATE_ENTRIES[i].id] = i;
    }
}

StateManager<ObjectBasabasaDummy>::~StateManager() {
    delete[] m_entryIds.data();
}

/// @addr{0x806B70D0}
ObjectBasabasa::ObjectBasabasa(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
    m_initialTimer = params.setting(1);
    m_batsPerGroup = params.setting(2);
    m_startFrame = params.setting(6);

    f32 railLen = RailManager::Instance()->rail(params.pathId())->getPathLength();
    m_batCount = (static_cast<u32>((railLen / static_cast<f32>(params.setting(0))) /
                          static_cast<f32>(m_initialTimer)) +
                         1) *
            m_batsPerGroup;

    m_bats = std::span<ObjectBasabasaDummy *>(new ObjectBasabasaDummy *[m_batCount], m_batCount);

    for (auto *&bat : m_bats) {
        bat = new ObjectBasabasaDummy(params);
        bat->load();
    }

    s_initialX = params.setting(3);
    s_initialY = params.setting(4);

    m_batDuration = static_cast<f32>(params.setting(5)) / static_cast<f32>(params.setting(0)) /
            static_cast<f32>(m_batsPerGroup);
}

/// @addr{0x806B72F4}
ObjectBasabasa::~ObjectBasabasa() {
    for (auto *&bat : m_bats) {
        delete bat;
    }

    delete[] m_bats.data();
}

/// @addr{0x806B7334}
void ObjectBasabasa::init() {
    for (auto *&bat : m_bats) {
        if (bat->active()) {
            bat->setActive(false);
            bat->unregisterCollision();
        }
    }

    m_cycleTimer = m_initialTimer;
    m_batsActive = 0;
}

/// @addr{0x806B74C4}
void ObjectBasabasa::calc() {
    if (System::RaceManager::Instance()->timer() <= m_startFrame) {
        return;
    }

    if (m_cycleTimer == m_initialTimer + m_batDuration * (m_batsActive % m_batsPerGroup)) {
        auto *&bat = m_bats[m_batsActive++];
        bat->init();
        bat->setActive(true);
        bat->loadAABB(0.0f);

        if ((m_batsActive % m_batsPerGroup) == 0) {
            m_cycleTimer = m_batDuration * m_batsPerGroup;
        }
    }

    if (m_batsActive == m_batCount) {
        m_batsActive = 0;
    }

    ++m_cycleTimer;
}

} // namespace Field
