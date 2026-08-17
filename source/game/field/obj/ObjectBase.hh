#pragma once

#include "game/field/BoxColManager.hh"
#include "game/field/RailInterpolator.hh"
#include "game/field/obj/ObjectId.hh"

#include "game/render/DrawMdl.hh"

#include "game/system/map/MapdataGeoObj.hh"

#include <egg/core/BitFlag.hh>
#include <egg/math/Matrix.hh>

namespace Kinoko::Field {

/// @brief %Abstract base class for all objects in the game
/// @details Objects maintain position, rotation, and scale vectors. Position and scale are combined
/// into a 3x4 transformation matrix via calcTransform(). Each object manages its own BoxColUnit (if
/// it has collision) and optional rail interpolator. The class provides lifecycle hooks
/// (init/calc/load/createCollision) implemented by subclasses and virtual methods for
/// collision/resource management.
class ObjectBase {
public:
    /// @brief Tracks what properties have changed and require recalculation in the transform matrix
    enum class eFlags {
        Position = 0,
        Rotation = 1,
        Matrix = 2,
        Scale = 3,
    };

    /// @brief A bitfield of @ref eFlags that represents the state of the object
    typedef EGG::TBitFlag<u16, eFlags> Flags;

    ObjectBase(const System::MapdataGeoObj &params);
    ObjectBase(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    virtual ~ObjectBase();

    /// @brief Run once during race initialization
    virtual void init() {}

    /// @brief Runs once per frame if the calc flag is set in loadFlags()
    virtual void calc() {}

    /// @addr{0x808217B8}
    virtual void calcModel() {
        calcTransform();
    }

    /// @brief Loads resources, collision, and registers the object to the ObjectDirector
    virtual void load() = 0;

    [[nodiscard]] virtual const char *getResources() const;
    virtual void loadGraphics();

    /// @brief Links associated animations from the object's resource file to the DrawMdl, if any
    virtual void loadAnims() {}

    /// @brief Creates a collision object that inherits from ObjectCollisionBase
    virtual void createCollision() = 0;

    virtual void loadRail();

    /// @brief Updates the position, rotation, and scale of the collision object
    virtual void calcCollisionTransform() = 0;

    [[nodiscard]] virtual const char *getName() const;

    /// @brief Returns a bitmask indicating which lifecycle hooks should be called for the object
    /// @addr{0x806BF434}
    /// @details In Kinoko, the only effective behavior of this bitmask is to determine whether
    /// calc() should be called. The other bits are unused.
    [[nodiscard]] virtual u32 loadFlags() const {
        // TODO: This references LOD to determine load flags
        return 0;
    }

    [[nodiscard]] virtual const char *getKclName() const;

    /// @brief Resizes the BoxColUnit associated with the object
    /// @addr{0x80821DB8}
    /// @details This does not affect the actual collision geometry of the object, only the
    /// BoxColUnit used for sptially indexed collision queries. This effectively adjusts how close
    /// the player needs to be to the object before collision checks are performed.
    /// @param radius The new radius of the collision box
    /// @param maxSpeed The maximum speed for the collision box
    virtual void resize(f32 radius, f32 maxSpeed) {
        m_boxColUnit->resize(radius, maxSpeed);
    }

    /// @brief Unregisters the BoxColUnit associated with the object from the BoxColManager
    /// @addr{0x80821DD8}
    virtual void unregisterCollision() {
        BoxColManager::Instance()->remove(m_boxColUnit);
    }

    /// @brief Skips collision checks for this object in the BoxColManager
    /// @addr{0x80821DEC}
    virtual void disableCollision() const {
        m_boxColUnit->m_flag.setBit(eBoxColFlag::Intangible);
    }

    /// @brief Re-enables collision checks for this object in the BoxColManager
    /// @addr{0x80821E00}
    virtual void enableCollision() const {
        m_boxColUnit->m_flag.resetBit(eBoxColFlag::Intangible);
    }

    /// @addr{0x80680618}
    [[nodiscard]] virtual const BoxColUnit *getUnit() const {
        return m_boxColUnit;
    }

    /// @brief The position of the object in world space
    /// @addr{0x80681598}
    [[nodiscard]] virtual const EGG::Vector3f &getPosition() const {
        return m_pos;
    }

    /// @brief Collision radius to use for GJK checks
    /// @addr{0x8080BDC0}
    [[nodiscard]] virtual f32 getCollisionRadius() const {
        return 100.0f;
    }

    /// @addr{0x80572574}
    [[nodiscard]] virtual ObjectId id() const {
        return m_id;
    }

    /// @beginSetters
    void setPos(const EGG::Vector3f &pos) {
        m_flags.setBit(eFlags::Position);
        m_pos = pos;
    }

    void addPos(const EGG::Vector3f &v) {
        m_flags.setBit(eFlags::Position);
        m_pos += v;
    }

    void subPos(const EGG::Vector3f &v) {
        m_flags.setBit(eFlags::Position);
        m_pos -= v;
    }

    void setScale(const EGG::Vector3f &scale) {
        m_flags.setBit(eFlags::Scale);
        m_scale = scale;
    }

    void setScale(f32 scale) {
        m_flags.setBit(eFlags::Scale);
        m_scale.set(scale);
    }

    void setRot(const EGG::Vector3f &rot) {
        m_flags.setBit(eFlags::Rotation);
        m_rot = rot;
    }

    void setRotNoFlag(const EGG::Vector3f &rot) {
        m_rot = rot;
    }

    void addRot(const EGG::Vector3f &v) {
        m_rotLock = true;
        m_flags.setBit(eFlags::Rotation);
        m_rot += v;
    }

    void subRot(const EGG::Vector3f &v) {
        m_rotLock = true;
        m_flags.setBit(eFlags::Rotation);
        m_rot -= v;
    }

    /// @addr{0x806C296C}
    void setTransform(const EGG::Matrix34f &mat) {
        m_rotLock = false;
        m_flags.setBit(eFlags::Matrix);
        m_transform = mat;
        m_pos = mat.base(3);
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] const RailInterpolator *railInterpolator() const {
        return m_railInterpolator;
    }

    [[nodiscard]] const EGG::Vector3f &pos() const {
        return m_pos;
    }

    [[nodiscard]] const EGG::Vector3f &scale() const {
        return m_scale;
    }

    [[nodiscard]] const EGG::Vector3f &rot() const {
        return m_rot;
    }

    [[nodiscard]] const EGG::Matrix34f &transform() const {
        return m_transform;
    }
    /// @endGetters

protected:
    void calcTransform();
    void linkAnims(const std::span<const char *> &names, const std::span<Render::AnmType> types);
    void calcRotLock();
    void setMatrixTangentTo(const EGG::Vector3f &up, const EGG::Vector3f &tangent);
    void setMatrixFromOrthonormalBasisAndPos(const EGG::Vector3f &v);

    [[nodiscard]] static f32 CheckPointAgainstLineSegment(const EGG::Vector3f &point,
            const EGG::Vector3f &a, const EGG::Vector3f &b);
    [[nodiscard]] static EGG::Vector3f RotateXZByYaw(f32 angle, const EGG::Vector3f &v);
    [[nodiscard]] static EGG::Vector3f RotateAxisAngle(f32 angle, const EGG::Vector3f &axis,
            const EGG::Vector3f &v1);
    static void SetRotTangentHorizontal(EGG::Matrix34f &mat, const EGG::Vector3f &up,
            const EGG::Vector3f &tangent);
    [[nodiscard]] static EGG::Matrix34f OrthonormalBasis(const EGG::Vector3f &v);
    [[nodiscard]] static EGG::Matrix34f RailOrthonormalBasis(
            const RailInterpolator &railInterpolator);
    [[nodiscard]] static EGG::Vector3f AdjustVecForward(f32 sidewaysScalar, f32 forwardScalar,
            f32 minSpeed, const EGG::Vector3f &src, EGG::Vector3f forward);

    /// @addr{0x806B59A8}
    /// @brief Solves the standard kinematic equation \f$y(t) = v_0\, t - \frac{1}{2} a t^{2}\f$
    [[nodiscard]] static f32 CalcParabolicDisplacement(f32 initVel, f32 accel, u32 frame) {
        f32 t = static_cast<f32>(frame);
        return initVel * t - t * (0.5f * accel * t);
    }

    /// @addr{0x8086C098}
    /// @brief Linearly interpolates between two vectors
    [[nodiscard]] static EGG::Vector3f Interpolate(f32 t, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1) {
        return v0 + (v1 - v0) * t;
    }

    Render::DrawMdl *m_drawMdl;
    Abstract::g3d::ResFile *m_resFile;
    ObjectId m_id;
    RailInterpolator *m_railInterpolator;
    BoxColUnit *m_boxColUnit;
    const System::MapdataGeoObj *m_mapObj;

private:
    Flags m_flags; ///< The properties of the transform matrix to be recalculated
    EGG::Vector3f m_pos;
    EGG::Vector3f m_scale;
    EGG::Vector3f m_rot;
    bool m_rotLock;
    EGG::Matrix34f m_transform; ///< The 3x4 matrix representing the object's position and rotation
};

} // namespace Kinoko::Field
