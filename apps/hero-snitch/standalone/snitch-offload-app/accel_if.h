#ifndef SNITCH_ACCEL_H
#define SNITCH_ACCEL_H

#include <string.h>
#include "snrt.h"

typedef uint32_t meta_t;
typedef uint32_t data_t;
#define META_SIZE (16)
#define META_SIZE_BYTES (META_SIZE * sizeof(meta_t))

typedef struct {
    uint32_t op;
    data_t *data;
    uint32_t size;
    uint32_t is_valid;
} SnitchCoreData_t;

typedef struct {
    SnitchCoreData_t coreData[8];
} SnitchAccelData_t;

int h2a_get_data (void *_payload, SnitchAccelData_t *data);
int h2a_put_data_1 (void *_payload, data_t result);

uint32_t fp32_mul (uint32_t a, uint32_t b);

#endif SNITCH_ACCEL_H