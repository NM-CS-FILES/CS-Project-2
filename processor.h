#include <sstream>
#include <queue>

#include "job.h"

#pragma once


struct processor {
	bool busy;
	job current_job;
	uint32_t time;
};


struct processor_context_metrics {
	uint32_t processor_count;
	
	uint32_t current_queue_size;
	uint32_t average_queue_size;
	uint32_t max_queue_size;

	uint32_t total_time_jobs_in_queue;
	uint32_t average_time_jobs_in_queue;

	uint32_t jobs_arrived[4];
	uint32_t jobs_completed[4];

	uint32_t total_processing_time;
	uint32_t total_idle_time;
};


struct processor_context {
	std::vector<processor> processors;
	
	job_queue arrival_queue;
	job_queue waiting_queue;
	
	uint32_t time;

	processor_context_metrics metrics;
	std::ofstream logger;
};


//
//


processor_context processor_context_make(uint32_t proc_count, const job_queue& arrival);

std::string processor_context_metrics_str(const processor_context& proc_ctx);

void processor_context_advance_time(processor_context& proc_ctx);