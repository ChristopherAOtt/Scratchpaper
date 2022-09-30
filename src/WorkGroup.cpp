#include "WorkGroup.hpp"

//-------------------------------------------------------------------------------------------------
// Scratch Memory
//-------------------------------------------------------------------------------------------------
WorkGroup::ScratchMemory::ScratchMemory(){

}

WorkGroup::ScratchMemory::~ScratchMemory(){
	free(m_memory_ptr);
}

void* WorkGroup::ScratchMemory::memoryPtr(){
	return m_memory_ptr;
}

void WorkGroup::ScratchMemory::writeData(void* data_ptr, size_t num_bytes){
	/*
	Copies a POD structure into scratch memory
	*/

	assert(num_bytes >= 0);
	if(!m_memory_ptr || m_num_bytes < num_bytes){
		m_memory_ptr = realloc(m_memory_ptr, num_bytes);
	}
	memcpy(m_memory_ptr, data_ptr, num_bytes);
}

//-------------------------------------------------------------------------------------------------
// Worker Group
//-------------------------------------------------------------------------------------------------
//-----------------------------------------------
// Public
//-----------------------------------------------
WorkGroup::WorkGroup(): m_communication_blocks(DEFAULT_WORK_SLOT_COUNT){
	init(DEFAULT_WORK_SLOT_COUNT);
}

WorkGroup::WorkGroup(int num_work_slots): m_communication_blocks(num_work_slots){
	init(num_work_slots);
}

WorkGroup::~WorkGroup(){
	free(m_communication_blocks_ptr);
}

void workerJobFunction(WorkGroup::WorkerJob job){
	/*

	NOTE: STATUS_RUNNING is set in initJob() before the thread launches to prevent multiple
		jobs being launched from the same slot.
	*/

	job.work_function(job.input_scratch_block, job.output_scratch_block);
	job.communication_block->worker_status = WorkGroup::STATUS_AWAITING_PICKUP;
}

WorkGroup::WorkerId WorkGroup::launchJob(WorkFunction work_func, void* input_data_ptr, 
	size_t input_data_size){
	/*
	Attempts to start a job with the given work function and input data pointer.
	If successful, it returns the id of the worker. If not, it returns an invalid id. This
	happens if the function is called when the group is full.

	NOTE: Attempts to avoid bias in which worker it launches jobs for by rotating through
		starting positions for the work slots.
	*/

	WorkerId chosen_worker = INVALID_WORKER_ID;
	for(int i = 0; i < m_num_work_slots; ++i){
		int slot_index = (m_rotating_launch_index + i) % m_num_work_slots;
		if(m_communication_blocks[slot_index].worker_status == STATUS_AVAILABLE){
			// Update bookkeeping
			m_rotating_launch_index = (slot_index + 1) % m_num_work_slots;
			chosen_worker = slot_index;
			m_input_scratch_blocks[slot_index].writeData(input_data_ptr, input_data_size);

			// Launch thread.
			WorkerJob job_struct = {
				.input_scratch_block=&m_input_scratch_blocks[slot_index],
				.output_scratch_block=&m_output_scratch_blocks[slot_index],
				.work_function=work_func,
				.worker_id=slot_index,
				.communication_block=&m_communication_blocks[slot_index]
			};
			m_communication_blocks[slot_index].worker_status = STATUS_RUNNING;
			m_worker_threads[slot_index] = std::thread(workerJobFunction, job_struct);
			m_worker_threads[slot_index].detach();
			break;
		}
	}

	return chosen_worker;
}

std::vector<WorkGroup::WorkerId> WorkGroup::waitingWorkers() const{
	/*
	Returns a vector with the ids of every worker waiting with a completed job.
	*/

	std::vector<WorkerId> waiting_workers;
	for(int i = 0; i < m_num_work_slots; ++i){
		if(m_communication_blocks[i].worker_status == STATUS_AWAITING_PICKUP){
			waiting_workers.push_back(i);
		}
	}

	return waiting_workers;
}

int WorkGroup::numWorkersAvailable() const{
	/*
	Returns a count of how many workers are available to start jobs. 
	NOTE: This does not include workers with STATUS_AWAITING_PICKUP. These need to be
		explicitly told by markAsAvailable() to change their status.
	*/

	int num_available = 0;
	for(int i = 0; i < m_num_work_slots; ++i){
		if(m_communication_blocks[i].worker_status == STATUS_AVAILABLE){
			num_available += 1;
		}
	}
	return num_available;
}

WorkGroup::ScratchMemory* WorkGroup::workerOutput(WorkerId id){
	/*
	Give the caller reference to the worker's scratch memory so they can collect the 
	output at their leisure. This will be held until markAsAvailable() is called for this
	worker.
	*/

	WorkGroup::ScratchMemory* output_scratch_ptr = NULL;
	if(m_communication_blocks[id].worker_status == STATUS_AWAITING_PICKUP){
		output_scratch_ptr = &m_output_scratch_blocks[id];
	}

	return output_scratch_ptr;
}

bool WorkGroup::markAsAvailable(WorkerId id){
	/*
	Tells a worker that was awaiting pickup that they can be considered available again.
	Any data that they had waiting for pickup has been processed.

	Returns true if the worker had a status of STATUS_AWAITING_PICKUP. 
	TODO: Remove assert once testing is complete.
	*/

	assert(id >= 0 && id < m_num_work_slots);
	bool was_waiting = m_communication_blocks[id].worker_status == STATUS_AWAITING_PICKUP;
	assert(was_waiting);
	if(was_waiting){
		m_communication_blocks[id].worker_status = STATUS_AVAILABLE;
	}
	return was_waiting;
}


//-----------------------------------------------
// Private
//-----------------------------------------------
void WorkGroup::init(int num_work_slots){
	m_num_work_slots = num_work_slots;
	m_rotating_launch_index = 0;

	m_input_scratch_blocks.resize(num_work_slots);
	m_output_scratch_blocks.resize(num_work_slots);
	m_worker_threads.resize(num_work_slots);
	m_communication_blocks_ptr = (CommunicationBlock*) malloc(
		num_work_slots * sizeof(CommunicationBlock));

	// Mark these as available for work
	for(int i = 0; i < m_num_work_slots; ++i){
		m_communication_blocks_ptr[i].worker_status = STATUS_AVAILABLE;
		m_communication_blocks[i].worker_status = STATUS_AVAILABLE;
	}
}
