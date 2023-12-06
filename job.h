#include <queue>
#include <string>
#include <fstream>

#define rand_range(_min, _max)	((_min) + rand() % ((_max) - (_min) + 1))

#pragma once


enum job_type {
	JOB_A,
	JOB_B,
	JOB_C,
	JOB_D
};


struct job {
	job_type type;
	uint32_t arrival_time;
	uint32_t processing_time;
};


struct job_comparator {
	inline bool operator()(const job& l, const job& r) {
		return (l.arrival_time == r.arrival_time) ?
			(r.type == JOB_D) : (l.arrival_time > r.arrival_time);
	}
};


using job_queue = std::priority_queue<job, std::vector<job>, job_comparator>;


job job_make(job_type type);


job_queue job_queue_make(
	uint32_t a,
	uint32_t b,
	uint32_t c,
	uint32_t d
);


job_queue job_queue_from_file(const std::string& file);


void job_queue_to_file(const std::string& file, const job_queue& queue);