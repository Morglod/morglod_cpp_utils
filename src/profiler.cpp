#include "./profiler.hpp"

#include "./log.hpp"
#include "./utils.hpp"

#include <robin_hood.h>
#include <algorithm>
#include <vector>

struct CacheEntry {
    uint64_t counter_microsec = 0;
    uint64_t counter = 0;
    uint64_t counter_not_self_time_microsec = 0;
};

SHOULD_FIT_CACHE_LINE(CacheEntry)

// name -> CacheEntry
robin_hood::unordered_node_map<std::string, CacheEntry> __profiler_global_counters;
std::vector<std::string> __profiler_stack;

void profiler_start(std::string const& name) {
    __profiler_stack.emplace_back(name);
}

void profiler_end(std::string const& name, uint64_t const& time_microsec) {
    __profiler_stack.pop_back();

    __profiler_global_counters[name].counter_microsec += time_microsec;
    ++(__profiler_global_counters[name].counter);

    if (!__profiler_stack.empty()) {
        __profiler_global_counters[__profiler_stack.back()].counter_not_self_time_microsec += time_microsec;
    }
}

void profiler_print() {
    struct FinalResult {
        std::string name;
        CacheEntry e;
        double single_op_time;
        double single_op_time_self;
    };

    std::vector<FinalResult> final_results;
    final_results.reserve(__profiler_global_counters.size());

    for (auto [ name, entry] : __profiler_global_counters) {
        final_results.emplace_back(FinalResult{
            .name = name,
            .e = entry,
            .single_op_time = ((double)entry.counter_microsec / entry.counter),
            .single_op_time_self = ((double)(entry.counter_microsec - entry.counter_not_self_time_microsec) / entry.counter)
        });
    }

    std::sort(final_results.begin(), final_results.end(), [](FinalResult const& a, FinalResult const& b) {
        return a.single_op_time_self * a.e.counter > b.single_op_time_self * b.e.counter;
    });

    int result_name_max_length = 4;
    for (auto const& result : final_results) {
        result_name_max_length = std::max((int)result.name.size(), result_name_max_length);
    }

    KS_CRIT_LOG("profiler");
    KS_CRIT_LOG("name", std::string(result_name_max_length - 4 - 31, ' ') ,"self total\t\t\tself mid\t\t\ttotal\t\t\tmid\t\t\tcount");
    for (auto const& result : final_results) {
        fmt::print(stdout, "{} {:f}{} {:f}{} {:f}{} {:f}{} {}\n",
            // name
            result.name + std::string(result_name_max_length - result.name.size() + 2, ' '),
            // self total
            ((double)(result.e.counter_microsec - result.e.counter_not_self_time_microsec) / 1000.0),
            "\t\t",
            // self mid
            result.single_op_time_self / 1000.0,
            "\t\t\t",
            // total
            ((double)result.e.counter_microsec / 1000.0),
            "\t\t",
            // mid
            result.single_op_time / 1000.0,
            "\t\t\t",
            // count
            result.e.counter
        );
    }
}