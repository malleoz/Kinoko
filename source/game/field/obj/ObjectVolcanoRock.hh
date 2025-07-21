#pragma once

#include "game/field/obj/ObjectKCL.hh"

#include "game/system/RaceManager.hh"

namespace Field {

class ObjectVolcanoRock : public ObjectKCL {
public:
    ObjectVolcanoRock(const System::MapdataGeoObj &params);
    ~ObjectVolcanoRock() override;

    void calc() override;

    /// @addr{0x8081A688}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x8081A668}
    [[nodiscard]] const char *getKclName() const override {
        return m_variant ? "VolcanoRock2" : "VolcanoRock1";
    }

    /// @addr{0x8081A60C}
    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override {
        u32 t = System::RaceManager::Instance()->timer() - timeOffset;
        m_rtMat.makeRT(m_initialRot, calcPos(t));
        return m_rtMat;
    }

    /// @addr{0x8081A5D0}
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return 2000.0f + m_zAmplitude;
    }

private:
    EGG::Vector3f calcPos(u32 frame);

    EGG::Vector3f m_initialPos; ///< Position when initialized
    EGG::Vector3f m_initialRot; ///< Rotation when initialized
    s16 m_phaseShift;           ///< Additional framecount applied when calculating z-axis position
    s16 m_zPeriod;              ///< Framecount of the platform's movement period along z-axis
    s16 m_yPeriod;              ///< Framecount of the platform's movement period along y-axis
    s16 m_zAmplitude;           ///< Scalar applied to computed z-axis position
    s16 m_yAmplitude;           ///< Scalar applied to computed y-axis position
    f32 m_zAngVel;              ///< 2pi / m_zPeriod
    f32 m_yAngVel;              ///< 2pi / m_yPeriod
    bool m_variant;             ///< Differentiates which KCL is used
    EGG::Matrix34f m_rtMat;     ///< Rotation and translation matrix
};

} // namespace Field
