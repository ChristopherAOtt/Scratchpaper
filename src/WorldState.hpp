#pragma once

#include <string>
#include <vector>

#include "Constants.hpp"
#include "Primitives.hpp"
#include "Geometry.hpp"
#include "GridEntities.hpp"
#include "Chunks.hpp"
#include "Widgets.hpp"
#include "EntityStructs.hpp"
#include "MathUtils.hpp"

//--------------------------------------------------------------------------------------------------
// World-Related Structs
//--------------------------------------------------------------------------------------------------
struct WidgetGroup{
	/*
	Group of widgets to be rendered as a single mesh. Useful for
	debugging.
	*/

	std::string name;
	std::vector<Widget> widgets;
	Fingerprint fingerprint;  // Changes every time the widgets change
};

struct Portal{
	float radius;
	FVec3 locations[2];
};

//--------------------------------------------------------------------------------------------------
// World State
//--------------------------------------------------------------------------------------------------
class WorldState{
	/*
	Blob of state representing the world. Anything that can be saved/loaded from a file should
	go here. This + player inputs should contain all the data needed to run a deterministic 
	simulation of the world.

	YAGNI: More efficient structures.
	*/

	public:
		WorldState();
		~WorldState();

		const WidgetGroup& groupData(std::string name) const;
		std::vector<std::string> allGroupNames() const;
		void addWidgetData(std::string name, const std::vector<Widget>& widgets, 
			bool should_append);
		
		//---------------------------------------
		// Other
		//---------------------------------------
		// Time
		float currentTime() const;
		void addDeltaT(double t);
	
	public:
		std::string m_name;

		//PlaceholderEntity m_viewer_entity;
		JankyPlaceholderEntityFactory m_jank_entity_factory;
		Uint64 m_viewer_entity_index;
		std::vector<PlaceholderEntity> m_placeholder_entities;
		
		Portal m_temp_portal;
		
		FVec3 m_fog_color;
		GridEntityTable m_grid_entity_table;
		ChunkTable m_chunk_table;

	private:
		Fingerprint generateNextFingerprint();
		
	private:
		double m_current_time;
		
		std::unordered_map<std::string, WidgetGroup> m_widget_groups;
		Fingerprint m_next_available_fingerprint;
};
