#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief Represents the drawbridge on DS Delfino Square
/// @details The drawbridge's collision changes depending on the angle of the bridge. The lowest
/// variant is not trickable, the middle is single flip trickable, and the highest is double flip
/// trickable.
/// @desync Because @ref m_objColMgr changes depending on the bridge's angle, this means that it is
/// more likely for the player to encounter the desync described in @ref ObjectKCL::checkCollision.
/// If the player gets airtime right before landing onto the bridge at the moment that the collision
/// changes, then it's possible for the player's collision checks to update the bridge collision's
/// AABB at a point in the frame when the ghost's collision check would not have had the AABB
/// updated, or vice versa.
class ObjectTownBridge final : public ObjectKCL {
public:
    ObjectTownBridge(const System::MapdataGeoObj &params);
    ~ObjectTownBridge() override;

    void calc() override;

    /// @addr{0x8080ACD8}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    void createCollision() override;

    /// @addr{0x8080A8D0}
    /// @copybrief ObjectKCL::colRadiusAdditionalLength()
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return 500.0f;
    }

private:
    /// @brief Describes the current motion/angle of the bridge
    enum class State {
        Raising = 0,  ///< The bridge is being raised
        Raised = 1,   ///< The bridge is fully raised and is now stationary
        Lowering = 2, ///< The bridge is being lowered
        Lowered = 3,  ///< The bridge is fully lowered and is now stationary
    };

    [[nodiscard]] f32 calcBridgeAngle(u32 t) const;
    [[nodiscard]] State calcState(u32 t) const;

    const bool m_rotateUpwards; ///< Normally true, otherwise the bridge will open downwards
    const f32 m_angVel;         ///< Speed of the bridge's movement
    const u32 m_pivotFrames;    ///< # of frames the bridge pivots up or down
    const u32 m_raisedFrames;   ///< # of frames the bridge remains raised
    const u32 m_loweredFrames;  ///< # of frames the bridge remains lowered
    const u32 m_fullAnimFrames; ///< The full duration of a bridge raise/lower loop
    State m_state;              ///< The current motion/angle state of the bridge
    ObjColMgr *m_raisedColMgr;  ///< Collision manager when the bridge angle is > 30 degrees
    ObjColMgr *m_midColMgr;     ///< Collision manager when the bridge angle is > 10 degrees
    ObjColMgr *m_flatColMgr;    ///< Collision manager for the flat bridge state
};

} // namespace Kinoko::Field
