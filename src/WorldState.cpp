#include "WorldState.hpp"


//-------------------------------------------------------------------------------------------------
// WorldState
//-------------------------------------------------------------------------------------------------
WorldState::WorldState(){
	// Init simple member variables
	m_current_time = 0.0f;
	m_fog_color = {0.2f, 0.2f, 0.95f};
	m_next_available_fingerprint = 1;
}

WorldState::~WorldState(){

}

const WidgetGroup& WorldState::groupData(std::string name) const{
	auto iter = m_widget_groups.find(name);
	assert(iter != m_widget_groups.end());
	return iter->second;
}

std::vector<std::string> WorldState::allGroupNames() const{
	std::vector<std::string> group_names;
	group_names.reserve(m_widget_groups.size());
	for(auto [key, value] : m_widget_groups){
		group_names.push_back(key);	
	}

	return group_names;
}

void WorldState::addWidgetData(std::string name, const std::vector<Widget>& widgets, 
	bool should_append){
	/*
	Checks if the given widget group already exists. If not, it creates the group
	and adds the widget vector. If the group exists and should_append is False, the
	data is overwritten. In either case, the fingerprint is updated.
	*/
	
	constexpr Int32 ARBITRARY_MAX_WIDGET_GROUP_SIZE = 50000;
	assert(m_widget_groups.size() < ARBITRARY_MAX_WIDGET_GROUP_SIZE);

	WidgetGroup& group = m_widget_groups[name];
	group.fingerprint = generateNextFingerprint();
	group.name = name;
	if(should_append){
		group.widgets.insert(group.widgets.end(), 
			widgets.begin(), widgets.end());
	}else{
		group.widgets = widgets;
	}
}

float WorldState::currentTime() const{
	return m_current_time;
}

void WorldState::addDeltaT(double t){
	m_current_time += t;
}

Fingerprint WorldState::generateNextFingerprint(){
	/*
	Returns a unique value that will change only when a group is updated. This
	allows other systems to quickly check if state has changed.

	TODO: Better strategy than this
	*/

	return m_next_available_fingerprint++;
}
