#pragma once

#include "game/field/obj/ObjectWoodboxWSub.hh"

namespace Kinoko::Field {

class ObjectWoodboxWSub;

/// @brief Represents a wooden box spawner for @ref ObjectWoodboxWSub boxes which follow a rail,
/// like on Toad's Factory
class ObjectWoodboxW final : public ObjectCollidable {
public:
    /// @addr{0x8077DF24}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Sets @ref m_spawnInterval based on param setting 6. Initializes the spawner.
    /// Creates and loads @ref ObjectWoodboxWSub objects based on the number specified by param
    /// setting 7, defaulted to `5` if the setting is zero.
    ObjectWoodboxW(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_spawnInterval(params.setting(5)) {
        constexpr u16 DEFAULT_BOX_COUNT = 5;

        ObjectCollidable::init();

        u16 boxCount = params.setting(6);

        if (boxCount == 0) {
            boxCount = DEFAULT_BOX_COUNT;
        }

        m_boxes = owning_span<ObjectWoodboxWSub *>(boxCount);

        for (auto *&box : m_boxes) {
            box = EGG::egg_new<ObjectWoodboxWSub>(params);
            box->load();
        }
    }

    /// @addr{0x8077E120}
    /// @brief Default virtual destructor
    ~ObjectWoodboxW() override = default;

    /// @addr{0x8077E1A0}
    /// @copybrief ObjectBase::init()
    /// @details Sets the initial @ref m_spawnTimer based on the start delay specified by param
    /// setting 5, with a default value of @ref m_spawnInterval if the setting is zero. Sets @ref
    /// m_nextBoxIdx to zero.
    void init() override {
        ASSERT(m_mapObj);
        u32 startDelay = m_mapObj->setting(4);
        if (startDelay == 0) {
            startDelay = m_spawnInterval;
        }

        m_spawnTimer = startDelay;
        m_nextBoxIdx = 0;
    }

    /// @addr{0x8077E1E4}
    /// @copybrief ObjectBase::calc()
    /// @details Decrements @ref m_spawnTimer and early returns if the timer is not zero. When the
    /// timer hits zero, a box will spawn at the start of the rail, @ref m_spawnTimer is reset to
    /// @ref m_spawnInterval, and @ref m_nextBoxIdx is incremented, wrapping around to 0 when it
    /// reaches the total number of boxes.
    void calc() override {
        if (--m_spawnTimer >= 1) {
            return;
        }

        m_spawnTimer = m_spawnInterval;
        m_boxes[m_nextBoxIdx]->enableCollision();
        m_nextBoxIdx = (m_nextBoxIdx + 1) % m_boxes.size();
    }

    /// @addr{0x8077ECDC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that the object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8077ECD0}
    /// @copybrief ObjectBase::createCollision()
    /// @details no-op because the spawner itself does not have any collision
    void createCollision() override {}

private:
    owning_span<ObjectWoodboxWSub *> m_boxes; ///< Pointers to the wooden boxes that spawn
    s32 m_spawnTimer;                         ///< Framecount until the next box spawns
    u32 m_nextBoxIdx;                         ///< Index of the next box to spawn
    const s32 m_spawnInterval;                ///< The fixed interval between spawns
};

} // namespace Kinoko::Field
