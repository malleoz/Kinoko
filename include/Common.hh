/// @file Common.hh
/// @brief This header houses common data types such as our integral types and enums.

#pragma once

#include <Types.hh>

#include <egg/core/Heap.hh>

#include <array>
#include <bit>
#include <limits>
#include <utility>

namespace Kinoko {

/// @brief Maps between a course and its internal ID
enum class Course {
    Mario_Circuit = 0,           ///< Mario Circuit
    Moo_Moo_Meadows = 1,         ///< Moo Moo Meadows
    Mushroom_Gorge = 2,          ///< Mushroom Gorge
    Grumble_Volcano = 3,         ///< Grumble Volcano
    Toads_Factory = 4,           ///< Toad's Factory
    Coconut_Mall = 5,            ///< Coconut Mall
    DK_Summit = 6,               ///< DK Summit
    Wario_Gold_Mine = 7,         ///< Wario's Gold Mine
    Luigi_Circuit = 8,           ///< Luigi Circuit
    Daisy_Circuit = 9,           ///< Daisy Circuit
    Moonview_Highway = 10,       ///< Moonview Highway
    Maple_Treeway = 11,          ///< Maple Treeway
    Bowsers_Castle = 12,         ///< Bowser's Castle
    Rainbow_Road = 13,           ///< Rainbow Road
    Dry_Dry_Ruins = 14,          ///< Dry Dry Ruins
    Koopa_Cape = 15,             ///< Koopa Cape
    GCN_Peach_Beach = 16,        ///< GCN Peach Beach
    GCN_Mario_Circuit = 17,      ///< GCN Mario Circuit
    GCN_Waluigi_Stadium = 18,    ///< GCN Waluigi Stadium
    GCN_DK_Mountain = 19,        ///< GCN DK Mountain
    DS_Yoshi_Falls = 20,         ///< DS Yoshi Falls
    DS_Desert_Hills = 21,        ///< DS Desert Hills
    DS_Peach_Gardens = 22,       ///< DS Peach Gardens
    DS_Delfino_Square = 23,      ///< DS Delfino Square
    SNES_Mario_Circuit_3 = 24,   ///< SNES Mario Circuit 3
    SNES_Ghost_Valley_2 = 25,    ///< SNES Ghost Valley 2
    N64_Mario_Raceway = 26,      ///< N64 Mario Raceway
    N64_Sherbet_Land = 27,       ///< N64 Sherbet Land
    N64_Bowsers_Castle = 28,     ///< N64 Bowser's Castle
    N64_DKs_Jungle_Parkway = 29, ///< N64 DK's Jungle Parkway
    GBA_Bowser_Castle_3 = 30,    ///< GBA Bowser Castle 3
    GBA_Shy_Guy_Beach = 31,      ///< GBA Shy Guy Beach
    Delfino_Pier = 32,           ///< Delfino Pier
    Block_Plaza = 33,            ///< Block Plaza
    Chain_Chomp_Roulette = 34,   ///< Chain Chomp Roulette
    Funky_Stadium = 35,          ///< Funky Stadium
    Thwomp_Desert = 36,          ///< Thwomp Desert
    GCN_Cookie_Land = 37,        ///< GCN Cookie Land
    DS_Twilight_House = 38,      ///< DS Twilight House
    SNES_Battle_Course_4 = 39,   ///< SNES Battle Course 4
    GBA_Battle_Course_3 = 40,    ///< GBA Battle Course 3
    N64_Skyscraper = 41,         ///< N64 Skyscraper
    Galaxy_Colosseum = 54,       ///< Galaxy Colosseum
    Win_Demo = 55,               ///< Win Demo
    Lose_Demo = 56,              ///< Lose Demo
    Draw_Demo = 57,              ///< Draw Demo
    Ending_Demo = 58,            ///< Ending Demo
};

/// @brief Maps between a vehicle and its internal ID
enum class Vehicle {
    Standard_Kart_S = 0,  ///< Standard Kart S (light weight class)
    Standard_Kart_M = 1,  ///< Standard Kart M (medium weight class)
    Standard_Kart_L = 2,  ///< Standard Kart L (heavy weight class)
    Baby_Booster = 3,     ///< Baby Booster
    Classic_Dragster = 4, ///< Classic Dragster
    Offroader = 5,        ///< Offroader
    Mini_Beast = 6,       ///< Mini Beast
    Wild_Wing = 7,        ///< Wild Wing
    Flame_Flyer = 8,      ///< Flame Flyer
    Cheep_Charger = 9,    ///< Cheep Charger
    Super_Blooper = 10,   ///< Super Blooper
    Piranha_Prowler = 11, ///< Piranha Prowler
    Tiny_Titan = 12,      ///< Tiny Titan
    Daytripper = 13,      ///< Daytripper
    Jetsetter = 14,       ///< Jetsetter
    Blue_Falcon = 15,     ///< Blue Falcon
    Sprinter = 16,        ///< Sprinter
    Honeycoupe = 17,      ///< Honeycoupe
    Standard_Bike_S = 18, ///< Standard Bike S
    Standard_Bike_M = 19, ///< Standard Bike M
    Standard_Bike_L = 20, ///< Standard Bike L
    Bullet_Bike = 21,     ///< Bullet Bike
    Mach_Bike = 22,       ///< Mach Bike
    Flame_Runner = 23,    ///< Flame Runner
    Bit_Bike = 24,        ///< Bit Bike
    Sugarscoot = 25,      ///< Sugarscoot
    Wario_Bike = 26,      ///< Wario Bike
    Quacker = 27,         ///< Quacker
    Zip_Zip = 28,         ///< Zip Zip
    Shooting_Star = 29,   ///< Shooting Star
    Magikruiser = 30,     ///< Magikruiser
    Sneakster = 31,       ///< Sneakster
    Spear = 32,           ///< Spear
    Jet_Bubble = 33,      ///< Jet Bubble
    Dolphin_Dasher = 34,  ///< Dolphin Dasher
    Phantom = 35,         ///< Phantom
    Max = 36,             ///< The total number of vehicles
};

/// @brief Maps between a character and its internal ID
enum class Character {
    Mario = 0,                       ///< Mario
    Baby_Peach = 1,                  ///< Baby Peach
    Waluigi = 2,                     ///< Waluigi
    Bowser = 3,                      ///< Bowser
    Baby_Daisy = 4,                  ///< Baby Daisy
    Dry_Bones = 5,                   ///< Dry Bones
    Baby_Mario = 6,                  ///< Baby Mario
    Luigi = 7,                       ///< Luigi
    Toad = 8,                        ///< Toad
    Donkey_Kong = 9,                 ///< Donkey Kong
    Yoshi = 10,                      ///< Yoshi
    Wario = 11,                      ///< Wario
    Baby_Luigi = 12,                 ///< Baby Luigi
    Toadette = 13,                   ///< Toadette
    Koopa_Troopa = 14,               ///< Koopa Troopa
    Daisy = 15,                      ///< Daisy
    Peach = 16,                      ///< Peach
    Birdo = 17,                      ///< Birdo
    Diddy_Kong = 18,                 ///< Diddy Kong
    King_Boo = 19,                   ///< King Boo
    Bowser_Jr = 20,                  ///< Bowser Jr
    Dry_Bowser = 21,                 ///< Dry Bowser
    Funky_Kong = 22,                 ///< Funky Kong
    Rosalina = 23,                   ///< Rosalina
    Small_Mii_Outfit_A_Male = 24,    ///< Small Mii Outfit A Male
    Small_Mii_Outfit_A_Female = 25,  ///< Small Mii Outfit A Female
    Small_Mii_Outfit_B_Male = 26,    ///< Small Mii Outfit B Male
    Small_Mii_Outfit_B_Female = 27,  ///< Small Mii Outfit B Female
    Small_Mii_Outfit_C_Male = 28,    ///< Small Mii Outfit C Male
    Small_Mii_Outfit_C_Female = 29,  ///< Small Mii Outfit C Female
    Medium_Mii_Outfit_A_Male = 30,   ///< Medium Mii Outfit A Male
    Medium_Mii_Outfit_A_Female = 31, ///< Medium Mii Outfit A Female
    Medium_Mii_Outfit_B_Male = 32,   ///< Medium Mii Outfit B Male
    Medium_Mii_Outfit_B_Female = 33, ///< Medium Mii Outfit B Female
    Medium_Mii_Outfit_C_Male = 34,   ///< Medium Mii Outfit C Male
    Medium_Mii_Outfit_C_Female = 35, ///< Medium Mii Outfit C Female
    Large_Mii_Outfit_A_Male = 36,    ///< Large Mii Outfit A Male
    Large_Mii_Outfit_A_Female = 37,  ///< Large Mii Outfit A Female
    Large_Mii_Outfit_B_Male = 38,    ///< Large Mii Outfit B Male
    Large_Mii_Outfit_B_Female = 39,  ///< Large Mii Outfit B Female
    Large_Mii_Outfit_C_Male = 40,    ///< Large Mii Outfit C Male
    Large_Mii_Outfit_C_Female = 41,  ///< Large Mii Outfit C Female
    Medium_Mii = 42,                 ///< Medium Mii
    Small_Mii = 43,                  ///< Small Mii
    Large_Mii = 44,                  ///< Large Mii
    Peach_Biker_Outfit = 45,         ///< Peach Biker Outfit
    Daisy_Biker_Outfit = 46,         ///< Daisy Biker Outfit
    Rosalina_Biker_Outfit = 47,      ///< Rosalina Biker Outfit
    Max = 48,                        ///< The total number of characters
};

/// @brief Represents the weight class of a character or vehicle
enum class WeightClass {
    Invalid = -1, ///< Represents an invalid or uninitialized weight class
    Light = 0,    ///< Represents the light weight class
    Medium = 1,   ///< Represents the medium weight class
    Heavy = 2,    ///< Represents the heavy weight class
};

/// @brief Unique identifier to better categorize regions of the game's heap allocation
enum class GroupID : u16 {
    None = 0,   ///< Represents no specific group or an uninitialized group
    Race = 1,   ///< Pertains to @ref System::RaceManager allocation
    Gfx = 2,    ///< Pertains to graphics-related allocation
    Kart = 3,   ///< Pertains to kart-related allocation
    Object = 4, ///< Pertains to object and rail-related allocation
    Course = 5, ///< Pertains to course data allocation
    UI = 6,
    Effect = 7,
    Sound = 8,
    Resource = 10, ///< Pertains to allocations for loading file archives
    HomeMenu = 11,
    Item = 12, ///< Pertains to allocations for item inventory management
    Net = 13,
};

/// @brief Maps between a character and its corresponding weight class
/// @return The weight class of the specified character
static constexpr WeightClass CharacterToWeight(Character character) {
    switch (character) {
    case Character::Baby_Peach:
    case Character::Baby_Daisy... Character::Baby_Mario:
    case Character::Toad:
    case Character::Baby_Luigi... Character::Koopa_Troopa:
    case Character::Small_Mii_Outfit_A_Male... Character::Small_Mii_Outfit_C_Female:
    case Character::Small_Mii:
        return WeightClass::Light;
    case Character::Mario:
    case Character::Luigi:
    case Character::Yoshi:
    case Character::Daisy... Character::Diddy_Kong:
    case Character::Bowser_Jr:
    case Character::Medium_Mii_Outfit_A_Male... Character::Medium_Mii_Outfit_C_Female:
    case Character::Medium_Mii:
    case Character::Peach_Biker_Outfit:
    case Character::Daisy_Biker_Outfit:
        return WeightClass::Medium;
    case Character::Waluigi:
    case Character::Bowser:
    case Character::Donkey_Kong:
    case Character::Wario:
    case Character::King_Boo:
    case Character::Dry_Bowser... Character::Rosalina:
    case Character::Large_Mii_Outfit_A_Male... Character::Large_Mii_Outfit_C_Female:
    case Character::Large_Mii:
    case Character::Rosalina_Biker_Outfit:
        return WeightClass::Heavy;
    default:
        return WeightClass::Invalid;
    }
}

/// @brief Maps between a vehicle and its corresponding weight class
/// @return The weight class of the specified vehicle
static constexpr WeightClass VehicleToWeight(Vehicle vehicle) {
    switch (vehicle) {
    case Vehicle::Standard_Kart_S:
    case Vehicle::Baby_Booster:
    case Vehicle::Mini_Beast:
    case Vehicle::Cheep_Charger:
    case Vehicle::Tiny_Titan:
    case Vehicle::Blue_Falcon:
    case Vehicle::Standard_Bike_S:
    case Vehicle::Bullet_Bike:
    case Vehicle::Bit_Bike:
    case Vehicle::Quacker:
    case Vehicle::Magikruiser:
    case Vehicle::Jet_Bubble:
        return WeightClass::Light;
    case Vehicle::Standard_Kart_M:
    case Vehicle::Classic_Dragster:
    case Vehicle::Wild_Wing:
    case Vehicle::Super_Blooper:
    case Vehicle::Daytripper:
    case Vehicle::Sprinter:
    case Vehicle::Standard_Bike_M:
    case Vehicle::Mach_Bike:
    case Vehicle::Sugarscoot:
    case Vehicle::Zip_Zip:
    case Vehicle::Sneakster:
    case Vehicle::Dolphin_Dasher:
        return WeightClass::Medium;
    case Vehicle::Standard_Kart_L:
    case Vehicle::Offroader:
    case Vehicle::Flame_Flyer:
    case Vehicle::Piranha_Prowler:
    case Vehicle::Jetsetter:
    case Vehicle::Honeycoupe:
    case Vehicle::Standard_Bike_L:
    case Vehicle::Flame_Runner:
    case Vehicle::Wario_Bike:
    case Vehicle::Shooting_Star:
    case Vehicle::Spear:
    case Vehicle::Phantom:
        return WeightClass::Heavy;
    default:
        return WeightClass::Invalid;
    }
}

/// @brief Maps between a course ID and its internal SZS filename
static constexpr const char *COURSE_NAMES[59] = {
        "castle_course",
        "farm_course",
        "kinoko_course",
        "volcano_course",
        "factory_course",
        "shopping_course",
        "boardcross_course",
        "truck_course",
        "beginner_course",
        "senior_course",
        "ridgehighway_course",
        "treehouse_course",
        "koopa_course",
        "rainbow_course",
        "desert_course",
        "water_course",
        "old_peach_gc",
        "old_mario_gc",
        "old_waluigi_gc",
        "old_donkey_gc",
        "old_falls_ds",
        "old_desert_ds",
        "old_garden_ds",
        "old_town_ds",
        "old_mario_sfc",
        "old_obake_sfc",
        "old_mario_64",
        "old_sherbet_64",
        "old_koopa_64",
        "old_donkey_64",
        "old_koopa_gba",
        "old_heyho_gba",
        "venice_battle",
        "block_battle",
        "casino_battle",
        "skate_battle",
        "sand_battle",
        "old_CookieLand_gc",
        "old_House_ds",
        "old_battle4_sfc",
        "old_battle3_gba",
        "old_matenro_64",
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        "ring_mission",
        "winningrun_demo",
        "loser_demo",
        "draw_dmeo",
        "ending_demo",
};

/// @brief Maps between a vehicle ID and its internal BSP filename
static constexpr const char *VEHICLE_NAMES[36] = {
        "sdf_kart",
        "mdf_kart",
        "ldf_kart",
        "sa_kart",
        "ma_kart",
        "la_kart",
        "sb_kart",
        "mb_kart",
        "lb_kart",
        "sc_kart",
        "mc_kart",
        "lc_kart",
        "sd_kart",
        "md_kart",
        "ld_kart",
        "se_kart",
        "me_kart",
        "le_kart",
        "sdf_bike",
        "mdf_bike",
        "ldf_bike",
        "sa_bike",
        "ma_bike",
        "la_bike",
        "sb_bike",
        "mb_bike",
        "lb_bike",
        "sc_bike",
        "mc_bike",
        "lc_bike",
        "sd_bike",
        "md_bike",
        "ld_bike",
        "se_bike",
        "me_bike",
        "le_bike",
};

/// @brief Compile-time assertion that checks the machine's floating-point epsilon value is
/// equivalent to the value expected by the base game
STATIC_ASSERT(std::numeric_limits<f32>::epsilon() == 1.0f / 8388608.0f);

/// @brief Compile-time assertion that checks the machine's native endianness is either big or
/// little
STATIC_ASSERT(
        std::endian::native == std::endian::big || std::endian::native == std::endian::little);

/// @brief Helper template which uses function overloading and implicit up-casting to determine
/// whether or not a class is derived from a templated base class (i.e. MapdataPointInfoAccessor
/// derives from MapdataAccessorBase). See: https://en.cppreference.com/w/cpp/language/sfinae
template <template <typename...> class Base, typename Derived>
struct is_derived_from_template {
private:
    /// @brief Overload which returns true when the template type is derived from the base class
    template <typename... Ts>
    static std::true_type test(const Base<Ts...> *);

    /// @brief Overload which returns false when the template type is not derived from the base
    /// class
    static std::false_type test(...);

public:
    /// @brief Indicates whether the Derived class is derived from the templated Base class
    static constexpr bool value = decltype(test(std::declval<Derived *>()))::value;
};

/// @brief Represents whether a class is derived from a templated base class
/// @tparam Derived The class to check if it is derived from the templated base class
template <template <typename...> class Base, typename Derived>
inline constexpr bool is_derived_from_template_v = is_derived_from_template<Base, Derived>::value;

template <typename T>
concept IntegralType = std::is_integral_v<T>;

/// @brief Concept that checks if a type is parseable (integral or floating-point)
/// @tparam T The type to check if it is parseable
/// @details When we say "parseable", we mean that the type can be passed into @ref parse() to
/// handle potential endianness differences between the Kinoko user's OS and the game files.
template <typename T>
concept ParseableType = std::is_integral_v<T> ||
        (std::is_floating_point_v<T> && (sizeof(T) == 4 || sizeof(T) == 8));

/// @brief Consistent file parsing with byte-swappable values
template <ParseableType T>
static inline constexpr T parse(T val, std::endian endian = std::endian::big) {
    if constexpr (std::is_integral_v<T>) {
        return endian == std::endian::native ? val : std::byteswap(val);
    } else {
        if constexpr (sizeof(T) == 4) {
            return std::bit_cast<T>(parse<u32>(std::bit_cast<u32>(val), endian));
        } else {
            return std::bit_cast<T>(parse<u64>(std::bit_cast<u64>(val), endian));
        }
    }
}

/// @brief Helper function to allow hex representation of f32 by bitcasting to u32
static inline constexpr u32 f2u(f32 val) {
    return std::bit_cast<u32>(val);
}

/// @brief The maximum number of players in a race
static constexpr size_t MAX_PLAYERS = 12;

/// @brief The size of memory blocks that are allocated for game heap space.
static constexpr size_t MEMORY_SPACE_SIZE = 0x1000000;

#ifdef BUILD_DEBUG
static constexpr auto DEFAULT_OPT = Abstract::Memory::MEMiHeapHead::OptFlag().setBit(
        Abstract::Memory::MEMiHeapHead::eOptFlag::DebugFillAlloc);
#else
static constexpr auto DEFAULT_OPT = Abstract::Memory::MEMiHeapHead::OptFlag().setBit(
        Abstract::Memory::MEMiHeapHead::eOptFlag::ZeroFillAlloc);
#endif

} // namespace Kinoko
