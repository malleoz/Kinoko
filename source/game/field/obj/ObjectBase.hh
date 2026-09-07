#pragma once

#include "game/field/BoxColManager.hh"
#include "game/field/RailInterpolator.hh"
#include "game/field/obj/ObjectId.hh"

#include "game/render/DrawMdl.hh"

#include "game/system/map/MapdataGeoObj.hh"

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

    /// @brief Describes whether the object needs to be updated every frame
    enum class eLoadFlags {
        None = 0, ///< No updates required
        Calc = 1, ///< The state of the object should be recalculated every frame
        Draw = 2, ///< The object should be re-drawn every frame
    };

    /// @brief A bitfield of @ref eLoadFlags that represents the update requirements of the object
    typedef EGG::TBitFlag<u16, eLoadFlags> LoadFlags;

    ObjectBase(const System::MapdataGeoObj &params);
    ObjectBase(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    virtual ~ObjectBase();

    /// @brief Run once during race initialization
    virtual void init() {}

    /// @brief Runs once per frame if the calc flag is set in loadFlags()
    virtual void calc() {}

    /// @addr{0x808217B8}
    /// @brief Updates the object's model
    virtual void calcModel() {
        calcTransform();
    }

    /// @brief Loads resources, collision, and registers the object to the ObjectDirector
    virtual void load() = 0;

    [[nodiscard]] virtual const char *getResources() const;
    virtual void loadGraphics();

    /// @brief Links associated animations from the object's resource file to the DrawMdl, if any
    virtual void loadAnims() {}

    /// @brief Creates a collision object that inherits from @ref ObjectCollisionBase
    virtual void createCollision() = 0;

    virtual void loadRail();

    /// @brief Updates the position, rotation, and scale of the collision object
    virtual void calcCollisionTransform() = 0;

    [[nodiscard]] virtual const char *getName() const;

    /// @addr{0x806BF434}
    /// @brief Returns a bitmask indicating which lifecycle hooks should be called for the object
    /// @return @ref eLoadFlags::None
    /// @details In Kinoko, the only effective behavior of this bitmask is to determine whether
    /// calc() should be called. The other bits are unused.
    [[nodiscard]] virtual LoadFlags loadFlags() const {
        // TODO: This references LOD to determine load flags
        return LoadFlags(eLoadFlags::None);
    }

    [[nodiscard]] virtual const char *getKclName() const;

    /// @addr{0x80821DB8}
    /// @brief Resizes the BoxColUnit associated with the object
    /// @details This does not affect the actual collision geometry of the object, only the
    /// BoxColUnit used for sptially indexed collision queries. This effectively adjusts how close
    /// the player needs to be to the object before collision checks are performed.
    /// @param radius The new radius of the collision box
    /// @param maxSpeed The maximum speed for the collision box
    virtual void resize(f32 radius, f32 maxSpeed) {
        m_boxColUnit->resize(radius, maxSpeed);
    }

    /// @addr{0x80821DD8}
    /// @brief Unregisters the BoxColUnit associated with the object from the BoxColManager
    virtual void unregisterCollision() {
        BoxColManager::Instance()->remove(m_boxColUnit);
    }

    /// @addr{0x80821DEC}
    /// @brief Skips collision checks for this object in the BoxColManager
    virtual void disableCollision() const {
        m_boxColUnit->m_flag.setBit(eBoxColFlag::Intangible);
    }

    /// @addr{0x80821E00}
    /// @brief Re-enables collision checks for this object in the BoxColManager
    virtual void enableCollision() const {
        m_boxColUnit->m_flag.resetBit(eBoxColFlag::Intangible);
    }

    /// @addr{0x80680618}
    /// @brief Fetches the @ref BoxColUnit associated with the object
    /// @return The @ref BoxColUnit associated with the object
    [[nodiscard]] virtual const BoxColUnit *getUnit() const {
        return m_boxColUnit;
    }

    /// @addr{0x80681598}
    /// @brief The position of the object in world space
    /// @return The position of the object in world space
    [[nodiscard]] virtual const EGG::Vector3f &getPosition() const {
        return m_pos;
    }

    /// @addr{0x8080BDC0}
    /// @brief Collision radius to use for GJK checks
    /// @return Default collision radius of `100.0f`
    [[nodiscard]] virtual f32 getCollisionRadius() const {
        return 100.0f;
    }

    /// @addr{0x80572574}
    /// @brief Fetches the unique identifier of the object
    /// @return The unique identifier of the object
    [[nodiscard]] virtual ObjectId id() const {
        return m_id;
    }

    /// @beginSetters
    /// @brief Sets the position of the object in world space and updates the position matrix flag
    /// @param pos The new position of the object in world space
    void setPos(const EGG::Vector3f &pos) {
        m_flags.setBit(eFlags::Position);
        m_pos = pos;
    }

    /// @brief Adds a vector to the object's position and updates the position matrix flag
    /// @param v The vector to add to the object's position
    void addPos(const EGG::Vector3f &v) {
        m_flags.setBit(eFlags::Position);
        m_pos += v;
    }

    /// @brief Subtracts a vector from the object's position and updates the position matrix flag
    /// @param v The vector to subtract from the object's position
    void subPos(const EGG::Vector3f &v) {
        m_flags.setBit(eFlags::Position);
        m_pos -= v;
    }

    /// @brief Sets the scale of the object and updates the scale flag
    /// @param scale The new scale of the object
    void setScale(const EGG::Vector3f &scale) {
        m_flags.setBit(eFlags::Scale);
        m_scale = scale;
    }

    /// @brief Sets the uniform scale of the object and updates the scale flag
    /// @param scale The new uniform scale of the object
    void setScale(f32 scale) {
        m_flags.setBit(eFlags::Scale);
        m_scale.set(scale);
    }

    /// @brief Sets the rotation of the object and updates the rotation flag
    /// @param rot The new rotation of the object
    void setRot(const EGG::Vector3f &rot) {
        m_flags.setBit(eFlags::Rotation);
        m_rot = rot;
    }

    /// @brief Sets the rotation of the object without updating the rotation flag
    /// @param rot The new rotation of the object
    void setRotNoFlag(const EGG::Vector3f &rot) {
        m_rot = rot;
    }

    /// @brief Adds a vector to the object's rotation and updates the rotation flag
    /// @param v The vector to add to the object's rotation
    void addRot(const EGG::Vector3f &v) {
        m_rotUpdated = true;
        m_flags.setBit(eFlags::Rotation);
        m_rot += v;
    }

    /// @brief Subtracts a vector from the object's rotation and updates the rotation flag
    /// @param v The vector to subtract from the object's rotation
    void subRot(const EGG::Vector3f &v) {
        m_rotUpdated = true;
        m_flags.setBit(eFlags::Rotation);
        m_rot -= v;
    }

    /// @addr{0x806C296C}
    /// @brief Sets the transformation matrix of the object and updates the matrix flag
    /// @param mat The new transformation matrix of the object
    void setTransform(const EGG::Matrix34f &mat) {
        m_rotUpdated = false;
        m_flags.setBit(eFlags::Matrix);
        m_transform = mat;
        m_pos = mat.base(3);
    }
    /// @endSetters

    /// @beginGetters
    /// @brief Gets the @ref RailInterpolator associated with the object
    /// @return The @RailInterpolator for the object
    [[nodiscard]] const RailInterpolator *railInterpolator() const {
        return m_railInterpolator;
    }

    /// @brief Gets the position of the object in world space
    /// @return The position of the object in world space
    [[nodiscard]] const EGG::Vector3f &pos() const {
        return m_pos;
    }

    /// @brief Gets the scale of the object
    /// @return The scale of the object
    [[nodiscard]] const EGG::Vector3f &scale() const {
        return m_scale;
    }

    /// @brief Gets the rotation of the object
    /// @return The rotation of the object
    [[nodiscard]] const EGG::Vector3f &rot() const {
        return m_rot;
    }

    /// @brief Gets the transformation matrix of the object
    /// @return The transformation matrix of the object
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
    /// @param initVel The initial velocity
    /// @param accel The acceleration
    /// @param frame The frame number
    /// @return The displacement at the given frame
    [[nodiscard]] static f32 CalcParabolicDisplacement(f32 initVel, f32 accel, u32 frame) {
        f32 t = static_cast<f32>(frame);
        return initVel * t - t * (0.5f * accel * t);
    }

    /// @addr{0x8086C098}
    /// @brief Linearly interpolates between two vectors
    /// @param t Interpolation factor
    /// @param v0 Starting vector
    /// @param v1 Ending vector
    /// @return The interpolated vector
    [[nodiscard]] static EGG::Vector3f Interpolate(f32 t, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1) {
        return v0 + (v1 - v0) * t;
    }

    Render::DrawMdl *m_drawMdl;           ///< The draw model associated with the object
    Abstract::g3d::ResFile *m_resFile;    ///< The resource file associated with the object
    ObjectId m_id;                        ///< The ID of the object
    RailInterpolator *m_railInterpolator; ///< The rail interpolator associated with the object
    BoxColUnit *m_boxColUnit;             ///< The box collision unit associated with the object
    const System::MapdataGeoObj *const m_mapObj; ///< The KMP object associated with the object

private:
    Flags m_flags;              ///< The properties of the transform matrix to be recalculated
    EGG::Vector3f m_pos;        ///< The position of the object
    EGG::Vector3f m_scale;      ///< The scale of the object
    EGG::Vector3f m_rot;        ///< The rotation of the object
    bool m_rotUpdated;          ///< Whether @ref m_rot is up-to-date
    EGG::Matrix34f m_transform; ///< The 3x4 matrix representing the object's position and rotation
};

} // namespace Kinoko::Field
