#include "job.h"

job job_make(job_type type) {
	constexpr uint32_t arrival_range[][2] = {
		{ 4, 6 },	{ 6, 8 },	// A, B
		{ 4, 18 },	{ 4, 14 }	// C, D
	};

	constexpr uint32_t processing_range[][2] = {
		{ 1, 5 },	{ 2, 8 },	// A, B
		{ 6, 12 },	{ 2, 22 }	// C, D
	};

	job j = { type, 0, 0 };

	j.arrival_time = rand_range(
		arrival_range[type][0],
		arrival_range[type][1]
	);

	j.processing_time = rand_range(
		processing_range[type][0],
		processing_range[type][1]
	);

	return j;
}


job_queue job_queue_make(
	uint32_t a,
	uint32_t b,
	uint32_t c,
	uint32_t d
) {
	job_queue queue;
	job j = { };

	uint32_t cumulative[4] = { 0, 0, 0, 0 };
	uint32_t counts[4] = { a, b, c, d };

	for (size_t i = 0; i != 4; i++) {

		job_type j_type = (job_type)i;

		while (counts[i]--) {
			j = job_make(j_type);

			cumulative[j_type] += j.arrival_time;
			j.arrival_time = cumulative[j_type];

			queue.push(j);
		}

	}

	return queue;
}


job_queue job_queue_from_file(const std::string& file) {
	std::ifstream f_in(file);
	
	job_queue j_queue;
	job j;
	char c;

	while (true) {
		f_in >> c;
		f_in >> j.arrival_time;
		f_in >> j.processing_time;

		c = (toupper(c) - 'A') % 4;

		j.type = (job_type)c;

		j_queue.push(j);

		if (f_in.eof()) {
			break;
		}
	}

	return j_queue;
}

void job_queue_to_file(const std::string& file, const job_queue& queue) {
	job_queue queue_cpy(queue);
	
	std::ofstream f_out(file);

	while (!queue_cpy.empty()) {
		job j = queue_cpy.top();
		queue_cpy.pop();
		f_out << "ABCD"[j.type] << '\t' << j.arrival_time << '\t' << j.processing_time << std::endl;
	}
}