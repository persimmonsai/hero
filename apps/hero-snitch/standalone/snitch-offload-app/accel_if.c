#include "math.h"

#include "accel_if.h"

int h2a_has_request (void *_payload) {
    meta_t * meta = (meta_t *)_payload;
    return meta[1] != 0;
}

int h2a_get_data (void *_payload, SnitchAccelData_t *data) {
    unsigned core_num = snrt_global_core_num();
    meta_t * meta = (meta_t *)_payload;
    float * payload = (float *)(meta + META_SIZE);

    uint32_t total_size = meta[0];
    uint32_t payload_size = (total_size - META_SIZE_BYTES) / sizeof(float);

    data->coreData[0].op = meta[1];
    data->coreData[0].data = payload;
    data->coreData[0].size = payload_size;
    data->coreData[0].is_valid = 1;

    return 0;
}

int h2a_put_data_vector (void *_payload, uint32_t *data, unsigned size) {
    meta_t * meta = (meta_t *)_payload;
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

double task_fp64_mul_fact (double *data, unsigned size) {
    double result = data[0];
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