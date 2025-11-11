#include "game/field/obj/ObjectKoopaFigure64.hh"

#include "game/field/ObjectDirector.hh"
#include "game/field/ObjectFlowTable.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x806DA914}
ObjectKoopaFigure64::ObjectKoopaFigure64(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), m_isBigStatue(params.setting(1) == 1),
      m_startDelay(static_cast<u32>(params.setting(2))) {}

/// @addr{0x806DB114}
ObjectKoopaFigure64::~ObjectKoopaFigure64() = default;

/// @addr{0x806DAA44}
void ObjectKoopaFigure64::init() {
    if (m_isBigStatue) {
        const auto &flowTable = ObjectDirector::Instance()->flowTable();
        const auto *collisionSet = flowTable.set(flowTable.slot(id()));
        ASSERT(collisionSet);
        s16 radius = parse<s16>(collisionSet->params.cylinder.radius);
        resize(20.0f * static_cast<f32>(radius), 0.0f);
    }

    m_cycleFrame = FIRE_DURATION + COOLDOWN_DURATION;

    if (m_isBigStatue) {
        disableCollision();
    }
}

/// @addr{0x806DAB5C}
void ObjectKoopaFigure64::calc() {
    u32 timer = System::RaceManager::Instance()->timer();
    if (timer < m_startDelay) {
        return;
    }

    bool condition2 = CYCLE_DURATION - m_cycleFrame == 0;

    bool bVar3 = m_cycleFrame >= FIRE_DURATION && !condition2;

    if (condition2) {
        m_cycleFrame = 0;

        if (m_isBigStatue) {
            enableCollision();
        }
    }

    if (FIRE_DURATION <= m_cycleFrame && bVar3 && m_isBigStatue) {
        disableCollision();
    }

    if (m_cycleFrame < CYCLE_DURATION) {
        ++m_cycleFrame;
    }
}

/// @addr{0x806DAFB8}
void ObjectKoopaFigure64::calcCollisionTransform() {
    calcTransform();
    EGG::Matrix34f local_38 = m_transform;
    local_38.setBase(3, EGG::Vector3f::zero);

    EGG::Matrix34f mStack_68 = EGG::Matrix34f::ident;
    mStack_68.setBase(3, EGG::Vector3f(0.0f, -280.0f, 4000.0f));

    EGG::Matrix34f mStack_98 = EGG::Matrix34f::ident;
    mStack_98.setBase(3, m_pos);

    m_collision->transform(mStack_98.multiplyTo(local_38.multiplyTo(mStack_68)), m_scale);
}

} // namespace Field
