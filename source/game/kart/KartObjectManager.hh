#pragma once

#include <Types.hh>

namespace Kinoko {

namespace Abstract::g3d {

class ResAnmChr;

}

namespace Host {

class Context;

} // namespace Host

namespace Kart {

class KartObject;

/// @brief Manages the lifecycle and per-frame calculation of @ref KartObject objects
class KartObjectManager : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    void init();
    void calc();

    /// @beginGetters

    [[nodiscard]] KartObject *object(size_t i) const;

    [[nodiscard]] static const Abstract::g3d::ResAnmChr *ThunderScaleUpAnmChr();
    [[nodiscard]] static const Abstract::g3d::ResAnmChr *ThunderScaleDownAnmChr();
    [[nodiscard]] static const Abstract::g3d::ResAnmChr *PressScaleUpAnmChr();

    /// @endGetters

    static KartObjectManager *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static KartObjectManager *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    KartObjectManager();
    ~KartObjectManager() override;

    void loadScaleAnimations();

    fixed_vector<KartObject *> m_objects; ///< Array of pointers to each player's @ref KartObject

    /// @addr{0x809C18A0}
    /// @brief The scale animation when the shrunken kart returns to full size
    static Abstract::g3d::ResAnmChr *s_thunderScaleUpAnmChr;

    /// @addr{0x809C18A4}
    /// @brief The scale animation when the kart shrinks
    static Abstract::g3d::ResAnmChr *s_thunderScaleDownAnmChr;

    /// @addr{0x809C18B0}
    /// @brief The scale animation when the crushed kart returns to full size
    static Abstract::g3d::ResAnmChr *s_pressScaleUpAnmChr;

    /// @addr{0x809C18F8}
    /// @brief Pointer to the @ref KartObjectManager singleton
    static KartObjectManager *s_instance;
};

} // namespace Kart

} // namespace Kinoko
