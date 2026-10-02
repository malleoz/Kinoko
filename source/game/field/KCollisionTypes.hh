#pragma once
/// @file KCollisionTypes.hh
/// This file houses information about KCL types, as well as the struct for collision prisms.
/// KCL files define collision for Mario %Kart Wii and other games. They organize collision
/// triangles using an octree, which allows for efficient indexing. Each triangle is associated with
/// a 16-bit flag, which determines the effect the triangle has on the player colliding with it.
/// <center><table><caption id="kcl_flag_components">KCL Flag Components</caption>
/// <tr><th>15<th>14<th>13<th>12<th>11<th>10<th>9<th>8<th>7<th>6<th>5<th>4<th>3<th>2<th>1<th>0
/// <tr><td colspan="3"><center>Collision Effect</center><td colspan="2"><center>Wheel
/// Depth</center><td colspan="3"><center>BLIGHT Index</center><td
/// colspan="3"><center>Variant</center><td colspan="5"><center>@ref KColType</center>
/// </table></center>
/// - **@ref KColType** - 0x00 to 0x1F. The main effect the triangle will have.
/// - **Variant** - 0 to 7. Determines the variant of the base type.
/// - **BLIGHT Index** - Ranges from 0 to 7. Has to do with lighting.
/// - **Wheel Depth** - 0 to 3. A higher value means player's wheel sinks further into the ground.
/// - **Collision effect** - Bitfield that makes surfaces trickable, reject road, and/or soft wall.
/// .

#include <egg/math/Vector.hh>

// Credit: em-eight/mkw

/// @brief Computes the "Base Type" portion of the KCL flags. It's the lower 5 bits of the flag.
#define KCL_ATTRIBUTE_TYPE(x) ((x) & 0x1f)

/// @brief Converts an attribute to the type mask bitfield for that attribute. The game uses this
/// method in order to create a mask that represents multiple KCL attributes.
#define KCL_TYPE_BIT(x) (1 << (x))

/// @brief Given the full 2 byte KCL flag for a triangle, extracts the "Base Type" portion of the
/// flag.
#define KCL_ATTRIBUTE_TYPE_BIT(x) KCL_TYPE_BIT(KCL_ATTRIBUTE_TYPE(x))

/// @brief KCL attribute types
typedef enum {
    COL_TYPE_ROAD = 0,                       ///< Default road.
    COL_TYPE_SLIPPERY_ROAD = 1,              ///< Like Dry Dry Ruins.
    COL_TYPE_WEAK_OFF_ROAD = 2,              ///< Slight off-road slowdown.
    COL_TYPE_OFF_ROAD = 3,                   ///< Moderate off-road slowdown.
    COL_TYPE_HEAVY_OFF_ROAD = 4,             ///< Heavy off-road slowdown.
    COL_TYPE_SLIPPERY_ROAD_2 = 5,            ///< N64 Sherbet Land ice.
    COL_TYPE_BOOST_PAD = 6,                  ///< Boost panel.
    COL_TYPE_BOOST_RAMP = 7,                 ///< Trickable. Variant affects boost duration.
    COL_TYPE_JUMP_PAD = 8,                   ///< Like GBA Shy Guy Beach.
    COL_TYPE_ITEM_ROAD = 9,                  ///< Road that is only solid for items.
    COL_TYPE_SOLID_OOB = 0xa,                ///< Solid out-of-bounds trigger.
    COL_TYPE_MOVING_WATER = 0xb,             ///< Koopa Cape and DS Yoshi Falls.
    COL_TYPE_WALL = 0xc,                     ///< Default wall.
    COL_TYPE_INVISIBLE_WALL = 0xd,           ///< Solid wall that is (typically) invisible.
    COL_TYPE_ITEM_WALL = 0xe,                ///< Wall that is only solid for items.
    COL_TYPE_WALL_2 = 0xf,                   ///< Difference to @ref COL_TYPE_WALL is unknown.
    COL_TYPE_FALL_BOUNDARY = 0x10,           ///< Non-solid out-of-bounds trigger.
    COL_TYPE_CANNON_TRIGGER = 0x11,          ///< Launches player to a destination "cannon point".
    COL_TYPE_FORCE_RECALCULATE_ROUTE = 0x12, ///< Forces CPU pathing recalculation.
    COL_TYPE_HALFPIPE_RAMP = 0x13,           ///< Bowser's Castle half-pipe ramps.
    COL_TYPE_PLAYER_WALL = 0x14,             ///< Allows items to pass through but not players.
    COL_TYPE_MOVING_ROAD = 0x15,             ///< TF conveyers and CM escalators.
    COL_TYPE_STICKY_ROAD = 0x16,             ///< Player sticks if within 200 units (rBC stairs).
    COL_TYPE_ROAD2 = 0x17,                   ///< @ref COL_TYPE_ROAD but with more variants.
    COL_TYPE_SOUND_TRIGGER = 0x18,           ///< Triggers for changing sound effects per-course.
    COL_TYPE_WEAK_WALL = 0x19,               ///< Pushes player away only at low speeds.
    COL_TYPE_EFFECT_TRIGGER = 0x1a,          ///< Activates various effects based on variant.
    COL_TYPE_ITEM_STATE_MODIFIER = 0x1b,     ///< Stops items when leaving curved TF conveyers.
    COL_TYPE_HALFPIPE_INVISIBLE_WALL = 0x1c, ///< Invisible wall after a half-pipe jump, like in BC.
    COL_TYPE_ROTATING_ROAD = 0x1d,           ///< Chain Chomp Roulette.
    COL_TYPE_SPECIAL_WALL = 0x1e,            ///< Various other wall types, determined by variant.
    COL_TYPE_INVISIBLE_WALL2 = 0x1f,         ///< Identical to @ref COL_TYPE_INVISIBLE_WALL.

    COL_TYPE_COUNT
} KColType;

#define KCL_SOFT_WALL_MASK 0x8000 ///< Masks the soft wall bit within the "Collision Effect" field
#define KCL_ANY 0xffffffff        ///< Represents all KCL type flags being set
#define KCL_NONE 0x00000000       ///< Represents no KCL type flags being set

/// @brief Represents KCL which is only treated as a collision if the object is moving towards it
/// @details Value: 0x05070000
#define KCL_TYPE_DIRECTIONAL \
    (KCL_TYPE_BIT(COL_TYPE_FALL_BOUNDARY) | KCL_TYPE_BIT(COL_TYPE_SOUND_TRIGGER) | \
            KCL_TYPE_BIT(COL_TYPE_FORCE_RECALCULATE_ROUTE) | \
            KCL_TYPE_BIT(COL_TYPE_EFFECT_TRIGGER) | KCL_TYPE_BIT(COL_TYPE_CANNON_TRIGGER))
STATIC_ASSERT(KCL_TYPE_DIRECTIONAL == 0x05070000);

/// @brief Represents a tangible surface that the player or items can interact with.
/// @details Value: 0xF0F8FFFF
#define KCL_TYPE_SOLID_SURFACE \
    (~KCL_TYPE_BIT(COL_TYPE_FALL_BOUNDARY) & ~KCL_TYPE_BIT(COL_TYPE_CANNON_TRIGGER) & \
            ~KCL_TYPE_BIT(COL_TYPE_FORCE_RECALCULATE_ROUTE) & \
            ~KCL_TYPE_BIT(COL_TYPE_SOUND_TRIGGER) & ~KCL_TYPE_BIT(COL_TYPE_WEAK_WALL) & \
            ~KCL_TYPE_BIT(COL_TYPE_EFFECT_TRIGGER) & ~KCL_TYPE_BIT(COL_TYPE_ITEM_STATE_MODIFIER))
STATIC_ASSERT(KCL_TYPE_SOLID_SURFACE == 0xF0F8FFFF);

/// @brief Represents surfaces that are drivable by players and/or items
/// @details Value: 0x20E80FFF
#define KCL_TYPE_FLOOR \
    (KCL_TYPE_BIT(COL_TYPE_ROAD) | KCL_TYPE_BIT(COL_TYPE_SLIPPERY_ROAD) | \
            KCL_TYPE_BIT(COL_TYPE_WEAK_OFF_ROAD) | KCL_TYPE_BIT(COL_TYPE_OFF_ROAD) | \
            KCL_TYPE_BIT(COL_TYPE_HEAVY_OFF_ROAD) | KCL_TYPE_BIT(COL_TYPE_SLIPPERY_ROAD_2) | \
            KCL_TYPE_BIT(COL_TYPE_BOOST_PAD) | KCL_TYPE_BIT(COL_TYPE_BOOST_RAMP) | \
            KCL_TYPE_BIT(COL_TYPE_JUMP_PAD) | KCL_TYPE_BIT(COL_TYPE_ITEM_ROAD) | \
            KCL_TYPE_BIT(COL_TYPE_SOLID_OOB) | KCL_TYPE_BIT(COL_TYPE_MOVING_WATER) | \
            KCL_TYPE_BIT(COL_TYPE_HALFPIPE_RAMP) | KCL_TYPE_BIT(COL_TYPE_MOVING_ROAD) | \
            KCL_TYPE_BIT(COL_TYPE_STICKY_ROAD) | KCL_TYPE_BIT(COL_TYPE_ROAD2) | \
            KCL_TYPE_BIT(COL_TYPE_ROTATING_ROAD))
STATIC_ASSERT(KCL_TYPE_FLOOR == 0x20E80FFF);

/// @brief Represents surfaces that are drivable by players but not items
/// @details Value: 0x20E80DFF
#define KCL_TYPE_DRIVER_FLOOR (KCL_TYPE_FLOOR & ~KCL_TYPE_BIT(COL_TYPE_ITEM_ROAD))
STATIC_ASSERT(KCL_TYPE_DRIVER_FLOOR == 0x20E80DFF);

/// @brief Represents surfaces that act like walls for players and/or items
/// @details Value: 0xD010F000
#define KCL_TYPE_WALL \
    (KCL_TYPE_BIT(COL_TYPE_WALL) | KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL) | \
            KCL_TYPE_BIT(COL_TYPE_ITEM_WALL) | KCL_TYPE_BIT(COL_TYPE_WALL_2) | \
            KCL_TYPE_BIT(COL_TYPE_PLAYER_WALL) | KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL) | \
            KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL) | KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL2))
STATIC_ASSERT(KCL_TYPE_WALL == 0xD010F000);

/// @brief Represents surfaces that act like walls for players
/// @details Value: 0xC010B000
#define KCL_TYPE_DRIVER_WALL \
    (KCL_TYPE_WALL & ~KCL_TYPE_BIT(COL_TYPE_ITEM_WALL) & \
            ~KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL))
STATIC_ASSERT(KCL_TYPE_DRIVER_WALL == 0xC010B000);

/// @brief Represents #KCL_TYPE_DRIVER_WALL but without #COL_TYPE_INVISIBLE_WALL
/// @details Value: 0xC0109000
#define KCL_TYPE_DRIVER_WALL_NO_INVISIBLE_WALL \
    (KCL_TYPE_DRIVER_WALL & ~KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL))
STATIC_ASSERT(KCL_TYPE_DRIVER_WALL_NO_INVISIBLE_WALL == 0xC0109000);

/// @brief Represents #KCL_TYPE_DRIVER_WALL but without #COL_TYPE_INVISIBLE_WALL2
/// @details Value: 0x4010B000
#define KCL_TYPE_DRIVER_WALL_NO_INVISIBLE_WALL2 \
    (KCL_TYPE_DRIVER_WALL & ~KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL2))
STATIC_ASSERT(KCL_TYPE_DRIVER_WALL_NO_INVISIBLE_WALL2 == 0x4010B000);

/// @brief Represents all visible wall KCL types
/// @details Value: 0x4010D000
#define KCL_TYPE_VISIBLE_WALL \
    (KCL_TYPE_DRIVER_WALL_NO_INVISIBLE_WALL & ~KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL2) | \
            KCL_TYPE_BIT(COL_TYPE_ITEM_WALL))
STATIC_ASSERT(KCL_TYPE_VISIBLE_WALL == 0x4010D000);

/// @brief Represents surfaces that act like walls when objects collide with them
/// @details Value: 0x4000D000
#define KCL_TYPE_OBJECT_WALL \
    (KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL) | KCL_TYPE_BIT(COL_TYPE_WALL_2) | \
            KCL_TYPE_BIT(COL_TYPE_ITEM_WALL) | KCL_TYPE_BIT(COL_TYPE_WALL))
STATIC_ASSERT(KCL_TYPE_OBJECT_WALL == 0x4000D000);

/// @brief Represents all surfaces that vehicles can interact with
/// @details Value: 0xEFFFBDFF
#define KCL_TYPE_VEHICLE_INTERACTABLE \
    (~KCL_TYPE_BIT(COL_TYPE_ITEM_ROAD) & ~KCL_TYPE_BIT(COL_TYPE_ITEM_WALL) & \
            ~KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL))
STATIC_ASSERT(KCL_TYPE_VEHICLE_INTERACTABLE == 0xEFFFBDFF);

/// @brief Represents #KCL_TYPE_VEHICLE_INTERACTABLE but without triggers or fall boundaries
/// @details Value: 0xEAF8BDFF
#define KCL_TYPE_VEHICLE_COLLIDEABLE \
    (KCL_TYPE_VEHICLE_INTERACTABLE & ~KCL_TYPE_BIT(COL_TYPE_SOUND_TRIGGER) & \
            ~KCL_TYPE_BIT(COL_TYPE_EFFECT_TRIGGER) & ~KCL_TYPE_BIT(COL_TYPE_FALL_BOUNDARY) & \
            ~KCL_TYPE_BIT(COL_TYPE_CANNON_TRIGGER) & \
            ~KCL_TYPE_BIT(COL_TYPE_FORCE_RECALCULATE_ROUTE))
STATIC_ASSERT(KCL_TYPE_VEHICLE_COLLIDEABLE == 0xEAF8BDFF);

/// @brief Represents the collision types that @ref Render::KartCamera checks for collision with
/// @details Value: 0x62F8BDFF
#define KCL_TYPE_CAMERA_COLLIDABLE \
    (KCL_TYPE_VEHICLE_COLLIDEABLE & ~KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL2) & \
            ~KCL_TYPE_BIT(COL_TYPE_ITEM_STATE_MODIFIER))
STATIC_ASSERT(KCL_TYPE_CAMERA_COLLIDABLE == 0x62F8BDFF);

/// @brief Represents all KCL types whose collision is not conditional on the direction of impact
/// @details Value: 0xE0F8BDFF
#define KCL_TYPE_NON_DIRECTIONAL \
    (KCL_TYPE_VEHICLE_COLLIDEABLE & ~KCL_TYPE_BIT(COL_TYPE_ITEM_STATE_MODIFIER) & \
            ~KCL_TYPE_BIT(COL_TYPE_WEAK_WALL))
STATIC_ASSERT(KCL_TYPE_NON_DIRECTIONAL == 0xE0F8BDFF);

/// @brief Represents the collision types that the kart's body can collide with
/// @details Value: 0xEAFABDFF
#define KCL_TYPE_BODY_COLLIDABLE \
    (KCL_TYPE_VEHICLE_COLLIDEABLE | KCL_TYPE_BIT(COL_TYPE_CANNON_TRIGGER))
STATIC_ASSERT(KCL_TYPE_BODY_COLLIDABLE == 0xEAFABDFF);

/// @brief Represents KCL types for which @ref Kart::KartReject can apply a rejection
/// @details Value: 0xB0E82DFF
#define KCL_TYPE_REJECTABLE \
    (KCL_TYPE_DRIVER_FLOOR | KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL) | \
            KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL) | KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL2))
STATIC_ASSERT(KCL_TYPE_REJECTABLE == 0xB0E82DFF);

/// @brief Represents a tangible surface that objects can interact with
/// @details Value: 0x60E8DFFF
#define KCL_TYPE_OBJECT_SOLID_SURFACE \
    (KCL_TYPE_FLOOR | KCL_TYPE_BIT(COL_TYPE_WALL) | KCL_TYPE_BIT(COL_TYPE_ITEM_WALL) | \
            KCL_TYPE_BIT(COL_TYPE_WALL_2) | KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL))
STATIC_ASSERT(KCL_TYPE_OBJECT_SOLID_SURFACE == 0x60E8DFFF);

/// @brief Represents all KCL types for which objects can collide with
/// @details Value: 0x64EBDFFF
#define KCL_TYPE_OBJECT_COLLIDABLE \
    (KCL_TYPE_OBJECT_SOLID_SURFACE | KCL_TYPE_BIT(COL_TYPE_EFFECT_TRIGGER) | \
            KCL_TYPE_BIT(COL_TYPE_CANNON_TRIGGER) | KCL_TYPE_BIT(COL_TYPE_FALL_BOUNDARY))
STATIC_ASSERT(KCL_TYPE_OBJECT_COLLIDABLE == 0x64EBDFFF);

/// @brief Represents all KCL types for which objects can collide with or modify item state
/// @details Value: 0x6CEBDFFF
#define KCL_TYPE_OBJECT_COLLIDABLE_MODIFIABLE \
    (KCL_TYPE_OBJECT_COLLIDABLE | KCL_TYPE_BIT(COL_TYPE_ITEM_STATE_MODIFIER))
STATIC_ASSERT(KCL_TYPE_OBJECT_COLLIDABLE_MODIFIABLE == 0x6CEBDFFF);

/// @brief Represents both #COL_TYPE_INVISIBLE_WALL and #COL_TYPE_INVISIBLE_WALL2
/// @details Value: 0x80002000
#define KCL_TYPE_INVISIBLE_WALL \
    (KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL) | KCL_TYPE_BIT(COL_TYPE_INVISIBLE_WALL2))
STATIC_ASSERT(KCL_TYPE_INVISIBLE_WALL == 0x80002000);

/// @brief Represents all types of invisible walls (#COL_TYPE_INVISIBLE_WALL,
/// #COL_TYPE_INVISIBLE_WALL2, and #COL_TYPE_HALFPIPE_INVISIBLE_WALL)
/// @details Value: 0x90002000
#define KCL_TYPE_ANY_INVISIBLE_WALL \
    (KCL_TYPE_INVISIBLE_WALL | KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL))
STATIC_ASSERT(KCL_TYPE_ANY_INVISIBLE_WALL == 0x90002000);

namespace Kinoko::Field {

/// @brief The header of the KCL file format. It is 0x3C bytes long (for Mario %Kart Wii).
struct KColHeader {
    u32 pos_data_offset;        ///< 0-indexed offset to array of position vectors.
    u32 nrm_data_offset;        ///< 0-indexed offset to array of normal vectors.
    u32 prism_data_offset;      ///< 1-indexed offset to array of normal vectors.
    u32 block_data_offset;      ///< Offset to collision octree blocks
    f32 prism_thickness;        ///< Depth of a triangular prism along its normal vector.
    EGG::Vector3f area_min_pos; ///< Smallest possible coordinate in the model
    u32 area_x_width_mask;      ///< The x dimension of the octree's bounding box. @see searchBlock.
    u32 area_y_width_mask;      ///< The y dimension of the octree's bounding box. @see searchBlock.
    u32 area_z_width_mask;      ///< The z dimension of the octree's bounding box. @see searchBlock.
    u32 block_width_shift;      ///< Used to initialize octree navigation. @see searchBlock.
    u32 area_x_blocks_shift;    ///< Used to initialize octree navigation. @see searchBlock.
    u32 area_xy_blocks_shift;   ///< Used to initialize octree navigation. @see searchBlock.
    f32 sphere_radius;          ///< Clamps the sphere we check collision against. @see searchBlock.
};
STATIC_ASSERT(sizeof(KColHeader) == 0x3c);

/// @brief A bitmask representing the @ref KColType of collision surfaces.
typedef u32 KCLTypeMask;

} // namespace Kinoko::Field
