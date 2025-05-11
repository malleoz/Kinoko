#pragma once

#include "game/field/obj/ObjectCollidable.hh"

#include "game/render/DrawMdl.hh"

namespace Field {

class ObjectKoopaBall : public ObjectCollidable {
public:
    ObjectKoopaBall(const System::MapdataGeoObj &params);
    ~ObjectKoopaBall() override;

    void init() override;
    void calc() override;

    /// @addr{0x80771F68}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    /// @addr{0x80771F60}
    const EGG::Vector3f &getCollisionTranslation() const override {
        return m_collisionTranslation;
    }

private:
    enum class State {
        Tangible = 0,
        Intangible = 1,
        Exploding = 2,
    };

    void calcTangible();
    void calcExploding();
    void calcIntangible();

    void checkSphereFull();

    State m_state;                        // 0xb4
    EGG::Vector3f m_curRot;               // 0xb8
    f32 m_startYpos;                      // 0xc4
    EGG::Vector3f m_collisionTranslation; // 0xc8
    f32 m_angSpeed;                       // 0xd8
    s32 m_cooldownTimer;                // 0xe0, Frames until koopa will start to shoot the fireball
    Render::DrawMdl *m_bombCoreDrawMdl; // 0xe4
    s32 m_explodeTimer;                 // 0xe8
    u32 m_animFramecount;               // 0xec, Length of the animation
    EGG::Matrix34f m_curTransform;      // 0xf0
    f32 m_angleRad;                     // 0x120
    f32 m_curScale;                     // 0x128

    static constexpr f32 SCALE_INITIAL = ((870.0f * 2.0f) / 940.0f) * 0.5f;
    static constexpr f32 SCALE_DELTA = SCALE_INITIAL / 25.0f; // 0x12c
    static constexpr f32 INITIAL_VELOCITY = 400.0f;
    static constexpr f32 INITIAL_ANGULAR_SPEED = 3.0f;
};

} // namespace Field
