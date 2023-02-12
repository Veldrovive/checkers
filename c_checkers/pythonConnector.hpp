#ifndef PYTHONCONNECTOR_H
#define PYTHONCONNECTOR_H

#include "minimax.hpp"
#include <string>
#include <cstring>

// Export getOptimalContinuation, isWinning, and getOptimalWinLength for use in Python.
extern "C" {
    void B_getOptimalContinuation(char* boardFile, char* outFile, int maxDepth, int maxTime){
        getOptimalContinuation(boardFile, outFile, maxDepth, maxTime);
    }
    bool B_isWinning(char* boardFile, int maxDepth, int maxTime){
        return isWinning(boardFile, maxDepth, maxTime);
    }
    int B_getOptimalWinLength(char* boardFile, int maxDepth, int maxTime){
        return getOptimalWinLength(boardFile, maxDepth, maxTime);
    }
    char* B_getOptimalContinuationFromString(const char* inBoard, char* output, std::size_t maxResultLength, int maxDepth, int maxTime){
        std::string res = getOptimalContinuationFromString(inBoard, maxDepth, maxTime);
        // Save a copy of the string in a char* and return it.
        strncpy(output, res.c_str(), maxResultLength);
        return output;
    }
    bool B_isWinningFromString(char* inBoard, int maxDepth, int maxTime){
        return isWinningFromString(inBoard, maxDepth, maxTime);
    }
    int B_getOptimalWinLengthFromString(char* inBoard, int maxDepth, int maxTime){
        return getOptimalWinLengthFromString(inBoard, maxDepth, maxTime);
    }
}

#endif
