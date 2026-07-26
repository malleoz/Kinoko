#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief Represents the sandcones that grow from the falling sand in the Dry Dry Ruins temple
/// @details The sandcones don't actually grow, rather their position gradually rises from beneath
/// the floor. The cone stops rising once it reaches the final size defined by param setting 2. The
/// growth speed is defined by param setting 1, and the start delay before growing is defined by
/// param setting 3.
class ObjectSandcone : public ObjectKCL {
public:
    ObjectSandcone(const System::MapdataGeoObj &params);
    ~ObjectSandcone() override;

    void init() override;

    /// @addr{0x806873BC}
    void calc() override {
        setTransform(getUpdatedMatrix(0));
    }

    /// @addr{0x80687E14}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override;
    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;

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
