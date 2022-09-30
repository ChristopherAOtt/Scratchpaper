#pragma once

#include "Types.hpp"
#include "Primitives.hpp"
#include "MathUtils.hpp"
#include "Geometry.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>

/*
This system is an overwrought version of the idea presented here:
	https://gameprogrammingpatterns.com/type-object.html



*/

//-----------------------------------------------
// Helper structs
//-----------------------------------------------
struct WorldEntity{
	FVec3 pos;
	Basis basis;
	EntityHandle handle;

	Int32 padding[2];
};
static_assert(sizeof(WorldEntity) == 64);

struct GridEntity{
	ICuboid bounds;
	EntityHandle handle;
};
static_assert(sizeof(GridEntity) == 32);

// NOTE: PropertyId has a 1:1 relationship with the name string.
// This is just to avoid large arrays of strings everywhere.
// The EntityManager tracks this information.
typedef Int64 PropertyId;
constexpr PropertyId INVALID_PROPERTY_ID = -1;
struct EntityProperty{
	PropertyId id;
	PODVariant value;
};

//-----------------------------------------------
// EntityManager
//-----------------------------------------------
class EntityManager{
	public:
		EntityManager();
		~EntityManager();

	private:
		MathUtils::Random::SebVignaSplitmix64 m_id_generator;
		std::unordered_map<EntityTypeIndex, std::unordered_set<EntityId>> m_active_ids_by_type;
};
