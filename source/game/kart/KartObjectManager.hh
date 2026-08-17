#pragma once

#include "game/kart/KartObject.hh"

#include <abstract/g3d/ResAnmChr.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Kart {

/// @brief Manages the lifecycle and per-frame calculation of @ref KartObject objects
class KartObjectManager : EGG::Disposer {
    friend class Host::Context;

public:
    void init();
    void calc();

    /// @beginGetters
    /// @addr{0x80590100}
    /// @brief Returns the @ref KartObject for the given player index
    /// @param i The player index
    /// @return A pointer to the @ref KartObject for the given player index
    [[nodiscard]] KartObject *object(size_t i) const {
        ASSERT(i < m_count);
        return m_objects[i];
    }

    [[nodiscard]] static const Abstract::g3d::ResAnmChr *ThunderScaleUpAnmChr() {
        return s_thunderScaleUpAnmChr;
    }

    [[nodiscard]] static const Abstract::g3d::ResAnmChr *ThunderScaleDownAnmChr() {
        return s_thunderScaleDownAnmChr;
    }

    [[nodiscard]] static const Abstract::g3d::ResAnmChr *PressScaleUpAnmChr() {
        return s_pressScaleUpAnmChr;
    }
    /// @endGetters

    /// @addr{0x8058FAA8}
    /// @brief Creates the singleton instance of the @ref KartObjectManager
    /// @return A pointer to the newly created singleton instance of the @ref KartObjectManager
    static KartObjectManager *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<KartObjectManager>();
        return s_instance;
    }

    /// @addr{0x8058FAF8}
    /// @brief Destroys the singleton instance of the @ref KartObjectManager
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @addr{0x809C18F8}
    /// @brief Returns the singleton instance of the @ref KartObjectManager
    /// @return The singleton instance of the @ref KartObjectManager
    [[nodiscard]] static KartObjectManager *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    KartObjectManager();
    ~KartObjectManager() override;

    void loadScaleAnimations();

    size_t m_count;
    KartObject **m_objects;

    static Abstract::g3d::ResAnmChr *s_thunderScaleUpAnmChr;   ///< @addr{0x809C18A0}
    static Abstract::g3d::ResAnmChr *s_thunderScaleDownAnmChr; ///< @addr{0x809C18A4}
    static Abstract::g3d::ResAnmChr *s_pressScaleUpAnmChr;     ///< @addr{0x809C18B0}
    static KartObjectManager *s_instance;                      ///< @addr{0x809C18F8}
};

} // namespace Kart

} // namespace Kinoko
