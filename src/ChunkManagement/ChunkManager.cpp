#include "ChunkManager.hpp"


//-------------------------------------------------------------------------------------------------
// Chunk Manager
//-------------------------------------------------------------------------------------------------
ChunkManager::ChunkManager(){
	//ChunkManagerConfig default_config = initDefaultConfig();
	//init(default_config);
}

ChunkManager::ChunkManager(std::weak_ptr<Settings> settings){
	updateWithSettings(settings);
}

ChunkManager::~ChunkManager(){

}

/*
ChunkManagerConfig ChunkManager::initDefaultConfig(){
	ChunkManagerConfig config;
	config.max_parallel_generators = 4;  // TODO: Automatic core count detection.
	config.max_loaded_chunks = 2048;  // TODO: Better method of determining an appropriate number.

	config.generator_settings.algorithm_name = PODString::init("Default");
	config.generator_settings.seed = 0;

	return config;
}
*/

void ChunkManager::addInstruction(ChunkInstruction instruction){
	m_waiting_instructions.push_back(instruction);
}

void ChunkManager::processInstructions(){
	/*
	TODO: More efficient instruction batching

	WARNING: Does not take LOD levels into account
	*/

	assert(m_managed_table_ptr != NULL);

	std::unordered_map<ChunkInstructionType, std::vector<IVec3>> batched_instructions;
	for(ChunkInstruction instruction : m_waiting_instructions){
		batched_instructions[instruction.type].push_back(instruction.lod_addr.corner_addr);
	}
	m_waiting_instructions.clear();

	for(auto iter : batched_instructions){
		ChunkInstructionType instruction_type = iter.first;
		std::vector<IVec3>& batched_coords = iter.second;
		switch(instruction_type){
			case CHUNK_INVALID:
				continue;
			case CHUNK_GENERATE:
				generateChunks(batched_coords);
				break;
			case CHUNK_ERASE:
				eraseChunks(batched_coords);
				break;
			case CHUNK_LOAD_FROM_FILE:
				loadChunksFromFile(batched_coords);
				break;
			case CHUNK_SAVE_TO_FILE:
				saveChunksToFile(batched_coords);
				break;
			case CHUNK_ERASE_FROM_FILE:
				eraseChunksFromFile(batched_coords);
				break;
			default:
				printf("ERROR: Unrecognized chunk instruction type: %i\n", instruction_type);
				assert(false);
		}
	}
}

void ChunkManager::setManagedTable(ChunkTable& table){
	m_managed_table_ptr = &table;
}

void ChunkManager::updateWithSettings(std::weak_ptr<Settings> settings_ptr){
	/*
	Initializes the ChunkManager
	*/

	m_settings_ptr = settings_ptr;
	auto shared_ptr = settings_ptr.lock();
	assert(shared_ptr);

	Settings::Namespace& gen_settings = shared_ptr->namespaceRef("CHUNK_GEN");
	PODVariant name_data = gen_settings["GenerationAlgorithm"];
	PODVariant seed_data = gen_settings["Seed"];  // WARNING: Only 32bit int!
	assert(name_data.type == PODVariant::DATATYPE_STRING);
	assert(seed_data.type == PODVariant::DATATYPE_INT32);
	PODString name = name_data.val_string;
	Int32 seed = seed_data.val_int;
	printf("Running with seed %i\n", seed);

	GenerationAlgorithm* generator_ptr = NULL;
	if(name == "Default"){
		generator_ptr = new NoiseLayers;
	}else if(name == "NoiseLayers"){
		NoiseLayers* noise_layers = new NoiseLayers;
		initNoiseLayers(noise_layers);
		generator_ptr = noise_layers;
	}else if(name == "Fractal"){
		generator_ptr = new Fractal;
	}else if(name == "CenteredSphere"){
		generator_ptr = new CenteredSphere;
	}else if(name == "Scratch"){
		generator_ptr = new Scratch;
	}else if(name == "PillarsAndCaves"){
		generator_ptr = new PillarsAndCaves;
	}else if(name == "MeshTesting"){
		generator_ptr = new MeshTesting;
	}else{
		generator_ptr = new NoiseLayers;
	}

	std::shared_ptr<GenerationAlgorithm> default_algorithm(generator_ptr);
	m_generator.setAlgorithm(default_algorithm);
	m_generator.setSeed(seed);
	m_managed_table_ptr = NULL;
}

void printChunkList(std::vector<IVec3>& coords){
	/*
	Nonsense helper function to make function calls to the manager visible in the terminal.
	TODO: Delete once functions are filled out.
	*/
	for(Uint64 i = 0; i < coords.size(); ++i){
		printf("\t[%li]: ", i); printPODStruct(coords[i]); printf("\n");
	}
}

void ChunkManager::generateChunks(std::vector<IVec3>& coords){
	/*
	WARNING: Function not complete
	WARNING: Blindly overwrites any existing chunks at the given coordinates.
	*/
	//printf("Generating %li chunks:\n", coords.size());
	//printChunkList(coords);

	for(IVec3 coord : coords){
		RawVoxelChunk new_chunk = m_generator.generate(m_managed_table_ptr, coord);
		m_managed_table_ptr->setChunk(coord, new_chunk);
	}
}

void ChunkManager::eraseChunks(std::vector<IVec3>& coords){
	/*
	WARNING: Function not complete
	*/
	
	//printf("Erasing %li chunks:\n", coords.size());
	//printChunkList(coords);

	m_managed_table_ptr->eraseChunks(coords);
}

void ChunkManager::loadChunksFromFile(std::vector<IVec3>& coords){
	/*
	WARNING: Function not implemented
	*/
	
	//printf("Loading %li chunks:\n", coords.size());
	//printChunkList(coords);

	for(IVec3 coord : coords){
		
	}
}

void ChunkManager::saveChunksToFile(std::vector<IVec3>& coords){
	/*
	WARNING: Function not implemented
	*/
	
	//printf("Saving %li chunks:\n", coords.size());
	//printChunkList(coords);

	for(IVec3 coord : coords){
		
	}
}

void ChunkManager::eraseChunksFromFile(std::vector<IVec3>& coords){
	/*
	WARNING: Function not implemented
	*/
	//printf("Erasing %li chunks from file:\n", coords.size());
	//printChunkList(coords);

	for(IVec3 coord : coords){
		
	}
}

void ChunkManager::initNoiseLayers(NoiseLayers* layers_algorithm){
	/*
	TODO: Take inputs from a settings object instead of having them
		hardcoded in here.

		In the SETTINGS file, put something like:
		LayerList = [
			{
				Scale = {bla1, bla1, bla1};
				ValueRange = {this1, that1};
			},
			{
				Scale = {bla2, bla2, bla2};
				ValueRange = {this2, that2};
			},
			etc
		]
	*/

	constexpr Int32 NUM_LAYERS = 5;
	constexpr float SCALES[] = {
		5, 17, 67, 271, 773  // Try to keep these prime
	};

	constexpr IVec2 VALUE_RANGES[] = {
		{-5, 10},
		{-10, 20},
		{0, 20},
		{-60, 120},
		{-100, 200},
	};

	for(Int32 i = 0; i < NUM_LAYERS; ++i){
		FVec3 scale_vec = {SCALES[i], SCALES[i], SCALES[i]};

		NoiseLayers::Layer new_layer = {
			.scale=scale_vec,
			.value_range=VALUE_RANGES[i]
		};

		layers_algorithm->addNoiseLayer(new_layer);
	}
}
