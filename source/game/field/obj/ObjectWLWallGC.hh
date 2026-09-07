#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief The horizontally moving piranha plant in GCN Waluigi Stadium
/// @details This object is separate and distinct from the pipe, which is a primitive @ref
/// ObjectKCL object. The piranha plant's motion is linear. The object's param settings define the
/// duration the piranha should remain inside the pipe, the duration is should remain extended
/// outside of the pipe, and the distance it should travel when extending.
/// @note If param setting 3 is zero, then the game implements a fail-safe where the piranha will
/// always remain retracted.
class ObjectWLWallGC final : public ObjectKCL {
public:
    ObjectWLWallGC(const System::MapdataGeoObj &params);
    ~ObjectWLWallGC() override;

    /// @addr{0x8086BE34}
    /// @copybrief ObjectBase::init()
    void init() override {
        setPos(m_initialPos);
        calcTransform();
        m_rtMat = transform();
    }

    /// @addr{0x8086C108}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        EGG::Vector3f prevPos = pos();
        setTransform(getUpdatedMatrix(0));
        setMovingObjVel(pos() - prevPos);
    }

    /// @addr{0x8086C640}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override;

    /// @addr{0x8086C648}
    /// @details Factors in the extended position of the piranha
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return (m_initialPos - m_extendedPos).length();
    }

    /// @addr{0x8086C328}
    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8086C5A8}
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

private:
    /// @addr{0x8086BF08}
    /// @brief Simple modulo to compute the current frame within the object's movement cycle
    [[nodiscard]] u32 cycleFrame(s32 t) const {
        u32 time = t < m_startFrame ? 0 : t - m_startFrame;
        return time % m_cycleDuration;
    }

    const s32 m_extendedDuration; ///< Duration the piranha should remain extended outside the pipe
    u32 m_moveDuration;           ///< Duration the piranha takes to move in and out of the pipe
    s32 m_startFrame;             ///< The frame at which the piranha plant's movement cycle starts
    s32 m_hiddenDuration;         ///< Duration the piranha should remain hidden inside the pipe
    s32 m_extendedFrame;          ///< Frame at which the piranha is fully extended outside the pipe
    s32 m_retractingFrame;        ///< Frame at which the piranha starts retracting into the pipe
    s32 m_cycleDuration;          ///< Total duration of the piranha plant's movement cycle
    const EGG::Vector3f m_initialPos; ///< Initial position of the piranha plant
    EGG::Vector3f m_extendedPos;      ///< Position of the piranha plant when it is fully extended
    EGG::Matrix34f m_rtMat;           ///< Current rotation/translation matrix
};

} // namespace Kinoko::Field
