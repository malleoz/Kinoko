namespace Field {

#include "game/field/obj/ObjectKCL.hh"

class ObjectTownBridge : public ObjectKCL {
public:
    ObjectTownBridge(const System::MapdataGeoObj &params);
    ~ObjectTownBridge();

    void calc() override;

    /// @addr{0x8080ACD8}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    void createCollision() override;

private:
    EGG::Vector3f m_bridgeRot;
    bool m_rotateUpwards; ///< Normally 1, otherwise the bridge will open downwards.
    f32 m_angVel;         ///< Speed of the bridge's movement.
    u32 m_pivotFrames;    ///< # of frames the bridge pivots up or down.
    u32 m_raisedFrames;   ///< # of frames the bridge remains raised.
    u32 m_120;
    u32 m_fullAnimFrames; ///< The full duration of a bridge raise/lower loop.
    u32 m_state;
    ObjColMgr *m_raisedColMgr;
    ObjColMgr *m_midColMgr;
    ObjColMgr *m_flatColMgr;
};

} // namespace Field
