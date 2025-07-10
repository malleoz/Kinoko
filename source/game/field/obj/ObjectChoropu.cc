#include "ObjectChoropu.hh"

#include "game/field/ObjectDirector.hh"
#include "game/field/RailManager.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x806B96A0}
ObjectChoropu::ObjectChoropu(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), StateManager(this) {
    m_startFrameOffset = static_cast<s16>(params.setting(1));
    m_isStationary = strcmp(getName(), "choropu") != 0;

    s16 railIdx = params.pathId();
    if (railIdx != -1) {
        auto *rail = RailManager::Instance()->rail(railIdx);
        rail->checkSphereFull();
    }

    // If the mole moves around, then we need to create the dirt trail.
    if (!m_isStationary) {
        const auto &flowTable = ObjectDirector::Instance()->flowTable();
        const auto *collisionSet =
                flowTable.set(flowTable.slot(flowTable.getIdFromName("choropu_ground")));
        ASSERT(collisionSet);

        s16 height = parse<s16>(collisionSet->params.cylinder.height);
        size_t groundCount = static_cast<u32>(M_SPEED_RELATED / EGG::Mathf::abs(height * 2)) + 1;
        m_groundObjs.reserve(groundCount);

        for (size_t i = 0; i < groundCount; ++i) {
            auto *objGround = new ObjectChoropuGround(m_pos, m_rot, m_scale);
            m_groundObjs.push_back(objGround);
            objGround->load();
            objGround->resize(300.0f, 20.0f);
        }
    }

    m_objHoll = new ObjectChoropuHoll(params);
    m_objHoll->load();
}

/// @addr{0x806B9B8C}
ObjectChoropu::~ObjectChoropu() {
    for (auto *&obj : m_groundObjs) {
        delete obj;
    }

    delete m_objHoll;
}

/// @addr{0x806B9BF8}
void ObjectChoropu::init() {
    if (m_isStationary) {
        disableCollision();
        m_scale.x = 1.0f;
        m_objHoll->setScale(EGG::Vector3f(1.0f, m_objHoll->scale().y, 1.0f));
        m_nextStateId = 0;
        m_isColliding = false;

        calcTransform();
        m_transMat = m_transform;
    } else {
        if (m_mapObj->pathId() == -1) {
            return;
        }

        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setPerPointVelocities(true);

        EGG::Matrix34f mat = FUN_806B46A4(m_railInterpolator);
        m_pos = mat.base(3);
        m_flags |= 1;

        disableCollision();
        m_objHoll->disableCollision();
        m_nextStateId = 0;
        m_isColliding = false;
    }
}

/// @addr{0x806B9E60}
void ObjectChoropu::calc() {
    constexpr u32 START_DELAY = 300;

    // Nothing to do if the mole hasn't spawned yet
    u32 t = System::RaceManager::Instance()->timer();
    if (t < m_startFrameOffset + START_DELAY) {
        return;
    }

    if (m_isStationary) {
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
    } else {
        if (m_mapObj->pathId() == -1) {
            return;
        }

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

    m_objHoll->setScale(EGG::Vector3f(1.0f, m_objHoll->scale().y, 1.0f));
}

/// @addr{0x806BBDE4}
void ObjectChoropu::loadAnims() {
    std::array<const char *, 2> names = {{
            "start",
            "jump",
    }};

    std::array<Render::AnmType, 2> types = {{
            Render::AnmType::Chr,
            Render::AnmType::Chr,
    }};

    linkAnims(names, types);
}

/// @addr{0x806BA144}
Kart::Reaction ObjectChoropu::onCollision(Kart::KartObject * /*kartObj*/,
        Kart::Reaction reactionOnKart, Kart::Reaction reactionOnObj, EGG::Vector3f & /*hitDepth*/) {
    // Poking head out of the hole
    if (m_currentStateId == 1) {
        return Kart::Reaction::SmallBump;
    }

    if ((reactionOnObj == Kart::Reaction::UNK_3 || reactionOnObj == Kart::Reaction::UNK_5) &&
            !m_isColliding) {
        m_isColliding = true;

        if (m_currentStateId == 1) {
            m_drawMdl->anmMgr()->playAnim(0.0f, 1.0f, 1);
        }
    }

    return reactionOnKart;
}

void ObjectChoropu::enterStateStub() {}

/// @addr{0x806BA6D8}
void ObjectChoropu::enterState0() {
    if (m_isStationary) {
        disableCollision();
        m_isColliding = false;
    } else {
        for (auto *&obj : m_groundObjs) {
            obj->enableCollision();
        }

        disableCollision();
        m_objHoll->disableCollision();
        m_isColliding = false;
    }
}

/// @addr{0x806BABEC}
void ObjectChoropu::enterState1() {}

void ObjectChoropu::enterState3() {}

void ObjectChoropu::calcStateStub() {}

void ObjectChoropu::calcState0() {}

void ObjectChoropu::calcState1() {}

void ObjectChoropu::calcState3() {}

const std::array<StateManagerEntry<ObjectChoropu>, 5> StateManager<ObjectChoropu>::STATE_ENTRIES = {
        {
                {0, &ObjectChoropu::enterState0, &ObjectChoropu::calcState0},
                {1, &ObjectChoropu::enterState1, &ObjectChoropu::calcState1},
                {2, &ObjectChoropu::enterStateStub, &ObjectChoropu::calcStateStub},
                {3, &ObjectChoropu::enterState3, &ObjectChoropu::calcState3},
                {3, &ObjectChoropu::enterStateStub, &ObjectChoropu::calcStateStub},
        }};

StateManager<ObjectChoropu>::StateManager(ObjectChoropu *obj) {
    constexpr size_t ENTRY_COUNT = 5;

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

StateManager<ObjectChoropu>::~StateManager() {
    delete[] m_entryIds.data();
}

/// @addr{0x806B8F94}
ObjectChoropuGround::ObjectChoropuGround(const EGG::Vector3f &pos, const EGG::Vector3f &rot,
        const EGG::Vector3f &scale)
    : ObjectCollidable("choropu_ground", pos, rot, scale) {}

/// @addr{0x806BBE6C}
ObjectChoropuGround::~ObjectChoropuGround() = default;

/// @addr{0x806B93CC}
ObjectChoropuHoll::ObjectChoropuHoll(const System::MapdataGeoObj &params)
    : ObjectCollidable(params) {}

/// @addr{0x806BBE6C}
ObjectChoropuHoll::~ObjectChoropuHoll() = default;

} // namespace Field
