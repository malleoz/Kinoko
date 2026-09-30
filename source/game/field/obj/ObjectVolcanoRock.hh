#pragma once

#include "game/field/obj/ObjectKCL.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief The oscillating platforms before and after the indoor section on Grumble Volcano.
/// @details Uses a cosine wave to induce oscillatory motion along the z and y axes.
class ObjectVolcanoRock final : public ObjectKCL {
public:
    /// @addr{0x8081A198}
    /// @copybrief ObjectKCL::ObjectKCL(const System::MapdataGeoObj &)
    /// @details Caches the platform's initial position and rotation to @ref m_initialPos and @ref
    /// m_initialRot respectively. Sets @ref m_phaseShift based on param setting 4. Sets @ref
    /// m_zPeriod and @ref m_yPeriod based on param settings 2 and 5 respectively, with a minimum
    /// value of `2`. Sets @ref m_zAmplitude and @ref m_yAmplitude based on param setting 3 and 6
    /// respectively. Derives @ref m_zAngVel and @ref m_yAngVel based on the periods. Sets @ref
    /// m_variant based on whether or not param setting 1 is non-zero. Finally, initializes the
    /// platform's position based on @ref calcPos().
    ObjectVolcanoRock(const System::MapdataGeoObj &params)
        : ObjectKCL(params),
          m_initialPos(pos()),
          m_initialRot(rot()),
          m_phaseShift(static_cast<s16>(params.setting(3))),
          m_zPeriod(std::max<s16>(static_cast<s16>(params.setting(1)), 2)),
          m_yPeriod(std::max<s16>(static_cast<s16>(params.setting(4)), 2)),
          m_zAmplitude(static_cast<f32>(static_cast<s16>(params.setting(2)))),
          m_yAmplitude(static_cast<f32>(static_cast<s16>(params.setting(5)))),
          m_zAngVel(F_TAU / static_cast<f32>(m_zPeriod)),
          m_yAngVel(F_TAU / static_cast<f32>(m_yPeriod)),
          m_variant(!!params.setting(0)) {
        setPos(calcPos(0));
    }

    /// @addr{0x8081A690}
    /// @brief Default virtual destructor
    ~ObjectVolcanoRock() override = default;

    /// @addr{0x8081A370}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the position of the platform based on @ref calcPos() for the current frame.
    /// Sets moving object velocity based on the difference in position between the current frame
    /// and the last frame.
    void calc() override {
        EGG::Vector3f prevPos = pos();
        setPos(calcPos(System::RaceManager::Instance()->timer()));
        setMovingObjVel(pos() - prevPos);
    }

    /// @addr{0x8081A688}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that the object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8081A668}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the volcano rock (`VolcanoRock1` or `VolcanoRock2`).
    [[nodiscard]] const char *getKclName() const override {
        return m_variant ? "VolcanoRock2" : "VolcanoRock1";
    }

    /// @addr{0x8081A60C}
    /// @copybrief ObjectKCL::getUpdatedMatrix()
    /// @param timeOffset The time offset used to calculate the current frame's transformation
    /// @details The transformation matrix maintains a rotation of @ref m_initialRot and a
    /// translation based on @ref CalcPos() for the current frame.
    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override {
        u32 t = System::RaceManager::Instance()->timer() - timeOffset;
        m_rtMat.makeRT(m_initialRot, calcPos(t));
        return m_rtMat;
    }

    /// @addr{0x8081A5D0}
    /// @copybrief ObjectKCL::colRadiusAdditionalLength()
    /// @returns The additional length to be added to the collision radius of the volcano rock,
    /// `2000.0f + @ref m_zAmplitude`.
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return 2000.0f + m_zAmplitude;
    }

private:
    /// @addr{0x8081A414}
    /// @brief Calculates the position of the volcano rock at a given frame based on oscillatory
    /// motion along the z and y axes
    /// @param frame The current frame used to calculate the position of the volcano rock.
    /// @return The calculated world position of the volcano rock for the given frame.
    /// @details The Y and Z-axis displacement is calculated using a cosine function based on the
    /// current frame, the Z-axis @ref m_phaseShift, and the respective periods and amplitudes.
    /// Updates the platform's transformation matrix and returns the updated world position of the
    /// platform.
    EGG::Vector3f calcPos(u32 frame) {
        f32 tz = static_cast<f32>((frame + m_phaseShift) % m_zPeriod);
        f32 ty = static_cast<f32>(frame % m_yPeriod);

        auto zDisplacement = EGG::Vector3f::ez * EGG::Mathf::cos(m_zAngVel * tz) * m_zAmplitude;
        auto yDisplacement = EGG::Vector3f::ey * EGG::Mathf::cos(m_yAngVel * ty) * m_yAmplitude;

        calcTransform();

        return m_initialPos + transform().multVector33(zDisplacement + yDisplacement);
    }

    const EGG::Vector3f m_initialPos; ///< Initial position of the volcano rock
    const EGG::Vector3f m_initialRot; ///< Initial rotation of the volcano rock
    const s16 m_phaseShift;           ///< Framecount offset for Z-axis oscillation
    const s16 m_zPeriod;              ///< Framecount of the platform's movement period along z-axis
    const s16 m_yPeriod;              ///< Framecount of the platform's movement period along y-axis
    const f32 m_zAmplitude;           ///< Scalar applied to computed z-axis position
    const f32 m_yAmplitude;           ///< Scalar applied to computed y-axis position
    const f32 m_zAngVel;              ///< `2pi / @ref m_zPeriod`
    const f32 m_yAngVel;              ///< `2pi / @ref m_yPeriod`
    const bool m_variant;             ///< Differentiates which KCL model is used
    EGG::Matrix34f m_rtMat;           ///< Current frame's rotation and translation matrix
};

} // namespace Kinoko::Field
