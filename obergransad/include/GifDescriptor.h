#ifndef VERGIF
#define VERGIF 1
#include "stdint.h"

typedef struct {
    float DisplayTime;
    float FrameRate;
    uint32_t FrameCount;
    intptr_t FrameList;
} GifDescriptor;

#endif