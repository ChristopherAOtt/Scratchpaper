#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <stdint.h>
#include <cstdlib>
#include <cstring>  // For memcpy
#include <cassert>
#include <vector>

class WorkGroup{
	/*
	TODO: Add support for resizing the group at runtime
	TODO: Add ability to "lock" workers so the calling thread blocks until locked workers complete.
	*/

	public:  // Public Structure Declarations
		struct ScratchMemory;
		typedef void (*WorkFunction)(ScratchMemory* input_ptr, ScratchMemory* output_ptr);
		typedef int32_t WorkerId;
		constexpr static int DEFAULT_WORK_SLOT_COUNT = 4;
		constexpr static int INVALID_WORKER_ID = -1;

		struct ScratchMemory{
			public:
				ScratchMemory();
				~ScratchMemory();

				void* memoryPtr();
				void writeData(void* data_ptr, size_t num_bytes);

			private:
				size_t m_num_bytes{0};
				void* m_memory_ptr{NULL};
		};

	private:  // Private Structure Declarations
		enum WorkerStatus: uint8_t{
			STATUS_INVALID = 0,

			STATUS_AVAILABLE = 1,
			STATUS_RUNNING = 2,
			STATUS_EARLY_TERMINATION_SIGNALLED = 3,
			STATUS_AWAITING_PICKUP = 4,
		};

		struct CommunicationBlock{
			std::atomic<WorkerStatus> worker_status{STATUS_INVALID};
		};

		struct WorkerJob{
			/*
			Packages info for the worker thread
			*/

			ScratchMemory* input_scratch_block{NULL};
			ScratchMemory* output_scratch_block{NULL};
			WorkFunction work_function{NULL};

			WorkerId worker_id{-1};
			CommunicationBlock* communication_block{NULL};
		};

	public:  // Public Function Declarations
		WorkGroup();
		WorkGroup(int num_work_slots);
		~WorkGroup();

		WorkerId launchJob(WorkFunction work_func, void* input_data_ptr, size_t input_data_size);

		std::vector<WorkGroup::WorkerId> waitingWorkers() const;
		int numWorkersAvailable() const;
		ScratchMemory* workerOutput(WorkerId id);
		bool markAsAvailable(WorkerId id);
		
	private:  // Private Function Declarations
		void init(int num_work_slots);

		friend void workerJobFunction(WorkerJob job);  // Used by launched threads

	private:  // Private Member Variable Declarations
		int m_num_work_slots;
		int m_rotating_launch_index;

		std::vector<ScratchMemory> m_input_scratch_blocks;
		std::vector<ScratchMemory> m_output_scratch_blocks;

		CommunicationBlock* m_communication_blocks_ptr{NULL};  // For runtime resizing
		std::vector<CommunicationBlock> m_communication_blocks;

		std::vector<std::thread> m_worker_threads;
};
