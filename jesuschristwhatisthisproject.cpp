#include <iostream>
#include "processor.h"

using namespace std;

int main(int argc, char* argv[]) {
	job_queue arrival_queue = job_queue_from_file("total_jobs.txt");

	for (uint32_t proc_count = 1; proc_count <= 10; proc_count++) {
		processor_context ctx = processor_context_make(proc_count, arrival_queue);

		while (ctx.time != 500) {
			processor_context_advance_time(ctx);
		}

		cout << processor_context_metrics_str(ctx) << endl;

		while (ctx.time != 10000) {
			processor_context_advance_time(ctx);
		}

		cout << processor_context_metrics_str(ctx) << endl;

	}
}