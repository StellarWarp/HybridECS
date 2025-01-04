#pragma once

#if defined _WIN32 || defined_WIN64

#include <crtdbg.h>
#include <iostream>
#include <assert.h>
#include "ut.hpp"

class MemoryLeakDetector {
public:
    MemoryLeakDetector() {
        _CrtMemCheckpoint(&memState_);
    }

    ~MemoryLeakDetector() {
        _CrtMemState stateNow, stateDiff;
        _CrtMemCheckpoint(&stateNow);
        int diffResult = _CrtMemDifference(&stateDiff, &memState_, &stateNow);
        if (diffResult)
            reportFailure(stateDiff.lSizes[1]);
    }
private:
    void reportFailure(unsigned int unfreedBytes) {
        boost::ut::expect(unfreedBytes == 0) << "Memory leak of " << unfreedBytes << " byte(s) detected.";
        // assert(unfreedBytes == 0);
    }
    _CrtMemState memState_;
};

#else
class MemoryLeakDetector { };
#endif
