
#include <benchmark/benchmark.h>
#include <windows.h>
#include <iostream>

#include "benchmark/auto_delegate_bench_mark.h"

using namespace std;


int main(int argc, char** argv)
{
    // Set thread priority to time-critical
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    // Pin thread to a specific CPU core
    SetThreadAffinityMask(GetCurrentThread(), (1 << 0));

#ifdef NDEBUG
    cout << "Running in release mode." << endl;
#else
    cout << "Running in debug mode." << endl;
#endif
    char arg0_default[] = "benchmark";
    char* args_default = arg0_default;
    if (!argv)
    {
        argc = 1;
        argv = &args_default;
    }
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    cout << "All Done" << endl;
    return 0;
}

