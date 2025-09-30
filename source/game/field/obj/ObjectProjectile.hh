#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

/// @brief Abstract class that represents an object that is "thrown" by an @ref
/// ObjectProjectileLauncher.
class ObjectProjectile : public ObjectCollidable {
public:
    ObjectProjectile(const System::MapdataGeoObj &params)
        : ObjectCollidable(params), m_idx(params.setting(0)) {}

    ~ObjectProjectile() override = default;

    virtual void initFromSniper(const EGG::Vector3f &pos) = 0;
    virtual void onLaunch() = 0;

    [[nodiscard]] s16 idx() const {
        return m_idx;
    }

protected:
    const s16 m_idx;
};

} // namespace Field
