#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <stdint.h>

#define IMAGE_WIDTH 25
#define IMAGE_HEIGHT 25
#define MAX_RUNS 0

typedef struct {
    uint8_t start;
    uint8_t length;
} Run;

const Run image_runs[IMAGE_HEIGHT][MAX_RUNS] = {
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
};

const uint8_t run_counts[IMAGE_HEIGHT] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

#endif
