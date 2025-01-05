#pragma once
#include <new>
#include <mutex>
#include <crtdbg.h>
#include <iostream>
#include <assert.h>



#define HYECS_LEAK_DETECTION

#if defined HYECS_LEAK_DETECTION
// #define LEAK_NEW_DETECTION
#endif

#if defined LEAK_NEW_DETECTION && defined HYECS_LEAK_DETECTION

bool g_printNewDelete = false;
constexpr size_t max_allocations = 1 << 12;
std::pair<void*, std::pair<size_t, int>> g_allocations[max_allocations];
std::mutex g_allocations_mutex{};
int g_allocation_counter = 0;
int g_allocation_index = 0;
thread_local bool g_tl_leak_exempt = false;

class ScopedPrintNewDelete {
public:
    ScopedPrintNewDelete(bool enable) {
        g_printNewDelete = enable;
    }
    ~ScopedPrintNewDelete() {
        g_printNewDelete = false;
        printAllocations();
    }

private:
    void printAllocations() {
        bool leaks_detected = false;
        for (int i = 0; i < max_allocations; ++i) {
            if (g_allocations[i].first != nullptr) {
                if (!leaks_detected) {
                    printf("Memory leaks detected:\n");
                    leaks_detected = true;
                }
                printf("Leaked %zu bytes at %p (Allocation #%d)\n", g_allocations[i].second.first, g_allocations[i].first, g_allocations[i].second.second);
            }
        }
        if (!leaks_detected) {
            printf("No memory leaks detected.\n");
        }
    }
};

class ScopedLeakExempt
{
    ScopedLeakExempt()
    {
        g_tl_leak_exempt = true;
    }
    ~ScopedLeakExempt()
    {
        g_tl_leak_exempt = false;
    }
};

void* operator new(std::size_t size) {
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    if (g_printNewDelete && !g_tl_leak_exempt) {
        std::lock_guard<std::mutex> lock(g_allocations_mutex);
        g_allocations[g_allocation_index] = {p, {size, g_allocation_counter++}};
        g_allocation_index = (g_allocation_index + 1) % max_allocations;
    }
    return p;
}

void operator delete(void* p) noexcept {
    if (g_printNewDelete && !g_tl_leak_exempt) {
        std::lock_guard<std::mutex> lock(g_allocations_mutex);
        for (int i = 0; i < max_allocations; ++i) {
            if (g_allocations[i].first == p) {
                g_allocations[i].first = nullptr;
                break;
            }
        }
    }
    std::free(p);
}

#endif

#if defined HYECS_LEAK_DETECTION && (defined _WIN32 || defined WIN64)

inline thread_local bool g_leak_detection_ignore = false;

class MemoryLeakExempt_t
{
public:
    MemoryLeakExempt_t() {
        prev_state = g_leak_detection_ignore;
        g_leak_detection_ignore = true;
    }

    ~MemoryLeakExempt_t() {
        g_leak_detection_ignore = prev_state;
    }
    bool  prev_state;
};

#define MemoryLeakExempt MemoryLeakExempt_t leak_exempt_scope_;

inline void* operator new(std::size_t size) {
    void* p = nullptr;
    if (!g_leak_detection_ignore)
        p = _malloc_dbg(size, _NORMAL_BLOCK, nullptr, -1);
    else
        p = _malloc_dbg(size, _CRT_BLOCK, nullptr, -1);
    if (!p) throw std::bad_alloc();
    return p;
}

inline int CrtDebugAllocHook(int allocType, void* userData, size_t size,
	int blockType, long requestNumber,
	const unsigned char* filename, int lineNumber)
{
	for (auto i : {4525})
	{
        if (requestNumber == i)
        {
            return 1;
        }
    }

	return 1;
}

struct MemoryLeakDetectInitializer {
    MemoryLeakDetectInitializer()
    {
        _CrtSetAllocHook(CrtDebugAllocHook);
    }
};
inline MemoryLeakDetectInitializer memoryLeakDetectInitializer;

class MemoryLeakDetector {
public:
    MemoryLeakDetector() {
        holder = _malloc_dbg(4, _CRT_BLOCK, __FILE__, __LINE__);
        _CrtMemCheckpoint(&memState_);
    }

	struct _CrtMemBlockHeader_Hack {
        // Pointer to the block allocated just before this one:
        _CrtMemBlockHeader_Hack* _block_header_next;
        // Pointer to the block allocated just after this one:
        _CrtMemBlockHeader_Hack* _block_header_prev;
        char const* _file_name;
        int _line_number;

        int _block_use;   // Type of block
        size_t _data_size;// Size of user block

        long _request_number;// Allocation number
    };

    ~MemoryLeakDetector() {
        _CrtMemState stateNow, stateDiff;
        _CrtMemCheckpoint(&stateNow);
        int diffResult = _CrtMemDifference(&stateDiff, &memState_, &stateNow);

        if (diffResult)
        {
            _CrtMemDumpStatistics(&stateDiff);
			//find out the diff block requestNumber
            _CrtMemBlockHeader_Hack* old = (_CrtMemBlockHeader_Hack*) memState_.pBlockHeader;
            _CrtMemBlockHeader_Hack* now = (_CrtMemBlockHeader_Hack*) stateNow.pBlockHeader;
            _CrtMemBlockHeader_Hack* begin = old->_block_header_prev;
            int leak_count = 0;
            while (begin != now)
            {
                if (begin->_block_use == _NORMAL_BLOCK)
                {
                    printf("Leak %ld: %llu bytes\n", begin->_request_number, begin->_data_size);
                    leak_count++;
                }
                begin = begin->_block_header_prev;
            } 
            if (begin && begin->_block_use == _NORMAL_BLOCK)
            {
                printf("Leak %ld: %llu bytes\n", begin->_request_number, begin->_data_size);
                leak_count++;
            }

            if(leak_count>0)
                reportFailure(stateDiff.lSizes[_NORMAL_BLOCK]);
            else
                reportDiff();

        }

        free(holder);
    }
private:
    void reportFailure(int64_t unfreedBytes);
    void reportDiff();

    _CrtMemState memState_;
    void* holder;
};





#endif


