#ifndef CPU_AFFINITY_H_
#define CPU_AFFINITY_H_

#include <thread>
#include <unistd.h>
#include <pthread.h>

/// A class for allocating CPU and setting CPU affinity
class CpuAffinity
{
public:
	CpuAffinity()
	: idle_cpu_core(0)
	{
		num_cpu_cores = static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN) - 1);
		default_cpu_core = num_cpu_cores;
	}

	/// Set default CPU core
	void set_default_cpu_core(uint32_t cpu_core)
	{
		default_cpu_core = cpu_core;
	}

	/// Set first idle CPU
	void set_first_cpu_core(uint32_t first_cpu_core)
	{
		idle_cpu_core = first_cpu_core;
	}

	/**
	 * Allocates a new CPU core
	 *
	 * @return allocated CPU number
	 */
	uint32_t allocate_cpu()
	{
		return idle_cpu_core++;
	}

	/// Sets CPU affinity for POSIX thread and return allocated CPU core number
	uint32_t set_cpu_affinity(pthread_t thread_id)
	{
		uint32_t cpu_core = allocate_cpu();
		set_cpu_affinity(thread_id, cpu_core);
		return cpu_core;
	}

	/// Sets CPU affinity for POSIX thread based on input CPU core number
	cpu_set_t set_cpu_affinity(pthread_t thread_id, uint32_t cpu_core)
	{
		cpu_set_t cpu_mask;
		fill_cpu_mask(cpu_mask, cpu_core);
		pthread_setaffinity_np(thread_id, sizeof(cpu_mask), &cpu_mask);
		return cpu_mask;
	}

	/// Sets CPU affinity for standard thread based and return allocated CPU core number
	uint32_t set_cpu_affinity(std::thread& thread)
	{
		return set_cpu_affinity(thread.native_handle());
	}

	/// Sets CPU affinity for standard thread based on input CPU core number
	cpu_set_t set_cpu_affinity(std::thread& thread, uint32_t cpu_core)
	{
		return set_cpu_affinity(thread.native_handle(), cpu_core);
	}

	/// Fills input cpu_mask based on cpu_core
	cpu_set_t fill_cpu_mask(cpu_set_t& cpu_mask, uint32_t cpu_core)
	{
		CPU_ZERO(&cpu_mask);

		if (cpu_core <= num_cpu_cores)
			CPU_SET(cpu_core, &cpu_mask);
		else
			CPU_SET(default_cpu_core, &cpu_mask);

		return cpu_mask;
	}

	uint32_t last_allocated_cpu()
	{
		return idle_cpu_core - 1;
	}

private:
	uint32_t idle_cpu_core;
	uint32_t num_cpu_cores;
	uint32_t default_cpu_core;
};

#endif
