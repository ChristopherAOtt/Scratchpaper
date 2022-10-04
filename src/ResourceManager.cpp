#include "ResourceManager.hpp"

//-------------------------------------------------------------------------------------------------
// ResourceManager
//-------------------------------------------------------------------------------------------------
//-----------------------------------------------
// Public
//-----------------------------------------------
ResourceManager::ResourceManager(){
	constexpr Uint64 ARBITRARY_STARTING_SEED = 525600;
	m_id_generator.state = ARBITRARY_STARTING_SEED;
}

ResourceManager::~ResourceManager(){

}

ResourceHandle ResourceManager::handleByName(std::string name) const{
	/*
	YAGNI: Another map to make this constant time. 
	*/

	for(const auto& [handle, metadata] : m_metadata_map){
		if(metadata.name == name){
			return metadata.handle;
		}
	}

	return ResourceManager::INVALID_RESOURCE_HANDLE;
}

ResourceHandle ResourceManager::addResource(std::string name, TriangleMesh data){
	ResourceId new_id = generateNewId();
	ResourceHandle new_handle = {.type=RESOURCE_RIGID_TRIANGLE_MESH, .id=new_id};
	m_metadata_map[new_handle] = {
		.handle=new_handle,
		.name=name
	};

	m_rigid_triangle_mesh_map[new_id] = data;
	return new_handle;
}

ResourceHandle ResourceManager::addResource(std::string name, VoxelKDTree::TreeData data){
	ResourceId new_id = generateNewId();
	ResourceHandle new_handle = {.type=RESOURCE_MKDTREE, .id=new_id};
	m_metadata_map[new_handle] = {
		.handle=new_handle,
		.name=name
	};

	m_mkdtree_map[new_id] = data;
	return new_handle;
}

ResourceHandle ResourceManager::addResource(std::string name, TempTexture data){
	ResourceId new_id = generateNewId();
	ResourceHandle new_handle = {.type=RESOURCE_TEXTURE, .id=new_id};
	m_metadata_map[new_handle] = {
		.handle=new_handle,
		.name=name
	};

	m_texture_map[new_id] = data;
	return new_handle;
}

TriangleMesh ResourceManager::getDataTriangleMesh(ResourceHandle handle) const{
	auto iter = m_metadata_map.find(handle);
	assert(iter != m_metadata_map.end());

	auto iter2 = m_rigid_triangle_mesh_map.find(iter->second.handle.id);
	assert(iter2 != m_rigid_triangle_mesh_map.end());
	return iter2->second;
}

TempTexture ResourceManager::getDataTexture(ResourceHandle handle) const{
	auto iter = m_metadata_map.find(handle);
	assert(iter != m_metadata_map.end());

	auto iter2 = m_texture_map.find(iter->second.handle.id);
	assert(iter2 != m_texture_map.end());
	return iter2->second;
}


//-----------------------------------------------
// Private
//-----------------------------------------------
ResourceId ResourceManager::generateNewId(){
	/*

	*/
	assert(m_active_ids.size() <= ResourceManager::ARBITRARY_MAX_ACTIVE_IDS);

	ResourceId new_id;
	do{
		new_id = m_id_generator.next();
	}while(m_active_ids.count(new_id));
	
	m_active_ids.insert(new_id);
	return new_id;
}






































































































































































































#if 0
ResourceManager::ResourceTransform initDefaultTransform(){
	return {
		.is_valid=false,
		.position={0, 0, 0},
		.rotation={0, 0, 0},
		.scale   ={1, 1, 1},
	};
}

//-------------------------------------------------------------------------------------------------
// Resource Manager
//-------------------------------------------------------------------------------------------------
ResourceManager::ResourceManager(){
	m_next_available_id = 1;
}

ResourceManager::~ResourceManager(){

}

ResourceId ResourceManager::addWidgetGroup(std::vector<Widget> widget_group, std::string name){
	ResourceId new_id = initNewResourceMetadata(RESOURCE_WIDGET_GROUP, name);
	markAsUnprocessed(new_id);
	m_widget_groups[new_id] = widget_group;
	
	return new_id;
}

void ResourceManager::updateWidgetGroupData(ResourceId id, std::vector<Widget> widget_group){
	markAsUnprocessed(id);
	m_widget_groups[id] = widget_group;
}

ResourceId ResourceManager::addWidgetMesh(WidgetMesh widget_mesh, std::string name){
	ResourceId new_id = initNewResourceMetadata(RESOURCE_WIDGET_MESH, name);
	markAsUnprocessed(new_id);
	m_widget_meshes[new_id] = widget_mesh;
	
	return new_id;
}

void ResourceManager::updateWidgetMeshData(ResourceId id, WidgetMesh widget_mesh){
	markAsUnprocessed(id);
	m_widget_meshes[id] = widget_mesh;
}

const std::vector<Widget>& ResourceManager::widgetGroupReference(ResourceId id) const{
	const std::vector<Widget>& widgets_reference = m_widget_groups.find(id)->second;
	return widgets_reference;
}

const WidgetMesh& ResourceManager::widgetMeshReference(ResourceId id) const{
	const WidgetMesh& widget_mesh_ref = m_widget_meshes.find(id)->second;
	return widget_mesh_ref;
}

void ResourceManager::markAsProcessed(ResourceId id){
	m_unprocessed_ids.erase(id);
}

void ResourceManager::markAsUnprocessed(ResourceId id){
	m_unprocessed_ids.insert(id);
}

std::vector<ResourceId> ResourceManager::allUnprocessedResources() const{
	std::vector<ResourceId> output;
	output.reserve(m_unprocessed_ids.size());
	for(ResourceId id : m_unprocessed_ids){
		output.push_back(id);
	}

	return output;
}

std::vector<ResourceId> ResourceManager::allResourcesOfType(ResourceType type) const{
	/*
	Returns a list of all resources matching the given type.
	*/

	std::vector<ResourceId> matching_resources;
	for(auto iter : m_resource_metadata_map){
		if(iter.second.type == type){
			matching_resources.push_back(iter.first);
		}
	}

	return matching_resources;
}

std::string ResourceManager::getResourceName(ResourceId id) const{
	/*
	Given a resource id, return its name if defined. Otherwise return an empty string.
	TODO: Remove asserts once testing is done.
	*/

	auto iter = m_resource_metadata_map.find(id);
	assert(iter != m_resource_metadata_map.end());
	if(iter->second.is_name_string_defined){
		auto string_iter = m_resource_name_map.find(id);
		assert(string_iter != m_resource_name_map.end());
		return string_iter->second;
	}

	return "";
}

ResourceManager::ResourceType ResourceManager::getResourceType(ResourceId id) const{
	/*
	Given a resource id, return the type. 
	*/

	auto iter = m_resource_metadata_map.find(id);
	if(iter != m_resource_metadata_map.end()){
		return iter->second.type;
	}

	return RESOURCE_INVALID;
}

void ResourceManager::eraseResource(ResourceId id){

}

void ResourceManager::setTransformPosition(ResourceId id, FVec3 position){
	ResourceTransform& transform = getTransformStruct(id);
	transform.position = position;
}

void ResourceManager::setTransformRotation(ResourceId id, FVec3 rotation){
	ResourceTransform& transform = getTransformStruct(id);
	transform.rotation = rotation;
}

void ResourceManager::setTransformScale(ResourceId id, FVec3 scale){
	ResourceTransform& transform = getTransformStruct(id);
	transform.scale = scale;
}

void ResourceManager::eraseTransforms(ResourceId id){
	// Erase the actual transform
	auto iter = m_transform_map.find(id);
	if(iter != m_transform_map.end()){
		m_transform_map.erase(iter);
	}

	// Mark the transform as undefined for the resource
	auto iter_metadata = m_resource_metadata_map.find(id);
	assert(iter_metadata != m_resource_metadata_map.end());
	iter_metadata->second.is_transform_defined = false;
}

ResourceManager::ResourceTransform ResourceManager::resourceTransform(ResourceId id) const{
	auto iter = m_transform_map.find(id);
	if(iter != m_transform_map.end()){
		return iter->second;
	}
	
	return initDefaultTransform();
}

ResourceId ResourceManager::getNewResourceId(){
	/*
	Centralizes the Id generation process in case I decide to change it to something
	smarter than a constantly incrementing number. 
	*/

	ResourceId new_id = m_next_available_id;
	assertNotInUse(new_id);
	++m_next_available_id;

	return new_id;
}

ResourceManager::ResourceTransform& ResourceManager::getTransformStruct(ResourceId id){
	/*
	If the transform already exists, return a reference to it. If not, init it and then
	return a reference.
	*/

	auto iter = m_transform_map.find(id);
	if(iter != m_transform_map.end()){
		return iter->second;
	}else{
		// Update the metadata for this resource
		auto iter_metadata = m_resource_metadata_map.find(id);
		assert(iter_metadata != m_resource_metadata_map.end());
		iter_metadata->second.is_transform_defined = true;

		// Init and return the transform
		ResourceTransform transform = initDefaultTransform();
		transform.is_valid = true;
		m_transform_map[id] = transform;
		return m_transform_map[id];
	}
}

ResourceId ResourceManager::initNewResourceMetadata(ResourceType type, 
	std::string optional_name=""){
	/*
	This function call is just to keep me from accidentally forgetting to fill out fields
	as the number of fields in this struct grows. Initializing metadata will be done from a number
	of different places, so I want problems to be a compile time error.
	*/

	ResourceId new_id = getNewResourceId();

	bool is_name_string_defined = optional_name != "";
	if(is_name_string_defined){
		m_resource_name_map[new_id] = optional_name;
	}

	m_type_counts[type] += 1;
	m_resource_metadata_map[new_id] = {
		.id=new_id, 
		.type=type,
		.is_name_string_defined=is_name_string_defined,
		.is_transform_defined=false,

		.index=INVALID_INTERNAL_INDEX
	};

	return new_id;
}

void ResourceManager::assertNotInUse(ResourceId id){
	/*
	Asserts that the given resource id does not exist yet in any id -> data maps.
	*/

	// Search in metadata map
	assert(m_resource_metadata_map.find(id) == m_resource_metadata_map.end());
	assert(m_resource_name_map.find(id) == m_resource_name_map.end()); // Names (shared by all)
	assert(id >= m_next_available_id);

	// Search in widget maps
	assert(m_widget_groups.find(id) == m_widget_groups.end());         // Groups
	assert(m_widget_meshes.find(id) == m_widget_meshes.end());         // Meshes
}
#endif
