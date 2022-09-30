#include "AssetManagerOpenGL.hpp"

AssetManagerOpenGL::AssetManagerOpenGL(){
	constexpr Int64 ARBITRARY_INITIAL_STATE = 3141592653589;
	m_id_generator.state = ARBITRARY_INITIAL_STATE;
	m_next_id = 1;
}

AssetManagerOpenGL::~AssetManagerOpenGL(){

}

RenderableId AssetManagerOpenGL::setRenderableData(ResourceHandle handle, 
	const TriangleMesh& data){
	/*
	*/

	deleteRenderable(getId(handle));

	RenderableId new_id = generateNewId();	
	m_asset_type_map[new_id] = ASSET_RIGID_TRIANGLE_MESH;
	m_handle_to_id_map[handle] = new_id;
	RenderableBufferMetadata new_metadata = initTriangleMesh(data);
	new_metadata.id = new_id;
	m_id_to_buffer_metadata_map[new_id] = new_metadata;

	return new_id;
}

RenderableId AssetManagerOpenGL::setRenderableData(ResourceHandle handle, 
	TempTexture data){
	/*
	*/

	deleteRenderable(getId(handle));
	return INVALID_RENDERABLE_ID;
}

RenderableId AssetManagerOpenGL::setRenderableData(CellAddress address, 
	const ChunkVoxelMesh& mesh){
	/*
	*/
	
	deleteRenderable(getId(address));

	RenderableId new_id = generateNewId();	
	m_asset_type_map[new_id] = ASSET_CHUNK_VOXEL_MESH;
	m_cell_addr_to_id_map[address] = new_id;
	RenderableBufferMetadata new_metadata = initChunkVoxelMesh(mesh);
	new_metadata.id = new_id;
	m_id_to_buffer_metadata_map[new_id] = new_metadata;

	return new_id;
}

RenderableId AssetManagerOpenGL::setRenderableData(std::string group_name, 
	const WidgetMesh& widget_mesh){
	/*
	*/

	deleteRenderable(getId(group_name));
	
	RenderableId new_id = generateNewId();	
	m_asset_type_map[new_id] = ASSET_WIDGET_MESH;
	m_widget_group_name_to_id_map[group_name] = new_id;
	RenderableBufferMetadata new_metadata = initWidgetMesh(widget_mesh);
	new_metadata.id = new_id;
	m_id_to_buffer_metadata_map[new_id] = new_metadata;

	return new_id;
}

RenderableId AssetManagerOpenGL::getId(ResourceHandle handle){
	auto iter = m_handle_to_id_map.find(handle);
	if(iter != m_handle_to_id_map.end()){
		return iter->second;
	}
	return INVALID_RENDERABLE_ID;
}

RenderableId AssetManagerOpenGL::getId(CellAddress addr){
	auto iter = m_cell_addr_to_id_map.find(addr);
	if(iter != m_cell_addr_to_id_map.end()){
		return iter->second;
	}
	return INVALID_RENDERABLE_ID;
}

RenderableId AssetManagerOpenGL::getId(std::string group_name){
	auto iter = m_widget_group_name_to_id_map.find(group_name);
	if(iter != m_widget_group_name_to_id_map.end()){
		return iter->second;
	}
	return INVALID_RENDERABLE_ID;
}

RenderableBufferMetadata AssetManagerOpenGL::getMetadata(RenderableId id) const{
	auto iter = m_id_to_buffer_metadata_map.find(id);
	if(iter != m_id_to_buffer_metadata_map.end()){
		return iter->second;
	}else{
		assert(false);
		RenderableBufferMetadata invalid_buffer;
		return invalid_buffer;	
	}
}

void AssetManagerOpenGL::deleteRenderable(RenderableId id){
	/*
	Clears any existing information associated with the target renderable. This is used to prevent
	memory leaks. The buffers are cleared and the entry is removed from the map.

	REFERENCE: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDeleteBuffers.xhtml
	*/

	auto buffer_iter = m_id_to_buffer_metadata_map.find(id);
	if(buffer_iter != m_id_to_buffer_metadata_map.end()){
		RenderableBufferMetadata metadata = buffer_iter->second;

		// NOTE: From glDeleteBuffers documentation "glDeleteBuffers silently ignores 0's and 
		// names that do not correspond to existing buffer objects."
		GLuint buffers_to_delete[] = {metadata.vao, metadata.vbo, metadata.ebo};
		glDeleteBuffers(3, &buffers_to_delete[0]);

		m_id_to_buffer_metadata_map.erase(buffer_iter);
		
		// Figure out what type of asset this was and erase its
		// entry in the appropriate map
		auto asset_iter = m_asset_type_map.find(id);
		assert(asset_iter != m_asset_type_map.end());
		AssetType asset_type = asset_iter->second;
		m_asset_type_map.erase(asset_iter);

		// TODO: Fix this absurd linear iteration nonsense
		bool did_delete = false;
		if(asset_type == ASSET_WIDGET_MESH){
			for(auto& [key, value] : m_widget_group_name_to_id_map){
				if(value == id){
					m_widget_group_name_to_id_map.erase(key);
					did_delete = true;
					break;
				}
			}
		}else if(asset_type == ASSET_CHUNK_VOXEL_MESH){
			for(auto& [key, value] : m_cell_addr_to_id_map){
				if(value == id){
					m_cell_addr_to_id_map.erase(key);
					did_delete = true;
					break;
				}
			}
		}else if(asset_type == ASSET_RIGID_TRIANGLE_MESH ||
			asset_type == ASSET_TEXTURE){
			for(auto& [key, value] : m_handle_to_id_map){
				if(value == id){
					m_handle_to_id_map.erase(key);
					did_delete = true;
					break;
				}
			}
		}else{
			assert(false);
		}
		assert(did_delete);
	}else if(id != INVALID_RENDERABLE_ID){
		assert(false);
	}
}

RenderableId AssetManagerOpenGL::generateNewId(){
	return m_next_id++;
	return m_id_generator.next();
}


RenderableBufferMetadata AssetManagerOpenGL::initTriangleMesh(
	const TriangleMesh& triangle_mesh) const{
	/*
	Internal helper function to init VAO, VBO, EBO buffers from a given triangle mesh.
	*/

	RigidTriangleMeshOpengl mesh = MesherOpenGL::toOpenglFormat(triangle_mesh);
	
	Uint32 VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind the new VAO for this mesh
	// All further buffers (just VBO and EBO) go under this VAO
	glBindVertexArray(VAO);
	{
		// VBO
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, 
			mesh.vertices.size() * sizeof(RigidTriangleMeshVertex), 
			&mesh.vertices[0], GL_STATIC_DRAW);

		// EBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
			mesh.indices.size() * sizeof(Uint32),
			&mesh.indices[0], GL_STATIC_DRAW);

		// Start setting attributes
		//-------------------------------------------------------------
		// Position
		glEnableVertexAttribArray(0);	
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
			sizeof(RigidTriangleMeshVertex), (void*) (sizeof(float) * 0));
		
		// UV's
		glEnableVertexAttribArray(1);	
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
			sizeof(RigidTriangleMeshVertex), (void*) (sizeof(float) * 3));
		
		// Normals
		glEnableVertexAttribArray(2);	
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 
			sizeof(RigidTriangleMeshVertex), (void*) (sizeof(float) * 5));
		//--------------------------------------------------------------
	}
	glBindVertexArray(0);

	RenderableBufferMetadata metadata;
	metadata.id = INVALID_RENDERABLE_ID;
	metadata.render_type = MESH_TRIANGLES;
	
	metadata.vao = VAO;
	metadata.vbo = VBO;
	metadata.ebo = EBO;
	metadata.num_vertices = mesh.vertices.size();
	metadata.num_indices = mesh.indices.size();

	return metadata;
}

RenderableBufferMetadata AssetManagerOpenGL::initWidgetMesh(
	const WidgetMeshOpengl& widget_mesh) const{
	/*
	Internal helper function to init VAO, VBO, EBO buffers from a given widget mesh.
	*/

	auto mesh = MesherOpenGL::toOpenglFormat(widget_mesh);
	
	Uint32 VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind the new VAO for this mesh
	// All further buffers (just VBO and EBO) go under this VAO
	glBindVertexArray(VAO);
	{
		// VBO
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, 
			mesh.vertices.size() * sizeof(WidgetMeshVertex), 
			&mesh.vertices[0], GL_STATIC_DRAW);

		// EBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
			mesh.indices.size() * sizeof(Uint32),
			&mesh.indices[0], GL_STATIC_DRAW);

		// Start setting attributes
		//-------------------------------------------------------------
		// Position
		glEnableVertexAttribArray(0);	
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
			sizeof(WidgetMeshVertex), (void*) (sizeof(float) * 0));
		
		// UV's
		glEnableVertexAttribArray(1);	
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
			sizeof(WidgetMeshVertex), (void*) (sizeof(float) * 3));
		
		// Colors
		glEnableVertexAttribArray(2);	
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
			sizeof(WidgetMeshVertex), (void*) (sizeof(float) * 5));
		//--------------------------------------------------------------
	}
	glBindVertexArray(0);

	RenderableBufferMetadata metadata;
	metadata.id = INVALID_RENDERABLE_ID;
	metadata.render_type = mesh.type;
	
	metadata.vao = VAO;
	metadata.vbo = VBO;
	metadata.ebo = EBO;
	metadata.num_vertices = mesh.vertices.size();
	metadata.num_indices = mesh.indices.size();

	return metadata;
}

RenderableBufferMetadata AssetManagerOpenGL::initChunkVoxelMesh(
	const ChunkVoxelMeshOpengl& chunk_mesh) const{
	/*
	Internal helper function to init VAO, VBO, EBO buffers from a given chunk voxel mesh.
	*/

	auto mesh = MesherOpenGL::toOpenglFormat(chunk_mesh);
	
	Uint32 VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind the new VAO for this mesh
	// All further buffers (just VBO and EBO) go under this VAO
	glBindVertexArray(VAO);
	{
		// VBO
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, 
			mesh.vertices.size() * sizeof(VoxelMeshVertex), 
			&mesh.vertices[0], GL_STATIC_DRAW);

		// EBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
			mesh.indices.size() * sizeof(Uint32),
			&mesh.indices[0], GL_STATIC_DRAW);

		// Start setting attributes
		//-------------------------------------------------------------
		// Position
		glEnableVertexAttribArray(0);	
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
			sizeof(VoxelMeshVertex), (void*) (sizeof(float) * 0));
		
		// UV's
		glEnableVertexAttribArray(1);	
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
			sizeof(VoxelMeshVertex), (void*) (sizeof(float) * 3));
		
		// Type Index
		glEnableVertexAttribArray(2);	
		glVertexAttribPointer(2, 1, GL_UNSIGNED_INT, GL_FALSE,
			sizeof(VoxelMeshVertex), (void*) (sizeof(Bytes4) * 5));

		// Normal Index
		glEnableVertexAttribArray(3);	
		glVertexAttribPointer(3, 1, GL_UNSIGNED_INT, GL_FALSE,
			sizeof(VoxelMeshVertex), (void*) (sizeof(Bytes4) * 6));
		//--------------------------------------------------------------
	}
	glBindVertexArray(0);

	RenderableBufferMetadata metadata;
	metadata.id = INVALID_RENDERABLE_ID;
	metadata.render_type = MESH_TRIANGLES;
	
	metadata.vao = VAO;
	metadata.vbo = VBO;
	metadata.ebo = EBO;
	metadata.num_vertices = mesh.vertices.size();
	metadata.num_indices = mesh.indices.size();

	return metadata;
}



#if 0
//-----------------------------------------------
// Public
//-----------------------------------------------
AssetManagerOpenGL::AssetManagerOpenGL(){
	m_next_available_id = 1;
}

AssetManagerOpenGL::~AssetManagerOpenGL(){

}

RenderableId AssetManagerOpenGL::initNewRenderable(){
	/*

	*/

	RenderableId new_id = getNextValidId();
	m_active_renderable_ids.insert(new_id);
	return new_id;
}

bool AssetManagerOpenGL::eraseRenderable(RenderableId id){
	/*
	Given the id of an existing renderable, delete the target.
	Returns True if a deletion occurred, false otherwise.
	*/
	
	// Check if there's an active id. Delete if so.
	bool did_erase = false;
	auto iter_ids = m_active_renderable_ids.find(id);
	if(iter_ids != m_active_renderable_ids.end()){
		m_active_renderable_ids.erase(iter_ids);
		did_erase = true;

		// If the id was active there might be buffer data associated with it
		auto iter_renderables = m_renderables.find(id);
		if(iter_renderables != m_renderables.end()){
			clearRenderableBuffer(id, false);
		}
	}

	return did_erase;
}

/*
bool AssetManagerOpenGL::setRenderableData(RenderableId id, 
	const std::vector<Widget>& widget_group){
	
	//TODO: Return false if something went wrong during the process.

	WidgetMeshOpengl mesh = MesherOpenGL::toOpenglFormat(generateWidgetMesh(widget_group));
	RenderableBufferMetadata metadata = initWidgetMesh(mesh);

	// Clear the old buffer (if any) and add the new one
	clearRenderableBuffer(id);
	metadata.id = id;
	m_renderables[id] = metadata;

	return true;
}
*/

bool AssetManagerOpenGL::setRenderableData(RenderableId id, const WidgetMesh& widget_mesh){
	/*
	TODO: Return false if something went wrong during the process
	*/

	WidgetMeshOpengl mesh = MesherOpenGL::toOpenglFormat(widget_mesh);
	RenderableBufferMetadata metadata = initWidgetMesh(mesh);

	// Clear the old buffer (if any) and add the new one
	clearRenderableBuffer(id, true);
	metadata.id = id;
	m_renderables[id] = metadata;

	return true;
}

bool AssetManagerOpenGL::setRenderableData(RenderableId id, const ChunkVoxelMesh& voxel_mesh){
	/*
	TODO: Return false if something went wrong during the process
	*/

	ChunkVoxelMeshOpengl mesh = MesherOpenGL::toOpenglFormat(voxel_mesh);
	RenderableBufferMetadata metadata = initChunkVoxelMesh(mesh);

	// Clear the old buffer (if any) and add the new one
	clearRenderableBuffer(id, true);
	metadata.id = id;
	m_renderables[id] = metadata;

	return true;
}

// bool setRenderableData(RenderableId id, RESOURCE_TYPE_1);
// bool setRenderableData(RenderableId id, RESOURCE_TYPE_2);
// bool setRenderableData(RenderableId id, RESOURCE_TYPE_3);

RenderableBufferMetadata AssetManagerOpenGL::getMetadata(RenderableId id){
	auto iter = m_renderables.find(id);
	if(iter != m_renderables.end()){
		return iter->second;
	}else{
		RenderableBufferMetadata invalid_metadata;  // Default initializes invalid.
		return invalid_metadata;
	}
}

bool AssetManagerOpenGL::eraseResource(ResourceId id){
	/*
	Find the renderable list associated with the given resource 
	and delete all members of the list if found
	*/

	bool did_erase = false;
	auto iter = m_resource_to_renderable.find(id);
	if(iter != m_resource_to_renderable.end()){
		for(RenderableId renderable_id : iter->second){
			eraseRenderable(renderable_id);	
		}
		did_erase = true;
	}

	return did_erase;
}

bool AssetManagerOpenGL::linkRenderableToResource(RenderableId renderable_id, 
	ResourceId resource_id){
	/*
	Returns true if the renderable could be linked with the given resource id. False if not.
	*/

	if(isRenderableInitialized(renderable_id)){
		// Now add to the list, making sure not to add duplicates
		for(RenderableId id : m_resource_to_renderable[resource_id]){
			if(id == renderable_id){
				return false;
			}
		}
		m_resource_to_renderable[resource_id].push_back(renderable_id);
		return true;
	}else{
		return false;
	}
}

std::vector<RenderableId> AssetManagerOpenGL::getAllLinkedRenderables(ResourceId id){
	/*
	If the given resource exists, return a list of all linked renderables. If not, just
	return an empty vector.
	*/

	auto iter = m_resource_to_renderable.find(id);
	if(iter != m_resource_to_renderable.end()){
		return iter->second;
	}else{
		return {};
	}
}

bool AssetManagerOpenGL::eraseCell(CellAddress addr){
	/*
	Find the renderable list associated with the given cell address 
	and delete all members of the list if found
	*/

	bool did_erase = false;
	auto iter = m_addr_to_renderable.find(addr);
	if(iter != m_addr_to_renderable.end()){
		for(RenderableId renderable_id : iter->second){
			eraseRenderable(renderable_id);	
		}
		did_erase = true;
	}

	return did_erase;
}

bool AssetManagerOpenGL::linkRenderableToCell(RenderableId renderable_id, CellAddress addr){
	/*
	Returns true if the renderable could be linked with the given cell address. False if not.
	*/

	if(isRenderableInitialized(renderable_id)){
		// Now add to the list, making sure not to add duplicates
		for(RenderableId id : m_addr_to_renderable[addr]){
			if(id == renderable_id){
				return false;
			}
		}
		m_addr_to_renderable[addr].push_back(renderable_id);
		return true;
	}else{
		return false;
	}
}

std::vector<RenderableId> AssetManagerOpenGL::getAllLinkedRenderables(CellAddress addr){
	/*
	If the given cell exists, return a list of all linked renderables. If not, just
	return an empty vector.
	*/

	auto iter = m_addr_to_renderable.find(addr);
	if(iter != m_addr_to_renderable.end()){
		return iter->second;
	}else{
		return {};
	}
}



//-----------------------------------------------
// Private methods
//-----------------------------------------------
RenderableBufferMetadata AssetManagerOpenGL::initWidgetMesh(const WidgetMeshOpengl& mesh){
	/*
	Internal helper function to init VAO, VBO, EBO buffers from a given widget mesh.
	*/
	
	Uint32 VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind the new VAO for this mesh
	// All further buffers (just VBO and EBO) go under this VAO
	glBindVertexArray(VAO);
	{
		// VBO
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, 
			mesh.vertices.size() * sizeof(WidgetMeshVertex), 
			&mesh.vertices[0], GL_STATIC_DRAW);

		// EBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
			mesh.indices.size() * sizeof(Uint32),
			&mesh.indices[0], GL_STATIC_DRAW);

		// Start setting attributes
		//-------------------------------------------------------------
		// Position
		glEnableVertexAttribArray(0);	
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
			sizeof(WidgetMeshVertex), (void*) (sizeof(float) * 0));
		
		// UV's
		glEnableVertexAttribArray(1);	
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
			sizeof(WidgetMeshVertex), (void*) (sizeof(float) * 3));
		
		// Colors
		glEnableVertexAttribArray(2);	
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
			sizeof(WidgetMeshVertex), (void*) (sizeof(float) * 5));
		//--------------------------------------------------------------
	}
	glBindVertexArray(0);

	RenderableBufferMetadata metadata;
	metadata.id = INVALID_RENDERABLE_ID;
	metadata.render_type = mesh.type;
	
	metadata.vao = VAO;
	metadata.vbo = VBO;
	metadata.ebo = EBO;
	metadata.num_vertices = mesh.vertices.size();
	metadata.num_indices = mesh.indices.size();

	return metadata;
}

RenderableBufferMetadata AssetManagerOpenGL::initChunkVoxelMesh(const ChunkVoxelMeshOpengl& mesh){
	/*
	Internal helper function to init VAO, VBO, EBO buffers from a given chunk voxel mesh.
	*/
	
	Uint32 VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind the new VAO for this mesh
	// All further buffers (just VBO and EBO) go under this VAO
	glBindVertexArray(VAO);
	{
		// VBO
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, 
			mesh.vertices.size() * sizeof(VoxelMeshVertex), 
			&mesh.vertices[0], GL_STATIC_DRAW);

		// EBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
			mesh.indices.size() * sizeof(Uint32),
			&mesh.indices[0], GL_STATIC_DRAW);

		// Start setting attributes
		//-------------------------------------------------------------
		// Position
		glEnableVertexAttribArray(0);	
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
			sizeof(VoxelMeshVertex), (void*) (sizeof(float) * 0));
		
		// UV's
		glEnableVertexAttribArray(1);	
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
			sizeof(VoxelMeshVertex), (void*) (sizeof(float) * 3));
		
		// Type Index
		glEnableVertexAttribArray(2);	
		glVertexAttribPointer(2, 1, GL_UNSIGNED_INT, GL_FALSE,
			sizeof(VoxelMeshVertex), (void*) (sizeof(Bytes4) * 5));

		// Normal Index
		glEnableVertexAttribArray(3);	
		glVertexAttribPointer(3, 1, GL_UNSIGNED_INT, GL_FALSE,
			sizeof(VoxelMeshVertex), (void*) (sizeof(Bytes4) * 6));
		//--------------------------------------------------------------
	}
	glBindVertexArray(0);

	RenderableBufferMetadata metadata;
	metadata.id = INVALID_RENDERABLE_ID;
	metadata.render_type = MESH_TRIANGLES;
	
	metadata.vao = VAO;
	metadata.vbo = VBO;
	metadata.ebo = EBO;
	metadata.num_vertices = mesh.vertices.size();
	metadata.num_indices = mesh.indices.size();

	return metadata;
}

void AssetManagerOpenGL::clearRenderableBuffer(RenderableId id, bool should_retain_entry){
	/*
	Clears any existing information associated with the target renderable. This is used to prevent
	memory leaks if an updated mesh is attached to an existing renderable id.
	
	Allows the user to keep the map entry in case they're overwriting an existing
	renderable's metadata with new data by setting should_retain_entry to true. Otherwise
	the buffers are cleared and the entry is removed from the map.

	REFERENCE: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDeleteBuffers.xhtml
	*/

	auto iter = m_renderables.find(id);
	if(iter != m_renderables.end()){
		RenderableBufferMetadata& old_metadata = iter->second;

		// NOTE: From glDeleteBuffers documentation "glDeleteBuffers silently ignores 0's and 
		// names that do not correspond to existing buffer objects."
		GLuint buffers_to_delete[] = {old_metadata.vao, old_metadata.vbo, old_metadata.ebo};
		glDeleteBuffers(3, &buffers_to_delete[0]);

		if(!should_retain_entry){
			m_renderables.erase(iter);
		}
	}
}

RenderableId AssetManagerOpenGL::getNextValidId(){
	/*
	Currently just returns an incrementing 64bit value.
	*/
	return m_next_available_id++;
}

bool AssetManagerOpenGL::isRenderableInitialized(RenderableId id){
	auto iter = m_active_renderable_ids.find(id);
	return iter != m_active_renderable_ids.end();
}

void AssetManagerOpenGL::printSummaryStatsDebugging() const{
	/*
	Prints a bunch of the internal tracking data for debugging purposes
	*/

	printf("<Summary Stats for AssetManagerOpengl>\n");
	printf("\tm_resource_to_renderable:\n");
	for(auto& [key, val] : m_resource_to_renderable){
		printf("\t\tRes'%li': Ren[", key);
		for(RenderableId id : val){
			printf(" %li", id);
		}
		printf("]\n");
	}

	printf("\n\tm_addr_to_renderable:\n");
	for(auto& [addr, val] : m_addr_to_renderable){
		printf("\t\tCell{");
		printPODStruct(addr.corner_addr);
		printf(", %i}: Ren[", addr.lod_power);
		for(RenderableId id : val){
			printf(" %li", id);
		}
		printf("]\n");
	}

	printf("\n\tm_renderables:\n");
	for(auto& [key, buffer] : m_renderables){
		printf("\t\tRen'%li': Buffer:\n", key);
		printf("\t\t\tRenId'%li', Type'%i'\n", buffer.id, buffer.render_type);
		printf("\t\t\tVAO'%u', VBO'%u', EBO'%u'\n", buffer.vao, buffer.vbo, buffer.ebo);
		printf("\t\t\tNumVerts'%u', NumIndices'%u'\n", buffer.num_vertices, buffer.num_indices);
	}

	printf("\n\tm_active_renderable_ids:\n\t(%li)", m_active_renderable_ids.size());
	for(RenderableId id : m_active_renderable_ids){
		printf(" %li", id);
	}
	printf("\n");

	printf("\n\tm_next_available_id: '%li'\n", m_next_available_id);
	printf("</SummaryStats>\n");
}
#endif
