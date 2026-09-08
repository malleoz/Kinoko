#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief Represents an object that eventually shakes and then falls, e.g. the ultra rock on GV.
/// @details Supports up to three separate collision managers for different parts of the volcano
/// piece. The piece initially is at rest. After @ref m_restDuration frames, it will start to shake.
/// After an additional @ref m_shakeDuration elapse, the piece will enter the quake state, in which
/// the piece remains at rest temporarily. Finally, the piece will begin falling for @ref
/// FALL_DURATION frames.
class ObjectVolcanoPiece final : public ObjectKCL {
public:
    ObjectVolcanoPiece(const System::MapdataGeoObj &params);
    ~ObjectVolcanoPiece() override;

    void calc() override;

    /// @addr{0x80805974}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x80803F6C}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the volcano piece.
    [[nodiscard]] const char *getKclName() const override {
        return m_modelName;
    }

    /// @brief Function pointer type for a function that represents a partial point collision check
    typedef bool (ObjColMgr::*CheckPointPartialFunc)(const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut);

    /// @brief Function pointer type for a function that represents a full point collision check
    typedef bool (ObjColMgr::*CheckPointFullFunc)(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut);

    /// @brief Function pointer type for a function that represents a partial sphere collision check
    typedef bool (ObjColMgr::*CheckSpherePartialFunc)(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut);

    /// @brief Function pointer type for a function that represents a full sphere collision check
    typedef bool (ObjColMgr::*CheckSphereFullFunc)(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut);

    void createCollision() override;

    template <typename T, typename U>
        requires(std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>) &&
            (std::is_same_v<U, CheckPointPartialFunc> || std::is_same_v<U, CheckPointFullFunc>)
    [[nodiscard]] bool checkPointImpl(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, T *pInfo, KCLTypeMask *pFlagsOut, U checkFunc);

    template <typename T, typename U>
        requires(std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>) &&
            (std::is_same_v<U, CheckSpherePartialFunc> || std::is_same_v<U, CheckSphereFullFunc>)
    [[nodiscard]] bool checkSphereImpl(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, T *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset, U checkFunc);

    [[nodiscard]] bool checkCollisionImpl(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset, CheckSphereFullFunc checkFunc);

    /// @addr{0x80805404}
    /// @copydoc ObjectDrivable::checkPointPartial()
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkPointImpl(v0, v1, flags, pInfo, pFlagsOut, &ObjColMgr::checkPointPartial);
    }

    /// @addr{0x8080554C}
    /// @copydoc ObjectDrivable::checkPointPartialPush()
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkPointImpl(v0, v1, flags, pInfo, pFlagsOut, &ObjColMgr::checkPointPartialPush);
    }

    /// @addr{0x80805694}
    /// @copydoc ObjectDrivable::checkPointFull()
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkPointImpl(v0, v1, flags, pInfo, pFlagsOut, &ObjColMgr::checkPointFull);
    }

    /// @addr{0x808057DC}
    /// @copydoc ObjectDrivable::checkPointFullPush()
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkPointImpl(v0, v1, flags, pInfo, pFlagsOut, &ObjColMgr::checkPointFullPush);
    }

    /// @addr{0x80804F68}
    /// @copydoc ObjectDrivable::checkSpherePartial()
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset,
                &ObjColMgr::checkSpherePartial);
    }

    /// @addr{0x808050EC}
    /// @copydoc ObjectDrivable::checkSpherePartialPush()
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset,
                &ObjColMgr::checkSpherePartialPush);
    }

    /// @addr{0x80805270}
    /// @copydoc ObjectDrivable::checkSphereFull()
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut,
            u32 timeOffset) override {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset,
                &ObjColMgr::checkSphereFull);
    }

    /// @addr{0x808053F4}
    /// @copydoc ObjectDrivable::checkSphereFullPush()
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkCollision(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

    void narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
            u32 timeOffset) override;

    /// @addr{0x80804A48}
    /// @copydoc ObjectDrivable::checkPointCachedPartial()
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkPointImpl(v0, v1, flags, pInfo, pFlagsOut, &ObjColMgr::checkPointCachedPartial);
    }

    /// @addr{0x80804B90}
    /// @copydoc ObjectDrivable::checkPointCachedPartialPush()
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkPointImpl(v0, v1, flags, pInfo, pFlagsOut,
                &ObjColMgr::checkPointCachedPartialPush);
    }

    /// @addr{0x80804CD8}
    /// @copydoc ObjectDrivable::checkPointCachedFull()
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkPointImpl(v0, v1, flags, pInfo, pFlagsOut, &ObjColMgr::checkPointCachedFull);
    }

    /// @addr{0x80804E20}
    /// @copydoc ObjectDrivable::checkPointCachedFullPush()
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkPointImpl(v0, v1, flags, pInfo, pFlagsOut,
                &ObjColMgr::checkPointCachedFullPush);
    }

    /// @addr{0x808045AC}
    /// @copydoc ObjectDrivable::checkSphereCachedPartial()
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset,
                &ObjColMgr::checkSphereCachedPartial);
    }

    /// @addr{0x80804730}
    /// @copydoc ObjectDrivable::checkSphereCachedPartialPush()
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset,
                &ObjColMgr::checkSphereCachedPartialPush);
    }

    /// @addr{0x808048B4}
    /// @copydoc ObjectDrivable::checkSphereCachedFull()
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset,
                &ObjColMgr::checkSphereCachedFull);
    }

    /// @addr{0x80804A38}
    /// @copydoc ObjectDrivable::checkSphereCachedFullPush()
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkCollisionCached(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

    void update(u32 timeOffset) override;
    void calcScale(u32 timeOffset) override;

    /// @addr{0x80805924}
    /// @copydoc ObjectKCL::setMovingObjVel()
    void setMovingObjVel(const EGG::Vector3f &v) override {
        m_objColMgr->setMovingObjVel(v);

        if (m_colMgrB) {
            m_colMgrB->setMovingObjVel(v);
        }
    }

    /// @addr{0x8080595C}
    /// @copybrief ObjectKCL::getUpdatedMatrix()
    /// @param timeOffset The time offset used to calculate the current frame's transformation
    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override {
        return calcShakeAndFall(nullptr, timeOffset);
    }

    /// @addr{0x808199A8}
    /// @copydoc ObjectKCL::checkCollision()
    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkCollisionImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset,
                &ObjColMgr::checkSphereFullPush);
    }

    /// @addr{0x80819DA0}
    /// @copydoc ObjectKCL::checkCollisionCached()
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkCollisionImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset,
                &ObjColMgr::checkSphereCachedFullPush);
    }

private:
    /// @brief Represents the different motion states of the volcano piece
    enum class State {
        Rest = 0,
        Shake = 1,
        Quake = 2,
        Fall = 3,
        Gone = 4,
    };

    const EGG::Matrix34f &calcShakeAndFall(EGG::Vector3f *vel, u32 timeOffset);
    State calcState(u32 frame) const;
    f32 calcT(u32 frame) const;

    char m_modelName[16];             ///< Name of the KCL file for this volcano piece
    const EGG::Vector3f m_initialPos; ///< Initial position of the volcano piece
    const EGG::Vector3f m_initialRot; ///< Initial rotation of the volcano piece
    const u32 m_restDuration;         ///< Duration of the rest state in frames
    const u32 m_shakeDuration;        ///< Duration of the shake state in frames
    const u32 m_quakeDuration;        ///< Duration of the quake state in frames
    EGG::Matrix34f m_rtMat;           ///< Current frame's rotation/translation matrix
    ObjColMgr *m_colMgrB;             ///< Collision manager for the "B" part of the volcano piece
    ObjColMgr *m_colMgrC;             ///< Collision manager for the "C" part of the volcano piece

    /// @brief How long the piece will fall for before transitioning to the Gone state
    static constexpr u32 FALL_DURATION = 900;
};

} // namespace Kinoko::Field
