#ifndef SNITCH_ACCEL_H
#define SNITCH_ACCEL_H

#include <string.h>
#include "snrt.h"

typedef uint32_t meta_t;
#define META_SIZE (16)
#define META_SIZE_BYTES (META_SIZE * sizeof(meta_t))

static inline uint32_t fp32_to_u32 (float _f) {
    float f = _f;
    uint32_t * uptr = (uint32_t *)(&f);
    return *uptr;
}

static inline float u32_to_fp32 (uint32_t _u) {
    uint32_t u = _u;
    float * fptr = (float *)(&u);
    return *fptr;
}

typedef struct {
    uint32_t op;
    uint32_t *data;
    uint32_t size;
    uint32_t is_valid;
} SnitchCoreData_t;

typedef struct {
    SnitchCoreData_t coreData[8];
} SnitchAccelData_t;

int h2a_has_request (void *_payload);
int h2a_get_data (void *_payload, SnitchAccelData_t *data);
int h2a_put_data_vector (void *_payload, uint32_t * data, unsigned size);

float task_fp32_mul_fact (float *data, unsigned size);
double task_fp64_mul_fact (double *data, unsigned size);
float task_fp32_max (float *data, uint32_t len);
void task_expf (float *input, size_t input_len);
void task_logf (float *input, size_t input_len);

void task_softmax(float *input, size_t input_len);

#endif SNITCH_ACCEL_H