#pragma once

#include "Primitives.hpp"
#include "Geometry.hpp"

#include <unordered_map>

//-------------------------------------------------------------------------------------------------
// Entity specific structs
//-------------------------------------------------------------------------------------------------
typedef Bytes8 EntityIdOld;
enum class EntityType{
	ENTITY_PLAYER,
	ENTITY_DRONE
};

enum class PlayerMode{
	MODE_SURVIVAL,  // Normal gameplay.
	MODE_NOCLIP,    // Unrestrained by gravity or physics. Fly around by moving in look direction.
	MODE_GODMODE,   // Noclip, can't take damage, infinite resources, instakill.
};

struct Player{
	int player_id;
	PlayerMode player_mode;
	
	// WARNING: Assumes that the player is just a floating head.
	// 		This is a hack to speed the developement of the rendering system.
	// TODO: Remove once rendering is working.
	// TODO: Add body animation tree once body rendering is added.
	Basis basis;  // Assumes the Y-axis is the look direction.
};

struct Drone{
	float fuel;
};

struct EntityData{
	EntityType type;
	union{
		Player player;
		Drone drone;
	};
};

struct Entity{
	// Data common to all entities
	FVec3 pos;
	FVec3 velocity;
	float mass;
	int curr_health;
	int max_health;

	// Variable data that changes based on type
	EntityData data;
};


//-------------------------------------------------------------------------------------------------
// Entity related structs
//-------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Unit Orders
//-----------------------------------------------------------------------------
enum UnitOrderType{
	ORDER_INVALID = 0, 

	ORDER_TRANSLATE_CROUCH_WALK,
	ORDER_TRANSLATE_WALK,
	ORDER_TRANSLATE_RUN,
	ORDER_ROTATE_ELEMENTWISE,
	ORDER_LOOK_AT,
	ORDER_JUMP,
	ORDER_SHOOT,  // For the moment, just shoots from the look direction
	ORDER_INTERACT
};

//-------------------------------------
// Translation Order
// TODO: Consider rolling the different translation types into the TranslationOrder instead
//-------------------------------------
struct TranslationOrder{
	/*
	Used for ORDER_TRANSLATE_CROUCH_WALK, ORDER_TRANSLATE_WALK, ORDER_TRANSLATE_RUN
	*/

	FVec3 relative_amounts;

	bool operator==(const TranslationOrder& other) const;
};

//-------------------------------------
// Rotation Order
//-------------------------------------
struct RotationOrder{
	FVec2 relative_amounts;  // Horizontal, Vertical

	bool operator==(const RotationOrder& other) const;
};

//-------------------------------------
// Look Order
//-------------------------------------
enum LookOrderType{
	LOOK_TRACK_POINT,   // Look at point, then keep tracking it
	LOOK_TRACK_ENTITY,  // Look at entity, then keep tracking it
};

struct LookOrder{
	LookOrderType type;
	union{
		FVec3 target_point;
		EntityIdOld target_entity;
	};

	bool operator==(const LookOrder& other) const;
};

//-------------------------------------
// Jump Order
//-------------------------------------
enum JumpOrderType{
	JUMP_VERTICAL,
	JUMP_TO_POINT_LINEAR,
	JUMP_TO_POINT_PARABOLIC
};

struct JumpOrder{
	JumpOrderType type;
	float parabola_height;  // Only defined if the type is JUMP_TO_POINT_PARABOLIC

	bool operator==(const JumpOrder& other) const;
};

//-------------------------------------
// Package for the different order types
//-------------------------------------
struct UnitOrder{
	/*
	Sent from an AI or human decision maker to control a unit in the world.
	Packages order type with various different information structs. Trivial order types
	are types that do not need any additional information (no corresponding struct).
	*/

	UnitOrderType type;
	union{
		LookOrder look_order;
		RotationOrder rotation_order;
		TranslationOrder translation_order;
		JumpOrder jump_order;
	};

	bool operator==(const UnitOrder& other) const;
};

//static_assert(std::has_unique_object_representations<UnitOrder>::value, 
//	"UnitOrder must be compact");
static_assert(std::is_pod<UnitOrder>::value,       "UnitOrder must be POD");

//-----------------------------------------------------------------------------
// Unit data
//-----------------------------------------------------------------------------
struct UnitPhysics{
	FVec3 position;
	Basis basis;
	FCuboid bounding_box;  // Volume defined in the unit's object space.
	bool is_on_ground;  // TOOD: Find a better solution
};

struct Unit{
	/*
	Bundle of data serving as the "base class" of a unit. 
	NOTE: At some point I want most of these values based on the unit's
		type, and have only instance-specific data in here. For now this 
		is simpler.
	*/

	PODString name;
	UnitPhysics physics;
	FVec3 look_dir;
	int health_max;
	int health_current;
};


//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------

class EntityTable{
	public:
		EntityTable();
		~EntityTable();

	private:
		std::unordered_map<EntityIdOld, Entity> m_entities;

};
