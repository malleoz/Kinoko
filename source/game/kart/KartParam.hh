#pragma once

#include "game/field/KCollisionTypes.hh"

namespace Kinoko::Kart {

/// @brief Houses hitbox and wheel positions, radii, and suspension info
/// @details Stands for <b>B</b>inary <b>S</b>ettings and <b>P</b>hysics. There is a `.bsp` file for
/// each character+vehicle combination which represents the associated vehicle collision data. It
/// has the following structure:

// clang-format off

/**
 * Offset | Type               | Member        | Description                                                                     |
 * ------ | ------------------ | ------------- | ------------------------------------------------------------------------------- |
 * 0x00   | f32                | offsetY       | The body's position above the floor                                             |
 * 0x04   | @ref BSP::Hitbox   | hitboxes      | Array of vehicle hitboxes, not all of which are active                          |
 * 0x184  | @ref EGG::Vector3f | cuboids[0]    | First cuboid for the inertia tensor with a mass of 1                            |
 * 0x190  | @ref EGG::Vector3f | cuboids[1]    | Second cuboid for the inertia tensor with a mass of 12                          |
 * 0x19C  | f32                | angVel0Factor | Multiplier applied to angular velocity on the vehicle                           |
 * 0x1A0  | f32                | _1a0          | Unused                                                                          |
 * 0x1A4  | @ref BSP::Wheel    | wheels[0]     | For bikes, front wheel. For karts, front right wheel                            |
 * 0x1D0  | @ref BSP::Wheel    | wheels[1]     | For bikes, back wheel. For karts, back right wheel                              |
 * 0x1FC  | @ref BSP::Wheel    | wheels[2-3]   | Unused (Quacker and karts just mirror the other wheel)                          |
 * 0x254  | f32                | rumbleHeight  | Max vertical distance of the vehicle body's rumble animation (unused in Kinoko) |
 * 0x258  | u16                | rumbleSpeed   | Speed of the vehicle body's rumble animation (unused in Kinoko)                 |
 * 0x25A  | 2 bytes            | Padding       |                                                                                 |
 **/

// clang-format on

/// The Hitboxes have the following structure:

// clang-format off

/**
 * Offset | Type               | Member    | Description                                       |
 * ------ | ------------------ | --------- | ------------------------------------------------- |
 * 0x00   | u16                | enable    | Specifies if this is an active hitbox             |
 * 0x02   | u16                | Padding   |                                                   |
 * 0x04   | @ref EGG::Vector3f | position  | The relative position of the hitbox's center      |
 * 0x10   | f32                | radius    | The hitbox's radius                               |
 * 0x14   | u16                | wallsOnly | Specifies if the hitbox only collides with walls  |
 * 0x16   | u16                | tireIdx   | The index of the tire associated with this hitbox |
 **/

// clang-format on

/// The Wheels have the following structure:

// clang-format off

/**
 * Offset | Type               | Member          | Description                                                                 |
 * ------ | ------------------ | --------------- | --------------------------------------------------------------------------- |
 * 0x00   | u16                | enable          | Specifies if this is an active wheel                                        |
 * 0x02   | u16                | Padding         |                                                                             |
 * 0x04   | f32                | springStiffness | The wheel's suspension spring stiffness                                     |
 * 0x08   | f32                | dampingFactor   | The wheel's suspension damping factor                                       |
 * 0x0C   | f32                | maxTravel       | The max distance between the topmost and bottommost wheel positions         |
 * 0x10   | @ref EGG::Vector3f | springTop       | The highest point the wheel can reach when bouncing                         |
 * 0x1C   | f32                | xRot            | The wheel's rotation around the X-axis (always 0)                           |
 * 0x20   | f32                | wheelRadius     | The radius of the wheel                                                     |
 * 0x24   | f32                | sphereRadius    | The radius of the wheel's sphere used for collision detection               |
 * 0x28   | u32                | _28             | Unused                                                                      |
 **/

// clang-format on
struct BSP {
    /// @brief Represents one of the many hitboxes that make up a vehicle
    struct Hitbox {
        u16 enable;             ///< Specifies if this is an active hitbox (1) or inactive (0)
        EGG::Vector3f position; ///< The relative position of the hitbox's center
        f32 radius;             ///< The hitbox's radius
        u16 wallsOnly;          ///< Specifies if the hitbox only collides with walls and not floors
        u16 tireIdx;            ///< Unused - kept so the struct's size static assert passes
    };
    STATIC_ASSERT(sizeof(Hitbox) == 0x18);

    /// @brief Represents a wheel of the vehicle and its suspension properties
    struct Wheel {
        u16 enable;              ///< Specifies if this is an active wheel (1) or inactive (0)
        f32 springStiffness;     ///< Suspension spring stiffness
        f32 dampingFactor;       ///< Suspension damping factor
        f32 maxTravel;           ///< Max distance between the topmost and bottommost positions
        EGG::Vector3f springTop; ///< Highest point the wheel can reach when bouncing
        f32 xRot;                ///< The wheel's rotation around the X-axis (always 0)
        f32 wheelRadius;         ///< The radius of the wheel
        f32 sphereRadius;        ///< The radius of the wheel's sphere used for collision detection
        u32 _28;                 ///< Unused - kept so the struct's size static assert passes
    };
    STATIC_ASSERT(sizeof(Wheel) == 0x2c);

    BSP();
    BSP(EGG::RamStream &stream);

    void read(EGG::RamStream &stream);

    f32 offsetY;                     ///< The body's position above the floor
    std::array<Hitbox, 16> hitboxes; ///< Array of vehicle hitboxes, not all of which are active
    EGG::Vector3f cuboids[2];        ///< Cuboid dimensions for computing moment of inertia
    f32 angVel0Factor;               ///< Multiplier applied to angular velocity on the vehicle
    f32 _1a0;                        ///< Unused - kept so the struct's size static assert passes
    std::array<Wheel, 4> wheels;     ///< Array of vehicle wheels, not all of which are active
    f32 rumbleHeight; ///< Max vertical distance of the vehicle body's rumble animation.
    u16 rumbleSpeed;  ///< Speed of the vehicle body's rumble animation.
};
STATIC_ASSERT(sizeof(BSP) == 0x25c);

/// @brief Houses stats regarding a given character/vehicle combo.
class KartParam {
public:
    /// @brief Parsed from `bikePartsDispParam.bin`. Contains display parameters for each bike.
    /// @brief @ref KartBodyBike uses the handlebar position and rotation to compute front wheel's
    /// transformation matrix.
    /// @note The camera distance is implemented in Kinoko because the camera performs collision
    /// checks during the race and this can result in an @ref Field::ObjColMgr transformation matrix
    /// update. This can cause desyncs on DS Delfino Square if not implemented.
    struct BikeDisp {
        /// @brief Uninitialized default constructor
        BikeDisp() = default;

        /// @brief Constructor which parses out the display parameters from the provided stream
        /// @param stream A @ref EGG::RamStream of data from `bikePartsDispParam.bin`
        BikeDisp(EGG::RamStream &stream) {
            read(stream);
        }

        /// @brief Default destructor
        ~BikeDisp() = default;

        /// @brief Parses out the display parameters for a given bike from a bikePartsDispParam.bin
        /// stream
        /// @param stream A @ref EGG::RamStream of data from `bikePartsDispParam.bin`
        void read(EGG::RamStream &stream) {
            m_cameraDistY = stream.read_f32();
            stream.skip(0x8);
            m_handlePos.read(stream);
            m_handleRot.read(stream);
        }

        f32 m_cameraDistY;         ///< Camera target vertical distance
        u8 _04[0x0c - 0x04];       ///< Unused - kept so the struct's size static assert passes
        EGG::Vector3f m_handlePos; ///< Handlebar position relative to the bike's origin
        EGG::Vector3f m_handleRot; ///< Handlebar rotation
        u8 _24[0xb0 - 0x24];       ///< Unused - kept so the struct's size static assert passes
    };
    STATIC_ASSERT(sizeof(BikeDisp) == 0xB0);

    /// @brief Parsed from `kartPartsDispParam.bin`. Contains display parameters for each kart.
    /// @note The camera distance is implemented in Kinoko because the camera performs collision
    /// checks during the race and this can result in an @ref Field::ObjColMgr transformation matrix
    /// update. This can cause desyncs on DS Delfino Square if not implemented.
    struct KartDisp {
        /// @brief Uninitialized default constructor
        KartDisp() = default;

        /// @brief Constructor which parses out the display parameters from the provided stream
        /// @param stream A @ref EGG::RamStream of data from `kartPartsDispParam.bin`
        KartDisp(EGG::RamStream &stream) {
            read(stream);
        }

        /// @brief Default destructor
        ~KartDisp() = default;

        /// @brief Parses out the display parameters for a given kart from a
        /// `kartPartsDispParam.bin` stream
        /// @param stream A @ref EGG::RamStream of data from kartPartsDispParam.bin
        void read(EGG::RamStream &stream) {
            m_cameraDistY = stream.read_f32();
        }

        f32 m_cameraDistY;      ///< Camera target vertical distance
        u8 _004[0x150 - 0x004]; ///< Unused - kept so the struct's size static assert passes
    };
    STATIC_ASSERT(sizeof(KartDisp) == 0x150);

    /// @brief Parsed from `kartParam.bin`. Contains vehicle statistics like handling and speed.
    struct Stats {
        /// @brief The body style of the vehicle. Basically the number of wheels.
        enum class Body {
            Four_Wheel_Kart = 0,       ///< Used by most karts
            Handle_Relative_Bike = 1,  ///< Used by most bikes
            Vehicle_Relative_Bike = 2, ///< Used by Quacker
            Three_Wheel_Kart = 3,      ///< Used by Blue Falcon
        };

        /// @brief The type of drift (inside/outside).
        enum class DriftType {
            Outside_Drift_Kart = 0, ///< Outward drifting kart
            Outside_Drift_Bike = 1, ///< Outward drifting bike
            Inside_Drift_Bike = 2,  ///< Inward drifting bike
        };

        /// @brief Uninitialized default constructor
        Stats() = default;

        /// @brief Constructor which parses out the stats for a given `kartParam.bin` stream
        /// @param stream A @ref EGG::RamStream of data from `kartParam.bin`
        Stats(EGG::RamStream &stream) {
            read(stream);
        }

        /// @brief Default destructor
        ~Stats() = default;

        void read(EGG::RamStream &stream);
        void applyCharacterBonus(EGG::RamStream &stream);

        Body body;               ///< The body style of the vehicle
        DriftType driftType;     ///< The type of drift
        WeightClass weightClass; ///< The weight class of the vehicle
        f32 _00c;                ///< Unused - kept so the struct's size static assert passes

        f32 weight; ///< Unused in Kinoko - kept so the struct's size static assert passes
        f32 bumpDeviationLevel; ///< Scale applied to forces resulting from wall collision
        f32 speed;              ///< Base full speed of the character/vehicle combo.
        f32 turningSpeed;       ///< Speed decrement percentage of the vehicle when handling.
        f32 tilt;               ///< Percentage of how much the kart can roll
        std::array<f32, 4> accelerationStandardA; ///< Boost accel values at each speed threshold
        std::array<f32, 3> accelerationStandardT; ///< Speed ratio threshold for acceleration values
        std::array<f32, 2> accelerationDriftA; ///< Mini-turbo accel values at each speed threshold
        std::array<f32, 1> accelerationDriftT; ///< Speed ratio threshold for mini-turbo accel vals
        f32 handlingManualTightness;    ///< Affects turn radius when manual and not drifting.
        f32 handlingAutomaticTightness; ///< Affects turn radius when auto and not drifting.
        f32 handlingReactivity;         ///< A weight applied to turn radius when not drifting.
        f32 driftManualTightness;       ///< Affects turn radius when manual drifting.
        f32 driftAutomaticTightness;    ///< Affects turn radius when automatic drifting.
        f32 driftReactivity;            ///< A weight applied to turn radius when drifting.
        f32 driftOutsideTargetAngle; ///< Target angle for outside drifting karts/bikes (always 45)
        f32 driftOutsideDecrement;   ///< Rate of angle change after a drift (always 0.8)
        u32 miniTurboDuration;       ///< The framecount duration of a charged mini-turbo.
        std::array<f32, KColType::COL_TYPE_COUNT> kclSpeed; ///< Speed multipliers for each KCL flag
        std::array<f32, KColType::COL_TYPE_COUNT> kclRot;   ///< Rotation scalars for each KCL flag
        f32 _170;           ///< Unused in Kinoko - kept so the struct's size static assert passes
        f32 _174;           ///< Unused in Kinoko - kept so the struct's size static assert passes
        f32 _178;           ///< Unused in Kinoko - kept so the struct's size static assert passes
        f32 _17c;           ///< Unused in Kinoko - kept so the struct's size static assert passes
        f32 maxNormalForce; ///< Maximum upwards force that can be applied to suspension
        f32 megaScale;      ///< Unused in Kinoko - kept so the struct's size static assert passes
        f32 shrinkScale;    ///< Vehicle scale when shocked
    };
    STATIC_ASSERT(sizeof(Stats) == 0x18c);

    /// @brief Parsed from `kartCameraParam.bin`. Stores camera parameters for a given weight class.
    /// @note The camera parameters are implemented in Kinoko because the camera performs collision
    /// checks during the race and this can result in an @ref Field::ObjColMgr transformation matrix
    /// update. This can cause desyncs on DS Delfino Square if not implemented.
    struct KartCameraParam {
        /// @brief Uninitialized default constructor
        KartCameraParam() = default;

        /// @brief Constructor which parses out the camera parameters for a given
        /// `KartCameraParam.bin` stream
        /// @param stream A @ref EGG::RamStream of data from `kartCameraParam.bin`
        KartCameraParam(EGG::RamStream &stream) {
            read(stream);
        }

        /// @brief Default destructor
        ~KartCameraParam() = default;

        /// @brief Parses out the camera parameters for a given `kartCameraParam.bin` stream
        /// @param stream A @ref EGG::RamStream of data from `kartCameraParam.bin`
        void read(EGG::RamStream &stream) {
            fov = stream.read_f32();
            dist = stream.read_f32();
            posY = stream.read_f32();
            targetPosY = stream.read_f32();
        }

        f32 fov;        ///< Camera lens / field of view
        f32 dist;       ///< Camera distance from the vehicle's origin
        f32 posY;       ///< Camera Y position offset
        f32 targetPosY; ///< Unused - kept so the struct's size static assert passes
    };
    STATIC_ASSERT(sizeof(KartCameraParam) == 0x10);

    KartParam(Character character, Vehicle vehicle, u8 playerIdx);
    ~KartParam();

    /// @beginSetters
    void setTireCount(u16 tireCount) {
        m_tireCount = tireCount;
    }

    void setSuspCount(u16 suspCount) {
        m_suspCount = suspCount;
    }
    /// @endSetters

    /// @beginGetters
    /// @addr{0x80591DBC}
    [[nodiscard]] const BSP &bsp() const {
        return m_bsp;
    }

    /// @addr{0x80591F4C}
    [[nodiscard]] const Stats &stats() const {
        return m_stats;
    }

    /// @addr{0x80592620}
    [[nodiscard]] const BikeDisp &bikeDisp() const {
        return m_bikeDisp;
    }

    /// @addr{0x80592558}
    [[nodiscard]] const KartDisp &kartDisp() const {
        return m_kartDisp;
    }

    /// @addr{0x805927D4}
    [[nodiscard]] const KartCameraParam &camera() const {
        return m_camera;
    }

    [[nodiscard]] u8 playerIdx() const {
        return m_playerIdx;
    }

    [[nodiscard]] bool isBike() const {
        return m_isBike;
    }

    [[nodiscard]] bool isVehicleRelativeBike() const {
        return m_stats.body == Stats::Body::Vehicle_Relative_Bike;
    }

    [[nodiscard]] u16 suspCount() const {
        return m_suspCount;
    }

    [[nodiscard]] u16 tireCount() const {
        return m_tireCount;
    }
    /// @endGetters

private:
    void initStats(Character character, Vehicle vehicle);
    void initBikeDispParams(Vehicle vehicle);
    void initKartDispParams(Vehicle vehicle);
    void initHitboxes(Vehicle vehicle);
    void initCameraParams(Character character);

    Stats m_stats;            ///< The stats for the given vehicle
    BikeDisp m_bikeDisp;      ///< The display parameters for the given bike (if applicable)
    KartDisp m_kartDisp;      ///< The display parameters for the given kart (if applicable)
    BSP m_bsp;                ///< The hitbox and wheel data for the given vehicle
    KartCameraParam m_camera; ///< The camera parameters for the given vehicle
    u8 m_playerIdx;           ///< The player index of the given vehicle (always 0 in Kinoko)
    bool m_isBike;            ///< True if the vehicle is a bike, false if it is a kart
    u16 m_suspCount;          ///< The number of active suspensions on the vehicle
    u16 m_tireCount;          ///< The number of active tires on the vehicle
};

} // namespace Kinoko::Kart
