#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief Represents the sandcones that grow from the falling sand in the Dry Dry Ruins temple
/// @details The sandcones don't actually grow, rather their position gradually rises from beneath
/// the floor. The cone stops rising once it reaches the final size defined by param setting 2. The
/// growth speed is defined by param setting 1, and the start delay before growing is defined by
/// param setting 3.
class ObjectSandcone final : public ObjectKCL {
public:
    ObjectSandcone(const System::MapdataGeoObj &params);
    ~ObjectSandcone() override;

    /// @addr{0x806872A0}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_duration = m_finalHeightDelta / m_flowRate;
        m_currentMtx = m_rtMat;

        // Moved from getUpdatedMatrix to init b/c this only needs to be computed once per object.
        m_finalPos = pos() + EGG::Vector3f::ey * (static_cast<f32>(m_duration) * m_flowRate);
    }

    /// @addr{0x806873BC}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        setTransform(getUpdatedMatrix(0));
    }

    /// @addr{0x80687E14}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override;

    /// @addr{0x80687A2C}
    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80687CC0}
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

private:
    const f32 m_flowRate;         ///< Controls how fast the sandcone grows/rises
    const f32 m_finalHeightDelta; ///< The final height the sandcone will reach
    const u16 m_startFrame;       ///< Initial delay before the sandcone starts growing
    u16 m_duration;               ///< Total time it takes the sandcone to reach its final height
    EGG::Matrix34f m_rtMat;       ///< Initial rotation/translation matrix of the sandcone
    EGG::Matrix34f m_currentMtx;  ///< Up-to-date collision transformation matrix of the sandcone

    EGG::Vector3f m_finalPos; ///< Not in base game. Stores position of fully poured sandcone.
};

} // namespace Kinoko::Field
