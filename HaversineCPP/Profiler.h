#pragma once
#ifdef _WIN32
#include <intrin.h>
#include <windows.h>
#include <Psapi.h>
#else
#include <x86intrin.h>
#include <stdio.h>
#include <sys/resource.h>
#endif

#include <mutex>

typedef std::uint64_t u64;
typedef std::uint8_t u8;

inline u64 GuesstimateCPUFrequency()
{
	auto dur = std::chrono::duration(std::chrono::milliseconds(100));
	auto start = std::chrono::high_resolution_clock::now();
	u64 nowCycles = __rdtsc();

	while (std::chrono::high_resolution_clock::now() - start < dur);

	u64 afterCycles = __rdtsc() + 1;

	return (afterCycles - nowCycles) * 10;
}

struct OSHandler
{
	OSHandler()
	{
#ifdef _WIN32
		procHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
#else

#endif

	}

	u64 GetPageFaults()
	{
#ifdef _WIN32
		PROCESS_MEMORY_COUNTERS_EX mem;
		mem.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);

		GetProcessMemoryInfo(procHandle, reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&mem), sizeof(mem));

		return mem.PageFaultCount;
#else
		rusage usage;
		getrusage(RUSAGE_SELF, &usage);
		return usage.ru_minflt + usage.ru_majflt;
#endif
	}
#ifdef _WIN32
	HANDLE procHandle;
#else

#endif
};

inline std::string humanSize(u64 bytes)
{
	//adapted from https://gist.github.com/dgoguerra/7194777
	const char* suffix[] = { "B", "KB", "MB", "GB", "TB" };
	const char length = sizeof(suffix) / sizeof(suffix[0]);

	int i = 0;
	double dblBytes = static_cast<double>(bytes);

	if (bytes > 1024) {
		for (i = 0; (bytes / 1024) > 0 && i < length - 1; i++, bytes /= 1024)
			dblBytes = bytes / 1024.0;
	}

	static char output[200];
	snprintf(output, sizeof(output), "%.02lf %s", dblBytes, suffix[i]);
	return output;
}

struct TraceAnchor
{
	u64 total_elapsed = 0;
	u64 num_occurances = 0;
	u64 bytes_processed = 0;
	const char* label = nullptr;
	int parent = -1;
	u64 num_page_faults = 0;
};

struct TU_Profiler;

struct GlobalProfiler
{
	static std::mutex profilers_mutex;
	static std::vector<TU_Profiler*> profilers;

	static void print_stats(u64 cpu_frequency);
};

const size_t num_anchors_per_TU = 128;

struct TU_Profiler
{
	TU_Profiler()
	{
		std::lock_guard<std::mutex> guard(GlobalProfiler::profilers_mutex);
		GlobalProfiler::profilers.push_back(this);
	}

	TraceAnchor anchors[num_anchors_per_TU];
	int cur_anchor = -1;
	OSHandler handler;
};

struct Trace
{
	Trace(const char* label, int _id, TU_Profiler* _profiler, u64 bytes_processed)
	{
		profiler = _profiler;
		id = _id;
		profiler->anchors[id].label = label;
		profiler->anchors[id].parent = profiler->cur_anchor;
		profiler->anchors[id].bytes_processed += bytes_processed;
		start_page_faults = profiler->handler.GetPageFaults();
		profiler->cur_anchor = id;
		start = __rdtsc();
	}

	~Trace()
	{
		profiler->anchors[id].total_elapsed += __rdtsc() - start;
		profiler->anchors[id].num_occurances++;
		profiler->anchors[id].num_page_faults = profiler->handler.GetPageFaults() - start_page_faults;
		profiler->cur_anchor = profiler->anchors[id].parent;
	}

	u64 start;
	int id;
	u64 start_page_faults;
	TU_Profiler* profiler;
};

inline void GlobalProfiler::print_stats(u64 cpu_frequency)
{
	std::lock_guard<std::mutex> guard(GlobalProfiler::profilers_mutex);
	std::string tabs;
	tabs.reserve(8);
	std::string percentStr;
	percentStr.reserve(16);

	for (size_t i = 0; i < profilers.size(); i++)
	{
		TU_Profiler* curprof = profilers[i];

		if (curprof)
		{
			for (size_t j = 0; j < num_anchors_per_TU; j++)
			{
				TraceAnchor& curanch = curprof->anchors[j];

				if (curanch.label)
				{
					int parent = curanch.parent;

					if (parent >= 0)
					{
						double percent = 100.0 * static_cast<double>(curanch.total_elapsed) / static_cast<double>(curprof->anchors[parent].total_elapsed);

						std::format_to(std::back_inserter(percentStr), " ({:.2f}%)", percent);
					}

					while (parent >= 0)
					{
						tabs.push_back('\t');
						parent = curprof->anchors[parent].parent;
					}

					double seconds = static_cast<double>(curanch.total_elapsed) / cpu_frequency;
					
					std::cout << tabs << curanch.label << " total: " << curanch.total_elapsed << 
						" (" << seconds << "s)";

					std::cout << percentStr;

					if (curanch.num_occurances > 1)
					{
						std::cout << ", samples: " << curanch.num_occurances
							<< ", avg: " << curanch.total_elapsed / curanch.num_occurances;
					}

					if (curanch.bytes_processed > 0)
					{
						std::string bytestr = humanSize(curanch.bytes_processed);
						std::string ratestr = humanSize(static_cast<u64>(static_cast<double>(curanch.bytes_processed) / seconds));

						std::cout << ", bytes processed: " << bytestr <<
							", throughput: " << ratestr << 's';
					}

					if (curanch.num_page_faults > 0)
					{
						std::cout << ", page faults: " << curanch.num_page_faults;
					}

					std::cout << std::endl;
					tabs.clear();
					percentStr.clear();
				}
			}
		}
	}
}

#define PASTE(a,b) a##b
#define PASTE_EXPANDED(a,b) PASTE(a,b)

#define PROFILED_FILE thread_local TU_Profiler add_PROFILED_FILE_to_the_top_of_this_file
#define SCOPED_TRACE(LABEL, BYTES_PROCESSED) Trace PASTE_EXPANDED(temp_trace, __LINE__)(LABEL, __COUNTER__, &add_PROFILED_FILE_to_the_top_of_this_file, BYTES_PROCESSED)