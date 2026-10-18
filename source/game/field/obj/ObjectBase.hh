#pragma once

#include "game/field/obj/ObjectId.hh"

#include <egg/core/BitFlag.hh>
#include <egg/math/Matrix.hh>

namespace Kinoko {

namespace Abstract::g3d {

class ResFile;

} // namespace Abstract::g3d

namespace Render {

enum class AnmType;
class DrawMdl;

} // namespace Render

namespace System {

class MapdataGeoObj;

} // namespace System

namespace Field {

class BoxColUnit;
class RailInterpolator;

/// @brief %Abstract base class for all objects in the game
/// @details Objects maintain position, rotation, and scale vectors. Position and scale are combined
/// into a 3x4 transformation matrix via calcTransform(). Each object manages its own BoxColUnit (if
/// it has collision) and optional rail interpolator. The class provides lifecycle hooks
/// (@ref init() / @ref calc() / @ref load() / @ref createCollision()) implemented by subclasses and
/// virtual methods for collision/resource management.
class ObjectBase {
public:
    /// @brief Tracks what properties have changed and require recalculation in the transform matrix
    enum class eFlags {
        Position = 0, ///< The object's position has changed
        Rotation = 1, ///< The object's rotation has changed
        Matrix = 2,   ///< The object's transformation matrix has changed
        Scale = 3,    ///< The object's scale has changed
    };

    /// @brief A bitfield of @ref eFlags that represents the state of the object
    typedef EGG::TBitFlag<u16, eFlags> Flags;

    /// @brief Describes whether the object needs to be updated every frame
    enum class eLoadFlags {
        Calc = 0, ///< The state of the object should be recalculated every frame
        Draw = 1, ///< The object should be re-drawn every frame
        Unk = 2,  ///< Unknown, used in @ref ObjectCollidable::loadAABB()
    };

    /// @brief A bitfield of @ref eLoadFlags that represents the update requirements of the object
    typedef EGG::TBitFlag<u16, eLoadFlags> LoadFlags;

    ObjectBase(const System::MapdataGeoObj &params);
    ObjectBase(const char *name, const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    virtual ~ObjectBase();

    virtual void init();
    virtual void calc();
    virtual void calcModel();
    virtual void loadGraphics();
    virtual void loadAnims();
    virtual void loadRail();
    [[nodiscard]] virtual const char *getResources() const;
    [[nodiscard]] virtual const char *getName() const;
    [[nodiscard]] virtual LoadFlags loadFlags() const;
    [[nodiscard]] virtual const char *getKclName() const;
    [[nodiscard]] virtual const EGG::Vector3f &getPosition() const;
    [[nodiscard]] virtual f32 getCollisionRadius() const;
    [[nodiscard]] virtual ObjectId id() const;

    /// @brief Loads resources, collision, and registers the object to the @ref ObjectDirector
    virtual void load() = 0;

    /// @brief Creates a collision object that inherits from @ref ObjectCollisionBase
    virtual void createCollision() = 0;

    /// @brief Updates the position, rotation, and scale of the collision object
    virtual void calcCollisionTransform() = 0;

    void resize(f32 radius, f32 maxSpeed);
    void unregisterCollision();
    void disableCollision() const;
    void enableCollision() const;
    [[nodiscard]] const BoxColUnit *getUnit() const;

    /// @beginSetters

    void setPos(const EGG::Vector3f &pos);
    void addPos(const EGG::Vector3f &v);
    void subPos(const EGG::Vector3f &v);
    void setScale(const EGG::Vector3f &scale);
    void setScale(f32 scale);
    void setRot(const EGG::Vector3f &rot);
    void setRotNoFlag(const EGG::Vector3f &rot);
    void addRot(const EGG::Vector3f &v);
    void subRot(const EGG::Vector3f &v);
    void setTransform(const EGG::Matrix34f &mat);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] const RailInterpolator *railInterpolator() const;
    [[nodiscard]] const EGG::Vector3f &pos() const;
    [[nodiscard]] const EGG::Vector3f &scale() const;
    [[nodiscard]] const EGG::Vector3f &rot() const;
    [[nodiscard]] const EGG::Matrix34f &transform() const;

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
    [[nodiscard]] static f32 CalcParabolicDisplacement(f32 initVel, f32 accel, u32 frame);

    Render::DrawMdl *m_drawMdl;           ///< The draw model associated with the object
    Abstract::g3d::ResFile *m_resFile;    ///< The resource file associated with the object
    const ObjectId m_id;                  ///< The ID of the object
    RailInterpolator *m_railInterpolator; ///< The rail interpolator associated with the object
    BoxColUnit *m_boxColUnit;             ///< The box collision unit associated with the object
    const System::MapdataGeoObj *const m_mapObj; ///< The KMP object associated with the object

private:
    Flags m_flags;              ///< The properties of the transform matrix to be recalculated
    EGG::Vector3f m_pos;        ///< The position of the object
    EGG::Vector3f m_scale;      ///< The scale of the object
    EGG::Vector3f m_rot;        ///< The rotation of the object (in radians)
    bool m_rotUpdated;          ///< Whether @ref m_rot is up-to-date
    EGG::Matrix34f m_transform; ///< The 3x4 matrix representing the object's position and rotation
};

} // namespace Field

} // namespace Kinoko
