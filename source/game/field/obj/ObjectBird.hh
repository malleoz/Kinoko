#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectBirdSubB0;
class ObjectBirdSubBC;

class ObjectBird final : public ObjectCollidable {
public:
    ObjectBird(const System::MapdataGeoObj &params);
    ~ObjectBird() override;

    void calc() override;

    /// @addr{0x8077CCF4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x8077CCF0}
    void loadGraphics() override {}

    /// @addr{0x8077CCE8}
    void createCollision() override {}

    /// @ADDR{0X8077CCE0}
    void loadRail() override {}

    [[nodiscard]] const ObjectBirdSubB0 *subB0() const {
        return m_subB0;
    }

    [[nodiscard]] const auto &subBC() const {
        return m_subBC;
    }

protected:
    ObjectBirdSubB0 *m_subB0;
    std::span<ObjectBirdSubBC *> m_subBC;
};

class ObjectBirdSubB0 : public ObjectCollidable {
public:
    ObjectBirdSubB0(const System::MapdataGeoObj &params, ObjectBird *bird);
    ~ObjectBirdSubB0() override;

    void init() override;
    void calc() override;

    /// @addr{0x8077CCD4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    void loadAnims() override;

    /// @brief Not overridden in the base game, but collision mode 0 will cause our assert to fail.
    void createCollision() override {}

    [[nodiscard]] const EGG::Vector3f &ac() const {
        return m_ac;
    }

protected:
    EGG::Vector3f m_ac;
    ObjectBird *m_bird;
};

class ObjectBirdSubBC final : public ObjectBirdSubB0 {
public:
    ObjectBirdSubBC(const System::MapdataGeoObj &params, ObjectBird *bird, u32 idx);
    ~ObjectBirdSubBC() override;

    void init() override;
    void calc() override;

private:
    void FUN_8077C8F4();

    u32 m_idx;
    EGG::Vector3f m_c4;
    f32 m_d0;
};

} // namespace Field
