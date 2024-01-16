#ifndef SNITCH_ACCEL_H
#define SNITCH_ACCEL_H

#include <string.h>
#include "snrt.h"

#include "sa.h"

typedef uint32_t meta_t;
#define META_SIZE (16)
#define META_SIZE_BYTES (META_SIZE * sizeof(meta_t))

typedef struct {
    void *data;
    size_t size_bytes;
} mat_t;

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
    void *data;
    uint32_t size;
} SnitchCoreData_t;

int h2a_has_request (void *shared_mem);
void h2a_get_data (void *shared_mem, SnitchCoreData_t *data);
int h2a_put_data (void *shared_mem, const uint32_t * data, unsigned size);
int h2a_put_dummy_data(void *shared_mem);

float task_fp32_mul_fact (float *data, unsigned size);
float task_fp32_max (float *data, uint32_t len);
float task_fp32_div (float *data, uint32_t len);
void task_expf (float *input, size_t input_len);
void task_logf (float *input, size_t input_len);

void task_softmax(float *input, size_t input_len);

void * task_mat_mul (sa_prop_t *sa_prop, SnitchCoreData_t *core_data);

#endif /*SNITCH_ACCEL_H*/