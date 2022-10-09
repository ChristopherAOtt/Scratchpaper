#pragma once

#include "SystemMessages.hpp"
#include "MultiresGrid.hpp"
#include "Chunks.hpp"
#include "ChunkGenerator.hpp"
#include "Settings.hpp"

#include "Debug.hpp"

#include <memory>
#include <vector>

//-------------------------------------------------------------------------------------------------
// Chunk Manager
//-------------------------------------------------------------------------------------------------
struct ChunkManagerConfig{
	int max_parallel_generators;
	int max_loaded_chunks;

	struct{
		PODString algorithm_name;
		Bytes8 seed;
	}generator_settings;
};

class ChunkManager{
	public:
		ChunkManager();
		ChunkManager(std::weak_ptr<Settings> settings);
		~ChunkManager();

		//static ChunkManagerConfig initDefaultConfig();
		void addInstruction(ChunkInstruction instruction);
		void processInstructions();
		void setManagedTable(ChunkTable& table);
		void updateWithSettings(std::weak_ptr<Settings> settings_ptr);

	private:
		// Chunk Instruction Handling
		void generateChunks(std::vector<IVec3>& coords);
		void eraseChunks(std::vector<IVec3>& coords);
		void loadChunksFromFile(std::vector<IVec3>& coords);
		void saveChunksToFile(std::vector<IVec3>& coords);
		void eraseChunksFromFile(std::vector<IVec3>& coords);
		
		// Init functions
		void initNoiseLayers(NoiseLayers* layers_algorithm);

	private:
		std::weak_ptr<Settings> m_settings_ptr;
		ChunkGenerator m_generator;
		ChunkTable* m_managed_table_ptr;
		std::vector<ChunkInstruction> m_waiting_instructions;
};


