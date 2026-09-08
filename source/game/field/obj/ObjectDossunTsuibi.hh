#include "game/field/obj/ObjectDossun.hh"

namespace Kinoko::Field {

class ObjectDossunTsuibiHolder;

/// @brief Represents one of the two thwomps that oscillate and stomp in the long rBC hallway.
/// @details Interfaces with @ref ObjectDossunTsuibiHolder so both Thwomps stop at the same time.
class ObjectDossunTsuibi final : public ObjectDossun {
    friend ObjectDossunTsuibiHolder;

public:
    /// @addr{0x8076393C}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectDossunTsuibi(const System::MapdataGeoObj &params, ObjectDossunTsuibiHolder *holder)
        : ObjectDossun(params),
          m_holder(holder) {}

    /// @addr{0x80764C48}
    /// @brief Default virtual destructor
    ~ObjectDossunTsuibi() override = default;

    /// @addr{0x80763A04}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        m_touchingGround = false;
    }

    /// @addr{0x80763A10}
    void startStill() override {
        ObjectDossun::startStill();
        m_holder->startStill();
    }

private:
    ObjectDossunTsuibiHolder *m_holder; ///< Pointer to the manager class
};

} // namespace Kinoko::Field
