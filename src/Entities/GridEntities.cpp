#include "GridEntities.hpp"

GridEntityTable::GridEntityTable(){
	m_next_available_id = 1;  // 
	initSharedEntityInfoTable();
}

GridEntityTable::~GridEntityTable(){

}

EntityIdOld GridEntityTable::addEntity(GridEntityData data, IVec3 occupied_voxels){
	GridEntityRecord new_record = {data, occupied_voxels};
	EntityIdOld new_id = m_next_available_id;
	++m_next_available_id;
	
	m_records[new_id] = new_record;

	return m_next_available_id;
}

bool GridEntityTable::deleteEntity(EntityIdOld entity_id){
	bool had_record = m_records.find(entity_id) != m_records.end();
	m_records.erase(entity_id);
	return had_record;
}

void GridEntityTable::initSharedEntityInfoTable(){
	/*
	REFACTOR: Figure out how to do this more elegantly.
	TODO: Once entity types are added fill in the relevant info here.
	YAGNI: Consider loading these values from a config file rather than having it hardcoded.
	*/	

	// These two are only here to keep the indexing consistent.
	m_shared_info[GENT_INVALID] = {};
	m_shared_info[GENT_OTHER] = {};
	m_shared_info[GENT_ITEM_PIPE] = {};
	m_shared_info[GENT_FLUID_PIPE] = {};
	m_shared_info[GENT_ASSEMBLER] = {};
	m_shared_info[GENT_STORAGE_BLOCK] = {};
}
