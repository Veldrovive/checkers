#ifndef PYTHONCONNECTOR_H
#define PYTHONCONNECTOR_H

#include "minimax.hpp"
#include <string>
#include <cstring>

extern "C" {
    char* B_getOptimalContinuationFromString(const char* inBoard, char* output, std::size_t maxResultLength, int maxDepth, int maxTime){
        std::string res = getOptimalContinuationFromString(inBoard, maxDepth, maxTime);
        // Save a copy of the string in a char* and return it.
        strncpy(output, res.c_str(), maxResultLength);
        return output;
    }
}

#endif
