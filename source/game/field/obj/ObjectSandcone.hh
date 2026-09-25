#pragma once

#include "game/field/obj/ObjectKCL.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief Represents the sandcones that grow from the falling sand in the Dry Dry Ruins temple
/// @details The sandcones don't actually grow, rather their position gradually rises from beneath
/// the floor. The cone stops rising once it reaches the final size defined by param setting 2. The
/// growth speed is defined by param setting 1, and the start delay before growing is defined by
/// param setting 3.
class ObjectSandcone final : public ObjectKCL {
public:
    /// @addr{0x80686F84}
    /// @copybrief ObjectKCL::ObjectKCL(const System::MapdataGeoObj &)
    /// @details Sets @ref m_flowRate based on param setting 1 divided by `100.0f`, @ref
    /// m_finalHeightDelta based on param setting 2, and @ref m_startFrame based on param setting 3.
    /// Caches the Pokey's initial position and rotation to @ref m_initMat. We take creative liberty
    /// in Kinoko to initialize @ref m_duration in the constructor rather than @ref init() so that
    /// it can be marked const. We also compute @ref m_finalPos to avoid repeatedly computing the
    /// sandcone's position after it has reached its final height.
    ObjectSandcone(const System::MapdataGeoObj &params)
        : ObjectKCL(params),
          m_flowRate(static_cast<f32>(params.setting(0)) / 100.0f),
          m_finalHeightDelta(static_cast<f32>(params.setting(1))),
          m_startFrame(params.setting(2)),
          m_duration(static_cast<u16>(m_finalHeightDelta / m_flowRate)),
          m_initMat(initMat()),
          m_finalPos(pos() + EGG::Vector3f::ey * (static_cast<f32>(m_duration) * m_flowRate)) {}

    /// @addr{0x806871E0}
    /// @brief Default virtual destructor
    ~ObjectSandcone() override = default;

    /// @addr{0x806872A0}
    /// @copybrief ObjectBase::init()
    /// @details Sets @ref m_currentMtx to @ref m_initMat. In the base game, this function also
    /// computes @ref m_duration, but since it is derived from two const members and it never
    /// changes once set, we take creative liberty in Kinoko to initialize this in the constructor
    /// so that it can be marked const.
    void init() override {
        m_currentMtx = m_initMat;
    }

    /// @addr{0x806873BC}
    /// @copybrief ObjectBase::calc()
    /// @details Sets the sandcone's transformation matrix based on @ref getUpdatedMatrix().
    void calc() override {
        setTransform(getUpdatedMatrix(0));
    }

    /// @addr{0x80687E14}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x80687800}
    /// @copybrief ObjectKCL::getUpdatedMatrix()
    /// @param timeOffset The time offset used to calculate the current frame's transformation
    /// @details Based off the current race timer, raises the sandcone's height gradually until it
    /// reaches the final height. The sandcone's height increases by @ref m_flowRate every frame
    /// until the sandcone has been flowing for @ref m_duration frames.
    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override {
        m_currentMtx = m_initMat;

        u32 t = System::RaceManager::Instance()->timer() - timeOffset;

        if (t > m_startFrame + m_duration) {
            // The sandcone has finished "flowing", so just return the final position.
            // For Kinoko, we introduce a slight performance improvement by caching the m_finalPos.
            m_currentMtx.setBase(3, m_finalPos);
        } else if (t > m_startFrame) {
            EGG::Vector3f deltaPos = EGG::Vector3f::ey * ((t - m_startFrame) * m_flowRate);
            m_currentMtx.setBase(3, m_initMat.base(3) + deltaPos);
        }

        return m_currentMtx;
    }

    /// @addr{0x80687A2C}
    /// @copydoc ObjectKCL::checkCollision()
    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x80687CC0}
    /// @copydoc ObjectKCL::checkCollisionCached()
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

private:
    /// @brief Computes the initial transformation matrix for the Pokey
    /// @return The initial transformation matrix for the Pokey
    [[nodiscard]] EGG::Matrix34f initMat() const {
        EGG::Matrix34f mat;
        mat.makeRT(rot(), pos());
        return mat;
    }

    const f32 m_flowRate;           ///< Controls how fast the sandcone grows/rises
    const f32 m_finalHeightDelta;   ///< The final height the sandcone will reach
    const u16 m_startFrame;         ///< Initial delay before the sandcone starts growing
    const u16 m_duration;           ///< Total time it takes the sandcone to reach its final height
    const EGG::Matrix34f m_initMat; ///< Initial rotation/translation matrix of the sandcone
    EGG::Matrix34f m_currentMtx;    ///< Up-to-date collision transformation matrix of the sandcone

    const EGG::Vector3f m_finalPos; ///< Not in base game. Stores position of fully poured sandcone.
};

} // namespace Kinoko::Field
