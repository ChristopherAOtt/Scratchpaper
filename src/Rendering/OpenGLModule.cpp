#include "OpenGLModule.hpp"


//-------------------------------------------------------------------------------------------------
// Helper structures and functions
//-------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------
// Placeholder Priority Queue
//-------------------------------------------------------------------------------------------------
JankPriorityQueue::JankPriorityQueue(){

}

JankPriorityQueue::~JankPriorityQueue(){

}

void JankPriorityQueue::setLeniency(CellAddress addr, Leniency leniency){
	/*
	If the value already exists, update the leniency. If not, init and set.
	*/

	m_queue.push(PriorityPair{
		.addr=addr,
		.leniency=leniency
	});
}

JankPriorityQueue::PriorityPair JankPriorityQueue::popNext(){
	/*
	Returns a pair with the lowest leniency
	*/
	
	auto output = m_queue.top();
	m_queue.pop();
	return output;
}

JankPriorityQueue::PriorityPair JankPriorityQueue::peekNext(){
	return m_queue.top();
}

size_t JankPriorityQueue::size() const{
	return m_queue.size();
}


//-------------------------------------------------------------------------------------------------
// Mesh Job
//-------------------------------------------------------------------------------------------------
void runChunkMeshingJob(WorkGroup::ScratchMemory* input_ptr, WorkGroup::ScratchMemory* output_ptr){
	/*

	*/

	assert(input_ptr != NULL && input_ptr->memoryPtr());

	ChunkMesherJobInput input_job = *((ChunkMesherJobInput*) input_ptr->memoryPtr());
	CellAddress addr = input_job.addr;
	std::unordered_map<CellAddress, ChunkVoxelMesh*, PODHasher>* working_mesh_map_ptr = 
		input_job.working_mesh_map_ptr;

	// For the moment, only unlit voxel chunk types are supported
	ChunkMesherJobOutput output = {.addr={}, .is_successful=false};
	if(input_job.output_mesh_type == MESHTYPE_CHUNK_VOXEL){
		MesherAdjacencyGrid adjacency_grid = generateAdjacencyGrid(input_job.chunk_data);
		ChunkVoxelMesh voxel_mesh = generateChunkMesh(adjacency_grid, addr);
		{
			std::lock_guard<std::mutex> write_lock(*input_job.write_mutex_ptr);
			*working_mesh_map_ptr->at(addr) = voxel_mesh;
		}
		
		// Record the fact that this was a success
		output = {.addr=addr, .is_successful=true};
	}else{
		assert(false);
	}
	output_ptr->writeData(&output, sizeof(ChunkMesherJobOutput));
	return;
}

//-------------------------------------------------------------------------------------------------
// OpenGL subclass
//-------------------------------------------------------------------------------------------------
OpenGLModule::OpenGLModule(): m_work_group(NUM_RENDERER_WORKERS){
	m_widget_shader = std::unique_ptr<BaseShader>(new WidgetShader);
	m_chunk_shader = std::unique_ptr<BaseShader>(new ChunkShader);
	m_rigid_triangle_mesh_shader = std::unique_ptr<BaseShader>(new RigidTriangleMeshShader);

	m_render_widgets = true;

	constexpr bool BACKFACE_CULLING_IS_ENABLED = true;
	if(BACKFACE_CULLING_IS_ENABLED){
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
}

OpenGLModule::~OpenGLModule(){

}

void OpenGLModule::initShadersFromSourceFiles(){
	/*
	For each shader, load its source file and recompile. This allows the user to reload the
	shaders at runtime.
	*/

	const std::string VERTEX_SECTION_NAME = "VERTEX_SHADER";
	const std::string FRAGMENT_SECTION_NAME = "FRAGMENT_SHADER";
	std::vector<std::pair<BaseShader*, std::string>> shaders_and_sources = {
		{m_widget_shader.get(), "Shaders/Widget.shader"},
		{m_chunk_shader.get(), "Shaders/Chunk.shader"},
		{m_rigid_triangle_mesh_shader.get(), "Shaders/RigidTriangleMesh.shader"},
	};

	const char* OUTCOME_TEXT[] = {
		"FAILURE",
		"SUCCESS"
	};
	
	printf("Loading %lu shader files...\n", shaders_and_sources.size());
	for(auto [shader_ptr, source_filepath] : shaders_and_sources){
		// Delete the old shader to avoid memory leaks
		shader_ptr->destroy();
		
		// Init the new shader
		auto shader_map = loadShadersFromFile(source_filepath);
		for(auto name : {VERTEX_SECTION_NAME, FRAGMENT_SECTION_NAME}){
			auto iter = shader_map.find(name);
			assert(iter != shader_map.end());
		}

		shader_ptr->compileShaders(
			shader_map[VERTEX_SECTION_NAME], 
			shader_map[FRAGMENT_SECTION_NAME]);
		printf("\tLoaded %p --> %s (%s)\n", (void*) shader_ptr, source_filepath.c_str(), 
			OUTCOME_TEXT[shader_ptr->isValid()]);
	}
}

void OpenGLModule::sendInstruction(SystemInstruction instruction){
	m_waiting_instructions.push_back(instruction);
}

void OpenGLModule::processStoredInstructions(const SimCache& state){
	/*
	Go through the list of instructions given to the renderer and run them.
	Most of these will likely be setting/removing data.

	TODO: Condense logic to remove repetitive code.
	REFACTOR: Self explanatory
	*/

	for(SystemInstruction& sys_instruction : m_waiting_instructions){
		if(sys_instruction.type == INSTRUCTION_ASSET){
			AssetInstruction& instruction = sys_instruction.asset_instruction;
			if(instruction.type == LOAD_ASSET_BY_ID){
				setData(state, instruction.handle);
			}else if(instruction.type == UNLOAD_ASSET_BY_ID){
				removeData(state, instruction.handle);
			}else if(instruction.type == LOAD_ASSET_BY_NAME){
				setData(state, &instruction.name.text[0]);
			}else if(instruction.type == UNLOAD_ASSET_BY_NAME){
				removeData(state, &instruction.name.text[0]);
			}
		}else if(sys_instruction.type == INSTRUCTION_CHUNK){
			ChunkInstruction& instruction = sys_instruction.chunk_instruction;
			CellAddress addr = instruction.lod_addr;

			if(instruction.type == CHUNK_GENERATE_UNLIT_MESH){	
				if(instruction.leniency == IMMEDIATE_ACTION_REQUIRED){
					// Everything must wait until this is done.
					RawVoxelChunk chunk_data;
					ChunkTable* table_ptr = &state.m_reference_world->m_chunk_table;
					
					table_ptr->m_access_mutex.lock();
					chunk_data = *table_ptr->getChunkPtr(addr.corner_addr);
					table_ptr->m_access_mutex.unlock();

					MesherAdjacencyGrid adjacency_grid = generateAdjacencyGrid(chunk_data);
					ChunkVoxelMesh voxel_mesh = generateChunkMesh(adjacency_grid, addr);
					setData(state, addr, voxel_mesh);
				}else{
					m_priority_queue.setLeniency(addr, instruction.leniency);	
				}	
			}else if(instruction.type == CHUNK_ERASE_UNLIT_MESH){
				removeData(state, addr);
			}
		}else if(sys_instruction.type == INSTRUCTION_GENERAL_TEXT){
			// Toggle rendermode
			if(sys_instruction.text_instruction.text == RENDERER_LINES_COMMAND_STRING){
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}else if(sys_instruction.text_instruction.text == RENDERER_FACES_COMMAND_STRING){
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			// Toggle widget visibility
			if(sys_instruction.text_instruction.text == RENDERER_SHOW_WIDGETS){
				m_render_widgets = true;
			}else if(sys_instruction.text_instruction.text == RENDERER_HIDE_WIDGETS){
				m_render_widgets = false;
			}

			// Reload shaders from files
			if(sys_instruction.text_instruction.text == RENDERER_RELOAD_SHADERS_FROM_FILE){
				initShadersFromSourceFiles();
			}

			// Generate meshes in a different thread
			if(sys_instruction.text_instruction.text == RENDERER_SCHEDULE_MESHING){
				this->scheduleChunkMeshing(state);
			}
		}
	}

	if(m_waiting_instructions.size() > 0){
		m_waiting_instructions.clear();	
	}
}

void OpenGLModule::render(const SimCache& state, const Camera& camera, 
	const RendererSettings& settings){
	/*

	*/

	const WorldState* world_ptr = state.m_reference_world;
	const ResourceManager* resource_manager = state.m_resource_manager;

	this->processStoredInstructions(state);
	this->scheduleChunkMeshing(state);

	// Generate matrices
	FMat4 view_matrix = MathUtils::Matrix::makeViewMatrix(camera);
	FMat4 projection_matrix = MathUtils::Matrix::makePerspectiveProjectionMatrix(camera);
	FMat4 identity = MathUtils::Matrix::initIdentityMatrix();

	//-------------------------------------------
	// Render all Widgets
	//-------------------------------------------
	if(m_render_widgets){
		m_widget_shader->use();
		m_widget_shader->setTime(world_ptr->currentTime());
		m_widget_shader->setViewMatrix(view_matrix);
		m_widget_shader->setProjectionMatrix(projection_matrix);
		m_widget_shader->setCameraPos(camera.pos);
		m_widget_shader->setFogColor(world_ptr->m_fog_color);
		m_widget_shader->setModelMatrix(identity);
		for(const std::string& group_name : m_tracked_widget_groups){
			RenderableId group_id = m_asset_manager.getId(group_name);
			if(group_id == INVALID_RENDERABLE_ID){
				continue;
			}

			RenderableBufferMetadata metadata = m_asset_manager.getMetadata(group_id);
			glBindVertexArray(metadata.vao);
			glDrawElements(
				GL_LINES,
				metadata.num_indices, 
				GL_UNSIGNED_INT, 
				(void*)0);
		}
		glBindVertexArray(0);
		m_widget_shader->stop();
	}

	//-------------------------------------------
	// Render all chunk meshes
	//-------------------------------------------
	m_chunk_shader->use();
	m_chunk_shader->setTime(world_ptr->currentTime());
	m_chunk_shader->setViewMatrix(view_matrix);
	m_chunk_shader->setProjectionMatrix(projection_matrix);
	m_chunk_shader->setCameraPos(camera.pos);
	m_chunk_shader->setFogColor(world_ptr->m_fog_color);
	for(CellAddress cell_addr : m_tracked_cells){
		RenderableId cell_id = m_asset_manager.getId(cell_addr);
		if(cell_id == INVALID_RENDERABLE_ID){
			continue;
		}

		IVec3 voxel_pos = cell_addr.corner_addr * CHUNK_LEN;
		FMat4 model_matrix = MathUtils::Matrix::makeModelMatrix(toFloatVector(voxel_pos));
		m_chunk_shader->setModelMatrix(model_matrix);

		RenderableBufferMetadata metadata = m_asset_manager.getMetadata(cell_id);
		glBindVertexArray(metadata.vao);
		glDrawElements(
			GL_TRIANGLES,
			metadata.num_indices, 
			GL_UNSIGNED_INT, 
			(void*)0);
	}
	glBindVertexArray(0);
	m_chunk_shader->stop();

	//-------------------------------------------
	// Render all entities
	//-------------------------------------------
	// TODO: Group all entities by type get handle/mesh info once per type
	ResourceHandle drone_handle = resource_manager->handleByName("DroneModel");
	assert(drone_handle.type != RESOURCE_INVALID);
	RenderableId drone_renderable_id = m_asset_manager.getId(drone_handle);
	assert(drone_renderable_id != INVALID_RENDERABLE_ID);
	RenderableBufferMetadata drone_metadata = m_asset_manager.getMetadata(drone_renderable_id);
	assert(drone_metadata.id != INVALID_RENDERABLE_ID);

	m_rigid_triangle_mesh_shader->use();
	m_rigid_triangle_mesh_shader->setTime(world_ptr->currentTime());
	m_rigid_triangle_mesh_shader->setViewMatrix(view_matrix);
	m_rigid_triangle_mesh_shader->setProjectionMatrix(projection_matrix);
	m_rigid_triangle_mesh_shader->setCameraPos(camera.pos);
	m_rigid_triangle_mesh_shader->setFogColor(world_ptr->m_fog_color);
	
	glBindVertexArray(drone_metadata.vao);
	for(const PlaceholderEntity& entity : world_ptr->m_placeholder_entities){
		// TODO: Legitimate filter for renderable entities
		if(entity.handle.type_index != 2){
			continue;
		}

		// TODO: Support rotation
		FMat4 model_matrix = MathUtils::Matrix::makeModelMatrix(entity.position);
		m_rigid_triangle_mesh_shader->setModelMatrix(model_matrix);

		glDrawElements(
			GL_TRIANGLES,
			drone_metadata.num_indices, 
			GL_UNSIGNED_INT, 
			(void*)0);
	}
	glBindVertexArray(0);
	m_rigid_triangle_mesh_shader->stop();
	
}

void OpenGLModule::setData(const SimCache& state, ResourceHandle handle){
	m_tracked_resources.insert(handle);

	const ResourceManager* resource_ptr = state.m_resource_manager;
	assert(handle.type != RESOURCE_INVALID);
	
	if(handle.type == RESOURCE_FONT){
		assert(false);
	}else if(handle.type == RESOURCE_TEXTURE){
		assert(false);
	}else if(handle.type == RESOURCE_RIGID_TRIANGLE_MESH){
		TriangleMesh mesh = resource_ptr->getDataTriangleMesh(handle);
		m_asset_manager.setRenderableData(handle, mesh);
	}else if(handle.type == RESOURCE_RIGGED_SKELETAL_MESH){
		assert(false);
	}else{
		assert(false);
	}
}

void OpenGLModule::setData(const SimCache& state, CellAddress addr, const ChunkVoxelMesh& mesh){
	/*
	This can be called from either processStoredInstructions or from a threaded
	mesh generation job. 
	*/

	m_tracked_cells.insert(addr);

	ChunkVoxelMeshOpengl opengl_mesh = MesherOpenGL::toOpenglFormat(mesh);
	m_asset_manager.setRenderableData(addr, opengl_mesh);
}

void OpenGLModule::setData(const SimCache& state, std::string name){
	const WorldState* world_ptr = state.m_reference_world;
	
	WidgetGroup group = world_ptr->groupData(name);
	m_tracked_widget_groups.insert(name);
	m_asset_manager.setRenderableData(group.name, generateWidgetMesh(group.widgets));
}

void OpenGLModule::removeData(const SimCache& state, ResourceHandle handle){
	m_tracked_resources.erase(handle);
	
	RenderableId id = m_asset_manager.getId(handle);
	assert(id != INVALID_RENDERABLE_ID);
	m_asset_manager.deleteRenderable(id);
}

void OpenGLModule::removeData(const SimCache& state, CellAddress addr){
	m_tracked_cells.erase(addr);

	RenderableId id = m_asset_manager.getId(addr);
	assert(id != INVALID_RENDERABLE_ID);
	m_asset_manager.deleteRenderable(id);
}

void OpenGLModule::removeData(const SimCache& state, std::string name){
	m_tracked_widget_groups.erase(name);

	RenderableId id = m_asset_manager.getId(name);
	assert(id != INVALID_RENDERABLE_ID);
	m_asset_manager.deleteRenderable(id);
}

void OpenGLModule::scheduleChunkMeshing(const SimCache& state){
	/*
	This runs regularly to run jobs in parallel. 
	TODO: Move this out of the rendering module and into a central work scheduler.
	WARNING: Leniency 0 jobs are not treated differently at the moment.
	TODO: Implement thread joining so this function blocks until all leniency 0 jobs finish.
	TODO: Make the output type customizable instead of being hardcoded with MESH_TRIANGLES
	TODO: Reuse memory so we're not calling new/delete for every new mesh.
	*/
	
	// STEP 1: Deal with prior work
	// TODO: Break this into two stages so that the mutex is only locked once during this stage
	std::vector<WorkGroup::WorkerId> waiting_workers = m_work_group.waitingWorkers();
	for(auto worker_id : waiting_workers){
		WorkGroup::ScratchMemory* output_scratch = m_work_group.workerOutput(worker_id);
		ChunkMesherJobOutput job_output = *((ChunkMesherJobOutput*) output_scratch->memoryPtr());
		if(job_output.is_successful){
			// Read the mesh out of the map
			m_working_mesh_mutex.lock();
			CellAddress& addr = job_output.addr;
			ChunkVoxelMesh finished_mesh = *m_working_chunk_voxel_meshes[job_output.addr];
			delete m_working_chunk_voxel_meshes[addr];  // Run destructors on object + free memory
			m_working_chunk_voxel_meshes.erase(addr);   // Delete map entry
			m_working_mesh_mutex.unlock();

			setData(state, addr, finished_mesh);
			//handleChunkMeshRenderable(job_output.addr, finished_mesh);
		}else{
			// The mesh job failed. That really shouldn't happen under the current system.
			assert(false);
		}

		m_work_group.markAsAvailable(worker_id);
	}

	// STEP 2: Launch new work
	int num_chunks_to_mesh = m_priority_queue.size();
	if(num_chunks_to_mesh > 0){
		int num_workers_available = m_work_group.numWorkersAvailable();
		if(num_workers_available > 0){
			int num_to_launch = min(num_chunks_to_mesh, num_workers_available);
			int num_pulled = 0;

			// Pull all addresses to launch, make sure none are currently running.
			// TODO: Batch the isRunning checks so the mutex is only locked/unlocked once per run.
			std::vector<CellAddress> addresses_to_launch;
			std::vector<RawVoxelChunk> chunk_data_vector;
			addresses_to_launch.reserve(num_to_launch);
			chunk_data_vector.reserve(num_to_launch);
			while(num_pulled < num_to_launch && m_priority_queue.size() > 0){
				auto [job_addr, leniency] = m_priority_queue.popNext();
				if(isMeshJobCurrentlyRunning(job_addr, MESHTYPE_CHUNK_VOXEL)){
					continue;
				}else{
					++num_pulled;
				}

				addresses_to_launch.push_back(job_addr);
			}

			// Reserve space for each new job in the working mesh map
			m_working_mesh_mutex.lock();
			for(const CellAddress& job_addr : addresses_to_launch){
				m_working_chunk_voxel_meshes[job_addr] = new ChunkVoxelMesh();
			}
			m_working_mesh_mutex.unlock();

			// Lock the chunk table and read out copies of data all at once.
			ChunkTable* table_ptr = &state.m_reference_world->m_chunk_table;
			table_ptr->m_access_mutex.lock();
			for(const CellAddress& job_addr : addresses_to_launch){
				const RawVoxelChunk* chunk_ptr = table_ptr->getChunkPtr(job_addr.corner_addr);
				assert(chunk_ptr != NULL);
				chunk_data_vector.push_back(*chunk_ptr);
			}
			table_ptr->m_access_mutex.unlock();

			// Now we're ready to launch. 
			for(Uint64 i = 0; i < addresses_to_launch.size(); ++i){
				ChunkMesherJobInput mesh_job = {
					.addr=addresses_to_launch[i], 
					.output_mesh_type=MESHTYPE_CHUNK_VOXEL,
					.write_mutex_ptr=&m_working_mesh_mutex,
					.working_mesh_map_ptr=&m_working_chunk_voxel_meshes,
					.chunk_data=chunk_data_vector[i]
				};

				m_work_group.launchJob(runChunkMeshingJob, &mesh_job, sizeof(ChunkMesherJobInput));
			}
		}
	}
}

bool OpenGLModule::isMeshJobCurrentlyRunning(CellAddress addr, MeshType type){
	/*
	Returns true if a job of the given type is currently running for the provided cell address.
	*/

	if(type == MESHTYPE_CHUNK_VOXEL){
		m_working_mesh_mutex.lock();
		bool result = m_working_chunk_voxel_meshes.find(addr) != m_working_chunk_voxel_meshes.end();
		m_working_mesh_mutex.unlock();

		return result;
	}else{
		return false;
	}
}





#if 0
/*
I'm keeping the code below as a Dexter-esque trophy to commemorate a nice
refactoring session.
*/

void OpenGLModule::processStoredInstructions(const SimCache& state){
	/*
	Go through the list of instructions given to the renderer and run them. Most of these
	will likely be asset updates.

	TODO: Condense logic to remove repetitive code.
	REFACTOR: Self explanatory
	*/

	const WorldState& world = *state.m_reference_world;
	const ResourceManager& resource_manager = *state.m_resource_manager;

	// Remove duplicates if applicable
	// TODO: Move this to its own function
	if(m_waiting_instructions.size() > 0){
		// TODO: Change to map of <key(instruction type+subtypes), value=unordered_set<ResourceId>>
		// This will make the code scale better than hand-coding new sets for every type
		std::unordered_set<ResourceId> loaded_asset_resource_ids;
		for(int i = m_waiting_instructions.size() - 1; i >= 0; --i){
			SystemInstruction sys_instruction = m_waiting_instructions[i];
			if(sys_instruction.type == INSTRUCTION_ASSET){
				AssetInstruction& asset_instruction = sys_instruction.asset_instruction;
				if(asset_instruction.type == LOAD_ASSET_BY_ID){
					if(loaded_asset_resource_ids.count(asset_instruction.id) > 0){
						m_waiting_instructions.erase(m_waiting_instructions.begin() + i);
					}else{
						loaded_asset_resource_ids.insert(asset_instruction.id);	
					}
				}
			}
		}
	}

	// Run instructions
	for(SystemInstruction sys_instruction : m_waiting_instructions){
		if(sys_instruction.type == INSTRUCTION_ASSET){
			AssetInstruction& instruction = sys_instruction.asset_instruction;
			ResourceManager::ResourceType type = resource_manager.getResourceType(instruction.id);

			if(type == ResourceManager::RESOURCE_WIDGET_GROUP){
				/*
				Handling of widget groups
				*/

				if(instruction.type == LOAD_ASSET_BY_ID){
					assert(m_asset_manager.getAllLinkedRenderables(instruction.id).size() == 0);
					const std::vector<Widget>& widgets = resource_manager.widgetGroupReference(
						instruction.id);
					RenderableId renderable_id = m_asset_manager.initNewRenderable();
					m_asset_manager.linkRenderableToResource(renderable_id, instruction.id);
					m_asset_manager.setRenderableData(renderable_id, generateWidgetMesh(widgets));
				}else if(instruction.type == UNLOAD_ASSET_BY_ID){
					m_asset_manager.eraseResource(instruction.id);
				}else if(instruction.type == REFRESH_ASSET_BY_ID){
					// WARNING: Assumes that widget groups only have one renderable each.
					std::vector<RenderableId> renderables = m_asset_manager.getAllLinkedRenderables(
						instruction.id);
					assert(renderables.size() == 1);
					const std::vector<Widget>& widgets = resource_manager.widgetGroupReference(
						instruction.id);
					m_asset_manager.setRenderableData(renderables[0], generateWidgetMesh(widgets));
				}
				//m_asset_manager.printSummaryStatsDebugging();
			}else if(type == ResourceManager::RESOURCE_WIDGET_MESH){
				/*
				Handling of widget meshes
				*/

				if(instruction.type == LOAD_ASSET_BY_ID){
					const WidgetMesh& mesh = resource_manager.widgetMeshReference(instruction.id);
					RenderableId renderable_id = m_asset_manager.initNewRenderable();
					m_asset_manager.linkRenderableToResource(renderable_id, instruction.id);
					m_asset_manager.setRenderableData(renderable_id, mesh);
				}else if(instruction.type == UNLOAD_ASSET_BY_ID){
					m_asset_manager.eraseResource(instruction.id);
				}
			}
		}else if(sys_instruction.type == INSTRUCTION_RELOAD_ASSETS_FROM_FILES){
			initShadersFromSourceFiles();
		}else if(sys_instruction.type == INSTRUCTION_GENERAL_TEXT){
			// Toggle rendermode
			if(sys_instruction.text_instruction.text == RENDERER_LINES_COMMAND_STRING){
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}else if(sys_instruction.text_instruction.text == RENDERER_FACES_COMMAND_STRING){
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			// Toggle widget visibility
			if(sys_instruction.text_instruction.text == RENDERER_SHOW_WIDGETS){
				m_render_widgets = true;
			}else if(sys_instruction.text_instruction.text == RENDERER_HIDE_WIDGETS){
				m_render_widgets = false;
			}

			// Reload shaders from files
			if(sys_instruction.text_instruction.text == RENDERER_RELOAD_SHADERS_FROM_FILE){
				initShadersFromSourceFiles();
			}

			// Generate all meshes
			if(sys_instruction.text_instruction.text == RENDERER_SCHEDULE_MESHING){
				this->scheduleChunkMeshing(state);
			}
		}else if(sys_instruction.type == INSTRUCTION_CHUNK){
			ChunkInstruction& c_instruct = sys_instruction.chunk_instruction;
			if(c_instruct.type == CHUNK_GENERATE_UNLIT_MESH){
				m_priority_queue.setLeniency(c_instruct.lod_addr, c_instruct.leniency);
			}else{
				assert(false);
			}
		}
	}

	if(m_waiting_instructions.size() > 0){
		m_waiting_instructions.clear();	
	}
}

void OpenGLModule::render(const SimCache& state, const Camera& camera, 
	const RendererSettings& settings){
	/*

	*/
	//printf("<OpenGL Render>\n");
	//printf("\tRendering Started: %u\n", glGetError());
	const WorldState* world = state.m_reference_world;
	const ResourceManager* resource_manager = state.m_resource_manager;

	// For every widget group that doesn't have any renderables yet, send instructions to
	// do so. Then, process all stored instructions at once.
	// TODO: Profile to see if doing this every frame is a performance issue.
	constexpr ResourceManager::ResourceType RESOURCE_TYPES[] = {
		ResourceManager::RESOURCE_WIDGET_GROUP, 
		ResourceManager::RESOURCE_WIDGET_MESH
	};
	for(int i = 0; i < 2; ++i){
		ResourceManager::ResourceType type = RESOURCE_TYPES[i];
		std::vector<ResourceManager::ResourceId> ids_of_type = 
			resource_manager->allResourcesOfType(type);
		
		for(ResourceId res_id : ids_of_type){
			std::vector<RenderableId> render_ids = m_asset_manager.getAllLinkedRenderables(res_id);
			if(render_ids.size() == 0){
				SystemInstruction initial_load_instruction;
				initial_load_instruction.type = INSTRUCTION_ASSET;
				initial_load_instruction.asset_instruction = {LOAD_ASSET_BY_ID, res_id};

				this->sendInstruction(initial_load_instruction);
			}
		}
	}
	this->processStoredInstructions(state);
	this->scheduleChunkMeshing(state);

	// Generate matrices
	FMat4 view_matrix = MathUtils::Matrix::makeViewMatrix(camera);
	FMat4 projection_matrix = MathUtils::Matrix::makePerspectiveProjectionMatrix(camera);
	FMat4 identity = MathUtils::Matrix::initIdentityMatrix();

	// Render all Widgets
	//printf("\tBeginning widget rendering...\n");
	m_widget_shader->use();
	m_widget_shader->setTime(world->currentTime());
	m_widget_shader->setViewMatrix(view_matrix);
	m_widget_shader->setProjectionMatrix(projection_matrix);
	m_widget_shader->setCameraPos(camera.pos);
	m_widget_shader->setFogColor(world->m_fog_color);
	constexpr ResourceManager::ResourceType WIDGET_RESOURCE_TYPES[] = {
		ResourceManager::RESOURCE_WIDGET_GROUP, 
		ResourceManager::RESOURCE_WIDGET_MESH
	};
	for(int i = 0; i < 2 && m_render_widgets; ++i){
		ResourceManager::ResourceType type = WIDGET_RESOURCE_TYPES[i];
		for(ResourceManager::ResourceId res_id : resource_manager->allResourcesOfType(type)){
			// If there's a transform, use it now. Otherwise fall back to the identity.
			ResourceManager::ResourceTransform transform = 
				resource_manager->resourceTransform(res_id);
			FMat4 model_matrix = identity;
			if(transform.is_valid){
				model_matrix = MathUtils::Matrix::makeModelMatrix(transform.position);
			}
			m_widget_shader->setModelMatrix(model_matrix);

			// Now go through all renderables for this resource
			std::vector<RenderableId> render_ids = m_asset_manager.getAllLinkedRenderables(res_id);
			for(RenderableId renderable_id : render_ids){
				RenderableBufferMetadata metadata = m_asset_manager.getMetadata(renderable_id);
				glBindVertexArray(metadata.vao);
				glDrawElements(
					GL_LINES,
					metadata.num_indices, 
					GL_UNSIGNED_INT, 
					(void*)0);
				//printf("\t\tRendered item with %u indices.\n", metadata.num_indices);
			}
		}
	}
	m_widget_shader->stop();

	// Render all Chunks
	//printf("\tPre-Chunk: %u\n", glGetError());
	m_chunk_shader->use();
	m_chunk_shader->setTime(world->currentTime());
	m_chunk_shader->setViewMatrix(view_matrix);
	m_chunk_shader->setProjectionMatrix(projection_matrix);
	m_chunk_shader->setCameraPos(camera.pos);
	m_chunk_shader->setFogColor(world->m_fog_color);
	constexpr ResourceManager::ResourceType CHUNK_RESOURCE_TYPES[] = {
		ResourceManager::RESOURCE_CHUNK_MESH_UNLIT, 
		ResourceManager::RESOURCE_CHUNK_MESH_LIT
	};
	for(int i = 0; i < 2; ++i){
		ResourceManager::ResourceType type = CHUNK_RESOURCE_TYPES[i];
		std::vector<CellAddress>& renderable_cells = m_renderable_cells_by_type[type];

		for(CellAddress cell_addr : renderable_cells){
			IVec3 voxel_pos = cell_addr.corner_addr * CHUNK_LEN;
			FMat4 model_matrix = MathUtils::Matrix::makeModelMatrix(toFloatVector(voxel_pos));
			m_widget_shader->setModelMatrix(model_matrix); ????????

			for(RenderableId renderable_id : m_asset_manager.getAllLinkedRenderables(cell_addr)){
				RenderableBufferMetadata metadata = m_asset_manager.getMetadata(renderable_id);
				glBindVertexArray(metadata.vao);
				glDrawElements(
					GL_TRIANGLES,
					metadata.num_indices, 
					GL_UNSIGNED_INT, 
					(void*)0);
			}
		}
	}
	m_chunk_shader->stop();
	//printf("\tPost-Chunk: %u\n", glGetError());
	//printf("\tRendered %i chunks\n", count);

	// TODO: Render all Entities

	// TODO: Render all Grid Entities


	//printf("</OpenGL Render>\n\n\n");
}

void OpenGLModule::handleChunkMeshRenderable(CellAddress mesh_addr, const ChunkVoxelMesh& mesh){

	// Get RenderableId if it already exists
	RenderableId renderable_id = INVALID_RENDERABLE_ID;
	std::vector<CellAddress>& renderables = 
		m_renderable_cells_by_type[ResourceManager::RESOURCE_CHUNK_MESH_UNLIT];
	for(CellAddress addr : renderables){
		if(addr == mesh_addr){
			std::vector<RenderableId> linked_ids = m_asset_manager.getAllLinkedRenderables(addr);
			assert(linked_ids.size() == 1);
			renderable_id = linked_ids[0];
		}
	}

	// If there wasn't a renderable, init one now.
	if(renderable_id == INVALID_RENDERABLE_ID){
		renderable_id = m_asset_manager.initNewRenderable();
		m_asset_manager.linkRenderableToCell(renderable_id, mesh_addr);
		renderables.push_back(mesh_addr);
	}

	ChunkVoxelMeshOpengl opengl_mesh = MesherOpenGL::toOpenglFormat(mesh);
	m_asset_manager.setRenderableData(renderable_id, opengl_mesh);
}

int main(int argc, char** argv){
	int WORKER_GROUP_SIZE = 32;
	int NUM_JOBS = 2;
	if(argc > 1){
		NUM_JOBS = atoi(argv[1]);
	}
	if(argc > 2){
		WORKER_GROUP_SIZE = atoi(argv[2]);
	}

	WorkGroup group(WORKER_GROUP_SIZE);
	int completed_job_counts[WORKER_GROUP_SIZE] = {};

	// Some fake work to test the concept
	InputBlockA block_a = {-5};
	InputBlockB block_b = {3.141592};
	void* input_pointers[] = {&block_a, &block_b};
	WorkGroup::WorkFunction functions[] = {workFuncA, workFuncB};
	int struct_sizes[] = {
		sizeof(InputBlockA),
		sizeof(InputBlockB),
	};

	std::vector<JobDescription> job_descriptions = randomJobs(NUM_JOBS);
	assert(job_descriptions.size() == NUM_JOBS);

	// Collect results as they come
	printf("Running job loop...\n");
	int num_jobs_completed = 0;
	int num_jobs_launched = 0;
	while(num_jobs_completed < NUM_JOBS){
		sleepSeconds(1);  // Pretending other processes are taking place

		// Collect stats
		std::vector<WorkGroup::WorkerId> waiting_workers = group.waitingWorkers();
		int num_workers_waiting = waiting_workers.size();
		int num_workers_available = group.numWorkersAvailable();
		
		// Print some stats
		printf("-----------------------------------\n");
		printf("New loop.\n-%i jobs left.\n", NUM_JOBS - num_jobs_launched);
		printf("-Workers running: %i\n", WORKER_GROUP_SIZE - num_workers_available - 
			num_workers_waiting);
		printf("-Workers waiting: %i\n", num_workers_waiting);
		printf("-Workers available: %i\n", num_workers_available);
		
		// Launch new jobs for every empty slot
		// Yeah, I know it would be better to collect THEN launch, but this makes counts
		// easier to track between loops.
		int num_jobs_available = NUM_JOBS - num_jobs_launched;
		int num_attempted_launches = std::min(num_workers_available, num_jobs_available);
		if(num_attempted_launches > 0){
			printf("\n-Attempting to launch %i jobs\n", num_attempted_launches);
			int num_successful_launches = 0;
			for(int i = 0; i < num_attempted_launches; ++i){
				int option = job_descriptions[num_jobs_launched].func_num;

				WorkGroup::WorkerId worker_id = -1;
				if(option == 0){
					InputBlockA block = {num_jobs_launched};
					worker_id = group.launchJob(functions[option], &block, struct_sizes[option]);
				}else if(option == 1){
					InputBlockB block = {num_jobs_launched + 0.0001};
					worker_id = group.launchJob(functions[option], &block, struct_sizes[option]);
				}
				
				printf("\tAttempted to launch job '%i' with worker '%i'\n", 
					num_jobs_launched, worker_id);
				
				bool was_successful = worker_id != WorkGroup::INVALID_WORKER_ID;
				num_successful_launches += was_successful;
				num_jobs_launched += was_successful;
			}
			printf("-Launched (%i/%i) jobs successfully\n", num_successful_launches, 
				num_attempted_launches);
		}

		// Collect finished work
		if(num_workers_waiting > 0){
			printf("\n-Collecting completed work...\n");
			for(auto worker_id : waiting_workers){
				WorkGroup::ScratchMemory* output_scratch = group.workerOutput(worker_id);
				printf("\tCollecting output from worker '%i': ", worker_id);
				JobOutput output = *((JobOutput*)output_scratch->memoryPtr());
				if(output.type == TYPE_A){
					printf("A<%i>", output.block_a.int_data);
				}else if(output.type == TYPE_B){
					printf("B<%f>", output.block_b.double_data);
				}else{
					printf("UNKNOWN!");
				}
				printf("\n");


				++num_jobs_completed;
				++completed_job_counts[worker_id];
				group.markAsAvailable(worker_id);
			}
		}

		printf("-----------------------------------\n\n");
	}
	printf("(%i/%i) jobs completed\n", num_jobs_completed, NUM_JOBS);

	printf("Worker job counts total:\n");
	for(int i = 0; i < WORKER_GROUP_SIZE; ++i){
		printf("\tWorker #%03i: %03i ", i, completed_job_counts[i]);
		for(int x = 0; x < completed_job_counts[i]; ++x){
			printf("|");
		}
		printf("\n");
	}

	return 0;
}
#endif
