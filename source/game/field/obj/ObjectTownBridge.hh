#include "game/field/obj/ObjectKCL.hh"

namespace Field {

class ObjectTownBridge : public ObjectKCL {
public:
    enum class State {
        Raising = 0,
        Raised = 1,
        Lowering = 2,
        Lowered = 3,
    };

    ObjectTownBridge(const System::MapdataGeoObj &params);
    ~ObjectTownBridge();

    void calc() override;

    /// @addr{0x8080ACD8}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    void createCollision() override;

    /// @addr{0x8080A8D0}
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return 500.0f;
    }

private:
    State calcState(s32 t) const;

    EGG::Vector3f m_bridgeRot;
    bool m_rotateUpwards; ///< Normally 1, otherwise the bridge will open downwards.
    f32 m_angVel;         ///< Speed of the bridge's movement.
    s32 m_pivotFrames;    ///< # of frames the bridge pivots up or down.
    s32 m_raisedFrames;   ///< # of frames the bridge remains raised.
    s32 m_loweredFrames;  ///< # of frames the bridge remains lowered.
    s32 m_fullAnimFrames; ///< The full duration of a bridge raise/lower loop.
    State m_state;
    ObjColMgr *m_raisedColMgr;
    ObjColMgr *m_midColMgr;
    ObjColMgr *m_flatColMgr;
};

} // namespace Field
