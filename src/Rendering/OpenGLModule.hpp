#pragma once

#include "RenderingModules.hpp"
#include "GLIncludes.hpp"
#include "ChunkShader.hpp"
#include "WidgetShader.hpp"
#include "AssetManagerOpenGL.hpp"
#include "MathUtils.hpp"
#include "SystemMessages.hpp"
#include "MultiresGrid.hpp"
#include "WorkGroup.hpp"
#include "FileIO.hpp"
#include "ResourceManager.hpp"

#include <mutex>
#include <memory>
#include <queue>       // Used for the priorityqueue

//-------------------------------------------------------------------------------------------------
// Helper structures and functions
//-------------------------------------------------------------------------------------------------
static PODString RENDERER_LINES_COMMAND_STRING = PODString::init("SET RENDERMODE: LINES");
static PODString RENDERER_FACES_COMMAND_STRING = PODString::init("SET RENDERMODE: FILL");
static PODString RENDERER_SHOW_WIDGETS = PODString::init("RENDER_WIDGETS: TRUE");
static PODString RENDERER_HIDE_WIDGETS = PODString::init("RENDER_WIDGETS: FALSE");
static PODString RENDERER_SCHEDULE_GROUP_WORK = PODString::init("RENDERER: SCHEDULE GROUP WORK");
static PODString RENDERER_RELOAD_SHADERS_FROM_FILE = PODString::init("RENDERER: RELOAD SHADERS");
static PODString RENDERER_SCHEDULE_MESHING = PODString::init("RENDERER: SCHEDULE MESHING");
constexpr int NUM_RENDERER_WORKERS = 2;


struct ChunkMesherJobInput{
	CellAddress addr;
	MeshType output_mesh_type;

	// Super jank, but it gets the references through
	std::mutex* write_mutex_ptr;
	std::unordered_map<CellAddress, ChunkVoxelMesh*, PODHasher>* working_mesh_map_ptr;

	// YAGNI: See if storing this locally and passing a pointer is more performant
	RawVoxelChunk chunk_data;
};

struct ChunkMesherJobOutput{
	CellAddress addr;
	bool is_successful;
};

void runChunkMeshingJob(WorkGroup::ScratchMemory* input_ptr, WorkGroup::ScratchMemory* output_ptr);


//-------------------------------------------------------------------------------------------------
// Placeholder Priority Queue
//-------------------------------------------------------------------------------------------------
class JankPriorityQueue{
	/*
	NOTE: Leniency functions like "niceness" in *nix systems. The lower the leniency, the more
	important it is to for the function to complete. The system will block until all jobs of 
	leniency zero are complete.

	WARNING: Placeholder implementation!!
	TODO: Move to a separate file and add template support.
	*/

	public:
		struct PriorityPair{
			CellAddress addr;
			Leniency leniency;
		};

		struct PairSorter{
			inline bool operator()(const PriorityPair& a, const PriorityPair& b) const{
				/*
				NOTE: This is the opposite of <, which is the default for a
					max heap.
				*/

				return a.leniency > b.leniency;
			}
		};

	public:
		JankPriorityQueue();
		~JankPriorityQueue();

		void setLeniency(CellAddress addr, Leniency leniency);
		PriorityPair popNext();
		PriorityPair peekNext();
		size_t size() const;

	private:
		std::priority_queue<PriorityPair, std::vector<PriorityPair>, PairSorter> m_queue;
};


//-------------------------------------------------------------------------------------------------
// OpenGL Module
//-------------------------------------------------------------------------------------------------
class OpenGLModule: public RenderingModule{
	/*
	NOTE: At the moment, the system will not allow multiple workers to run on the same cell address
		simultaneously. This means that if multiple orders exist for the same cell, the system
		will just terminate the instruction.

	NOTE: There are four main actions this module can take
		1) Set (identifier)
			The identifiers (CellAddress, ResourceHandle, etc) will be tracked for rendering. Their
			meshes will be built when the call is made, and rendered whenever the Render action is taken.
		2) Remove (identifier)
			Removes this identifier from the set of tracked identifiers.
		3) Render
			Go through the provided SimCache reference and render the relevant parts
			of the scene using only tracked resources/chunks/widget_groups etc. If something
			is not tracked, like a cell corresponding to a chunk mesh in view of the camera,
			it is simply not rendered.

	WARNING: SimCache is passed as const, but some functions lock the ChunkTable
		mutex when reading. This is undermines the entire point of const arguments.
	TODO: Figure out a way to keep as much const typing as possible while still having
		safe chunk updates. Possibly a state mutex that locks before rendering?
	*/

	public:
		OpenGLModule();
		~OpenGLModule();

		void initShadersFromSourceFiles();
		void sendInstruction(SystemInstruction instruction);
		void processStoredInstructions(const SimCache& state);

		void render(const SimCache& state, const Camera& camera, 
			const RendererSettings& settings);

	private:
		// Setting/Removing data types. These are controlled 
		// indirectly using SytemInstruction inputs or helper functions
		// internal to this class.
		void setData(const SimCache& state, ResourceHandle handle);
		void setData(const SimCache& state, CellAddress addr, const ChunkVoxelMesh& mesh);
		void setData(const SimCache& state, std::string name);
		void removeData(const SimCache& state, ResourceHandle handle);
		void removeData(const SimCache& state, CellAddress addr);
		void removeData(const SimCache& state, std::string name);
		
		// Chunk meshing
		void scheduleChunkMeshing(const SimCache& state);
		bool isMeshJobCurrentlyRunning(CellAddress addr, MeshType type);
		void handleChunkMeshRenderable(CellAddress addr, const ChunkVoxelMesh& mesh);
		
	private:
		// List of all shaders used by this module
		std::unique_ptr<BaseShader> m_widget_shader;
		std::unique_ptr<BaseShader> m_chunk_shader;

		// Settings
		bool m_render_widgets;

		// List of waiting instructions that need to be processed
		std::vector<SystemInstruction> m_waiting_instructions;

		// Working group uses meshes built in-place inside the m_working_meshes map
		WorkGroup m_work_group;
		JankPriorityQueue m_priority_queue;
		std::mutex m_working_mesh_mutex;
		std::unordered_map<CellAddress, ChunkVoxelMesh*, PODHasher> m_working_chunk_voxel_meshes;

		// Manages links between game resources and renderables
		AssetManagerOpenGL m_asset_manager;

		// Bookkeeping for tracked data
		std::unordered_set<ResourceHandle, PODHasher> m_tracked_resources;
		std::unordered_set<CellAddress, PODHasher> m_tracked_cells;
		std::unordered_set<std::string> m_tracked_widget_groups;
};
