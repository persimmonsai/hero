#include "math.h"

#include "printf.h"
#include "snrt.h"

#include "accel_if.h"
#include "sa.h"

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
    core_data->w = meta[2];
    core_data->h = meta[3];
    core_data->dtype = meta[4];
}

int h2a_put_data_2d (void *shared_mem, const uint32_t *data, unsigned w, unsigned h) {
    meta_t * meta = (meta_t *)get_core_mem(shared_mem);
    uint32_t * dst = meta + META_SIZE;
    uint32_t size = w * h;
    meta[0] = META_SIZE_BYTES + sizeof(uint32_t) * size;
    meta[2] = w;
    meta[3] = h;

    for (unsigned i = 0; i < size; i++) {
        dst[i] = data[i];
    }
    //Note: this operation must be the last as it acts as a signal to the host application
    meta[1] = 0;
    return 0;
}

int h2a_put_data (void *shared_mem, const uint32_t *data, unsigned size) {
    return h2a_put_data_2d(shared_mem, data, size, 1);
}

int h2a_put_dummy_data(void *shared_mem) {
  float dummy = 0.0f;
  h2a_put_data_2d(shared_mem, &dummy, 1, 0);
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

static void copy_u32 (uint32_t * dst, uint32_t *src, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    dst[i] = src[i];
  }
}

static void mat_rotate_cw_u32 (uint32_t *dst, uint32_t *src, uint32_t w, uint32_t h) {
  for (uint32_t i = 0; i < w; i++) {
    for (uint32_t j = 0; j < h; j++) {
      dst[(i * h) + j] = src[(j * w) + i];
    }
  }
}

void * task_mat_mul (sa_prop_t *sa_prop, void * a, void * b) {
  uint32_t * tcdm_ptr = (uint32_t *)0x10000000;

  uint32_t mat_size = sa_prop->width * sa_prop->height;
  uint32_t * a_ptr = tcdm_ptr;
  uint32_t * b_ptr = a_ptr + mat_size;
  uint32_t * c_ptr = tcdm_ptr;

  copy_u32(a_ptr, a, mat_size);
  mat_rotate_cw_u32(b_ptr, b, sa_prop->width, sa_prop->height);

  exec_sa(sa_prop, a_ptr, b_ptr, c_ptr);

  return c_ptr;
}