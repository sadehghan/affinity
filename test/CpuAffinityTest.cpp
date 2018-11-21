#include <cmath>
#include <thread>

#include "CpuAffinity.h"

void thread_function()
{
	for (int i = 0; i < 1000000000; ++i)
	{
		for (int j = 0; j < 5000; ++j)
			pow(j, 5000);
		usleep(3);
	}
}

int main(int argc, const char* argv[])
{
	CpuAffinity cpu_affinity;

	uint32_t first_cpu_core = 0;
	uint32_t cpu_core;

	if (argc == 3)
		first_cpu_core = atoi(argv[2]);

	cpu_affinity.set_first_cpu_core(first_cpu_core);
	cpu_core = cpu_affinity.allocate_cpu();

	if ("P" == std::string(argv[1]))
	{
		std::thread test_thread(thread_function);
		test_thread.join();
	}
	else if ("S" == std::string(argv[1]))
	{
		std::thread test_thread(thread_function);

		cpu_affinity.set_cpu_affinity(test_thread, cpu_core);
		test_thread.join();
	}
}
