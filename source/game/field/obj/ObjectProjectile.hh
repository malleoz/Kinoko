#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Abstract class that represents an object thrown by an @ref ObjectProjectileLauncher.
class ObjectProjectile : public ObjectCollidable {
public:
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @details Initializes the projectile's @ref m_idx based on param setting 1.
    ObjectProjectile(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_idx(params.setting(0)) {}

    /// @brief Default virtual destructor
    ~ObjectProjectile() override = default;

    /// @brief Init callback function called by the managing @ref ObjectSniper.
    virtual void initProjectile(const EGG::Vector3f &pos) = 0;

    /// @brief Callback function use by the @ref ObjectSniper that wants to throw this projectile.
    virtual void onLaunch() = 0;

    /// @beginGetters

    /// @brief Gets the unique index of this projectile within the @ref ObjectSniper's list of
    /// projectiles.
    /// @return The unique index of this projectile within the @ref ObjectSniper's list of
    /// projectiles.
    [[nodiscard]] s16 idx() const {
        return m_idx;
    }

    /// @endGetters

protected:
    const s16 m_idx; ///< Unique index in the @ref ObjectSniper's list of projectiles
};

} // namespace Kinoko::Field
