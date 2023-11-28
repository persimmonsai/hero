#include "math.h"

#include "accel_if.h"

#define PER_CORE_MEM_OFFSET 0x100000
#define CORE_MEM_OFFSET(idx) ((idx) * PER_CORE_MEM_OFFSET)


static inline void * get_core_mem (void *_shared_mem) {
    uint32_t core_idx = snrt_global_core_idx();

    uint8_t * shared_mem = (uint8_t *)_shared_mem;

    return shared_mem + CORE_MEM_OFFSET(core_idx);
}

int h2a_has_request (void *shared_mem) {
    meta_t * meta = (meta_t *)get_core_mem(shared_mem);
    return meta[1] != 0;
}

void h2a_get_data (void *shared_mem, SnitchCoreData_t *core_data) {
    meta_t * meta = (meta_t *)get_core_mem(shared_mem);
    float * payload = (float *)(meta + META_SIZE);

    uint32_t total_size = meta[0];
    uint32_t payload_size = (total_size - META_SIZE_BYTES) / sizeof(float);

    core_data->op = meta[1];
    core_data->data = payload;
    core_data->size = payload_size;
}

int h2a_put_data (void *shared_mem, uint32_t *data, unsigned size) {
    meta_t * meta = (meta_t *)get_core_mem(shared_mem);
    uint32_t * dst = meta + META_SIZE;
    meta[0] = META_SIZE_BYTES + sizeof(uint32_t) * size;
    meta[2] = size;
    meta[3] = 1;

    for (unsigned i = 0; i < size; i++) {
        dst[i] = data[i];
    }
    //Note: this operation must be the last as it acts as a signal to the host application
    meta[1] = 0;
    return 0;
}

float task_fp32_mul_fact (float *data, unsigned size) {
    float result = data[0];
    for (unsigned i = 1; i < size; i++) {
        result = result * data[i];
    }
    return result;
}

float task_fp32_max (float *data, uint32_t len) {
  float m = -INFINITY;
  for (size_t i = 0; i < len; i++) {
    if (data[i] > m) {
        m = data[i];
    }
  }
  return m;
}

float task_fp32_div (float *data, uint32_t len) {
  float result = data[0];
  for (size_t i = 1; i < len; i++) {
    result /= data[i];
  }
  return result;
}

void task_expf (float *input, size_t input_len) {
    for (size_t i = 0; i < input_len; i++) {
        input[i] = expf(input[i]);
    }
}

void task_logf (float *input, size_t input_len) {
    for (size_t i = 0; i < input_len; i++) {
        input[i] = logf(input[i]);
    }
}

void task_softmax(float *input, size_t input_len) {
  float m = -INFINITY;
  for (size_t i = 0; i < input_len; i++) {
    if (input[i] > m) {
      m = input[i];
    }
  }

  float sum = 0.0;
  for (size_t i = 0; i < input_len; i++) {
    sum += expf(input[i] - m);
  }

  float offset = m + logf(sum);
  for (size_t i = 0; i < input_len; i++) {
    input[i] = expf(input[i] - offset);
  }
}