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

        m_groundHeight = m_groundObjs[0]->height();
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
        m_164 = 0.0f;

        calcTransform();
        m_transMat = m_transform;
    } else {
        if (m_mapObj->pathId() == -1) {
            return;
        }

        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setPerPointVelocities(true);

        m_railMat = FUN_806B46A4(m_railInterpolator);
        m_pos = m_railMat.base(3);
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
        m_164 = 0.0f;
    }
}

/// @addr{0x806BABEC}
void ObjectChoropu::enterState1() {
    if (m_isStationary) {
        m_pos = m_transMat.base(3);
        m_flags |= 4;
        m_rot.z = 0.0f;

        enableCollision();
        m_isColliding = false;

        m_drawMdl->anmMgr()->playAnim(0.0f, 1.0f, 0);
    } else {
        m_pos = m_railMat.base(3);
        m_flags |= 4;
        m_rot.z = 0.0f;
        m_rot.y = 0.0f;

        enableCollision();

        const auto &curTanDir = m_railInterpolator->curTangentDir();
        s16 curPointIdx = m_railInterpolator->curPointIdx();
        EGG::Matrix34f mat;
        SetRotTangentHorizontal(mat, m_railInterpolator->floorNrm(curPointIdx), curTanDir);
        mat.setBase(3, m_railInterpolator->curPos());
        m_objHoll->setTransform(mat);
        m_objHoll->setPos(mat.base(3));
        m_objHoll->enableCollision();
        m_isColliding = false;

        m_drawMdl->anmMgr()->playAnim(0.0f, 1.0f, 0);
    }
}

/// @addr{0x806BB39C}
void ObjectChoropu::enterState3() {
    enableCollision();
    m_isColliding = false;
    m_drawMdl->anmMgr()->playAnim(0.0f, 1.0f, 1);

    if (m_isStationary) {
        m_pos = m_transMat.base(3);
        m_flags |= 1;
    } else {
        m_pos = m_railMat.base(3);
        m_flags |= 1;
    }

    m_flags |= 2;
    m_rot.z = 0.0f;
}

void ObjectChoropu::calcStateStub() {}

/// @addr{0x806BA7FC}
void ObjectChoropu::calcState0() {
    if (!m_isStationary) {
        m_pos = m_railInterpolator->curPos();
        m_flags |= 1;

        if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd) {
            if (m_railInterpolator->curPoint().setting[1] == 1) {
                m_nextStateId = 1;
            } else {
                f32 currVel = m_railInterpolator->getCurrVel();
                if (currVel < 0.1f) {
                    currVel = m_railInterpolator->currVel();
                }

                m_164 = m_railInterpolator->getCurrVel();
                if (m_164 > M_SPEED_RELATED) {
                    m_164 = M_SPEED_RELATED - 1.0f;
                }

                calcGround();
            }
        }
    }
}

void ObjectChoropu::calcState1() {}

void ObjectChoropu::calcState3() {}

/// @addr{0x806BB840}
void ObjectChoropu::calcGround() {
    size_t groundCount = static_cast<s32>(m_groundObjs.size());
    size_t idx = std::min(static_cast<size_t>(m_164 / m_groundHeight) + 1, groundCount);

    for (auto *&obj : m_groundObjs) {
        obj->enableCollision();
    }

    for (size_t i = idx; i < m_groundObjs.size(); ++i) {
        m_groundObjs[i]->disableCollision();
    }

    TODO;
}

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
    : ObjectCollidable("choropu_ground", pos, rot, scale) {
    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    const auto *collisionSet =
            flowTable.set(flowTable.slot(flowTable.getIdFromName("choropu_ground")));
    ASSERT(collisionSet);

    s16 height = parse<s16>(collisionSet->params.cylinder.height);
    m_height = 2.0f * EGG::Mathf::abs(static_cast<f32>(height));
}

/// @addr{0x806BBE6C}
ObjectChoropuGround::~ObjectChoropuGround() = default;

/// @addr{0x806B93CC}
ObjectChoropuHoll::ObjectChoropuHoll(const System::MapdataGeoObj &params)
    : ObjectCollidable(params) {}

/// @addr{0x806BBE6C}
ObjectChoropuHoll::~ObjectChoropuHoll() = default;

} // namespace Field
