#include "processor.h"

processor_context processor_context_make(uint32_t proc_count, const job_queue& arrival) {
	processor_context proc_ctx = { };

	proc_ctx.processors.resize(proc_count, { false, { }, 0 });
	proc_ctx.arrival_queue = arrival;
	proc_ctx.time = 0;

	proc_ctx.metrics = { };
	proc_ctx.metrics.processor_count = proc_count;

	proc_ctx.logger = std::ofstream(
		std::to_string(proc_count) + "-processors.log"
	);

	return proc_ctx;
}


std::string processor_context_metrics_str(const processor_context& proc_ctx) {
	// yea, decided to cheap out on the formatting
	// no <iomanip> or `printf()` needed, this project
	// is convoluted enough, as-is
	
	const processor_context_metrics& metrics = proc_ctx.metrics;

	std::stringstream ss;

	ss << "Number of processor(s) being used: " << metrics.processor_count << std::endl;
	ss << "Current queue size: " << metrics.current_queue_size << std::endl;
	ss << "Average queue size: " << ((float)metrics.average_queue_size / proc_ctx.time) << std::endl;
	ss << "Maxiumum jobs in queue: " << metrics.max_queue_size << std::endl;
	ss << "Total time jobs are in queue:   " << metrics.total_time_jobs_in_queue << " time units\n";
	ss << "Average time jobs are in queue: " << ((float)metrics.average_time_jobs_in_queue / proc_ctx.time) << " time units\n";

	uint32_t total_jobs_completed = 0;

	for (uint32_t i = 0; i != 4; i++) {
		ss << "Total number of " << (char)('A' + i) << " jobs arrived:   " << metrics.jobs_arrived[i] << std::endl;
		ss << "Total number of " << (char)('A' + i) << " jobs completed: " << metrics.jobs_completed[i] << std::endl;

		total_jobs_completed += metrics.jobs_completed[i];
	}

	ss << "Total jobs completed: " << total_jobs_completed << std::endl;
	ss << "Total time CPU(s) were processing: " << metrics.total_processing_time << " time units\n";
	ss << "Total time CPU(s) were idle:       " << metrics.total_idle_time << " time units\n";

	return ss.str();
}

// Good god what have I done, maybe this should have been more
// than a single function, to late now, it's just gonna be this way
// NOTE: jobs are not logged with their numbers or type numbers,
// that was an oversight I made when I began this project,
// it is not so much an explicit requirement as it is a property of
// the logging format in windswept.pdf, with how convoluted this
// project has gotten, I'm not going to bother adding properties
// where they really aren't needed, I would love it if there were
// more rigid outlines for these projects and if they're stated
// explicitly
void processor_context_advance_time(processor_context& proc_ctx) {
	proc_ctx.time += 1; // advance one unit of time

	// advance each processor one unit of time
	for (processor& proc : proc_ctx.processors) {
		proc.time += 1;

		if (proc.busy && proc.time >= proc.current_job.processing_time) {
			// processor has completed job
			// job completed metric
			proc_ctx.metrics.jobs_completed[proc.current_job.type] += 1;

			proc.busy = false;
			proc.time = 0;

			// log the completion of a job
			proc_ctx.logger << "Time " << proc_ctx.time << ": Complete Processing Job: Job"
				<< "ABCD"[proc.current_job.type] << std::endl;
		}
	}

	// check if jobs have arrived
	while (!proc_ctx.arrival_queue.empty()) {
		job j = proc_ctx.arrival_queue.top();

		if (j.arrival_time <= proc_ctx.time) {
			proc_ctx.arrival_queue.pop();
			proc_ctx.waiting_queue.push(j);

			// job arrived metric
			proc_ctx.metrics.jobs_arrived[j.type] += 1;

			// log job arrival
			proc_ctx.logger << "Time " << proc_ctx.time << ": Arrival: Job " << "ABCD"[j.type]
				<< ", Processing Time " << j.processing_time << ';' << std::endl;
		}
		else {
			// break if job has not yet arrived
			break;
		}
	}

	// iterate through processors and assign waiting jobs 
	for (processor& proc : proc_ctx.processors) {
		if (proc_ctx.waiting_queue.empty()) {
			continue;
		}

		job j = proc_ctx.waiting_queue.top();

		if (!proc.busy) {
			proc_ctx.waiting_queue.pop();

			proc.busy = true;
			proc.current_job = j;
			proc.time = 0;

			// log the beginning of the processing of a job
			// hacky pointer arithmetic, again because I don't
			// feel like rewriting this loop
			proc_ctx.logger << "Time " << proc_ctx.time << ": Begin Processing Job: Job " 
				<< "ABCD"[j.type] << " in CPU " << ((((size_t)&proc - (size_t)&proc_ctx.processors[0]) / sizeof(processor)) + 1)
				<< std::endl;
		}
	}

	// handle "Highest Priority" waiting job
	if (!proc_ctx.waiting_queue.empty() && proc_ctx.waiting_queue.top().type == JOB_D) {
		uint32_t i = 0;

		// find the processor running a non-high priority job
		for (; i != proc_ctx.processors.size(); i++) {
			processor& proc = proc_ctx.processors[i];

			if (proc.current_job.type != JOB_D) {
				break;
			}
		}

		// make sure not all processors are running
		// high-priority jobs
		if (i != proc_ctx.processors.size()) {
			// subtract the processors running time
			// from the job's remaining processing time
			job j = proc_ctx.processors[i].current_job;
			j.processing_time -= proc_ctx.processors[i].time;

			// set processor job
			proc_ctx.processors[i].current_job = proc_ctx.waiting_queue.top();
			proc_ctx.processors[i].time = 0;
			proc_ctx.processors[i].busy = true;

			// remove high priority from queue
			proc_ctx.waiting_queue.pop();

			// insert back into waiting queue
			// job_comparator ensures ordering
			proc_ctx.waiting_queue.push(j);
		}
	}

	// here, the metric for total time processing / idle
	// was interpreted to mean, the total units of time
	// at least one processor was processing / idle
	uint32_t is_one_proc_busy = 0;
	uint32_t is_one_proc_idle = 0;

	for (processor& proc : proc_ctx.processors) {
		is_one_proc_busy = proc.busy ? 1 : is_one_proc_busy;
		is_one_proc_idle = !proc.busy ? 1 : is_one_proc_idle;
	}

	proc_ctx.metrics.total_processing_time += is_one_proc_busy;
	proc_ctx.metrics.total_idle_time += is_one_proc_idle;

	// handle queue metrics after assigning jobs to processors
	if (!proc_ctx.waiting_queue.empty()) {
		// total time jobs are waiting in queue metric
		proc_ctx.metrics.total_time_jobs_in_queue += 1;

		// average time jobs are waiting in queue metric
		// really an accumulation that gets divided when
		// processing as string
		proc_ctx.metrics.average_time_jobs_in_queue += 1;

		// current queue size metric
		proc_ctx.metrics.current_queue_size = proc_ctx.waiting_queue.size();

		// average queue size metric, really an accumulation that
		// gets divided when processing as a string
		proc_ctx.metrics.average_queue_size += proc_ctx.waiting_queue.size();

		// max queue size metric
		if (proc_ctx.waiting_queue.size() > proc_ctx.metrics.max_queue_size) {
			proc_ctx.metrics.max_queue_size = proc_ctx.waiting_queue.size();
		}
	}

	// log the synopsis, queue, cpus...
	proc_ctx.logger << "Time " << proc_ctx.time << ": ";
	proc_ctx.logger << "Queue: " << (proc_ctx.waiting_queue.empty() ? "Empty" : (std::to_string(proc_ctx.waiting_queue.size()) + " Jobs")) << ';';
	
	for (uint32_t i = 0; i != proc_ctx.processors.size(); i++) {
		processor& proc = proc_ctx.processors[i];
		proc_ctx.logger << " CPU " << (i + 1) << ' ';
		proc_ctx.logger << (proc.busy ? "Run" : "Idle") << " Time:" << proc.time << ';';
	}

	proc_ctx.logger << std::endl;
}