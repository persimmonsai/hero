#include "math.h"
#include "string.h"

#include "snitch_config.h"
#include "snrt.h"
#include "printf.h"

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
    uint32_t * payload = (uint32_t *)(meta + META_SIZE);

    uint32_t total_size = meta[0];
    uint32_t payload_size = (total_size - META_SIZE_BYTES) / sizeof(uint32_t);

    core_data->op = meta[1];
    core_data->data = payload;
    core_data->size = payload_size;
}

int h2a_put_data (void *shared_mem, const uint32_t *data, const size_t size_bytes) {
    meta_t * meta = (meta_t *)get_core_mem(shared_mem);
    uint32_t * dst = meta + META_SIZE;
    meta[0] = META_SIZE_BYTES + size_bytes;

    memcpy(dst, data, size_bytes);
    //Note: this operation must be the last as it acts as a signal to the host application
    meta[1] = 0;
    return 0;
}

int h2a_put_dummy_data(void *shared_mem) {
  uint32_t dummy = 0;
  return h2a_put_data(shared_mem, &dummy, sizeof(uint32_t));
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

static void mat_print_u16 (uint16_t * src, uint32_t size) {
  printf("[\n");
  for (uint32_t j = 0; j < size; j++) {
    uint32_t ij = src[j];
    printf("%x, ", ij);
  }
  printf("]\n\n");
}

static uint32_t sa_data_cnt = 0;

void * task_mat_mul (sa_prop_t *sa_prop, SnitchCoreData_t *core_data) {

  unsigned core_idx = snrt_global_core_idx();
  uint64_t *cmdq_ptr = NULL;
  uint32_t inw_bytes = sa_prop->in_width/8;

  if (core_idx == 0) {
    mat_t ab[16] = {{0}};
    uint32_t batch_cnt = 0;

    uint32_t *ptr = core_data->data;
    for (uint32_t i = 0; i < 16; i++) {
      uint32_t data_size = ptr[2];
      printf("task_mat_mul: ptr = %p, data_size = %d\n", ptr, data_size);

      if (ptr[2] == 0) {
        break;
      }

      ab[i].data = &ptr[3];
      ab[i].size_bytes = ptr[2];

      if ((ptr[2] % sizeof(uint32_t)) != 0) {
        printf("task_mat_mul: Size is not aligned !\n");
        return NULL;
      }

      ptr += (ptr[2] / sizeof(uint32_t)) + 3;
      batch_cnt++;
    }

    batch_cnt /= 2;

    void * tcdm_ptr = (void *)TCDM_BASE_ADDR;
    uint8_t * tcdm_data_ptr = (uint8_t *)TCDM_BASE_ADDR + 0x1000;
    cmdq_ptr = (uint64_t *)tcdm_ptr;

    uint32_t c_size = sa_prop->width * sa_prop->height;

    sa_write_ctrl(0);

    sa_config(cmdq_ptr);

    printf("task_mat_mul: setting cmdq\n");

    for (uint32_t i = 0; i < batch_cnt; i++) {
      uint8_t *a_ptr = tcdm_data_ptr;
      uint8_t *b_ptr = a_ptr + ab[i/2].size_bytes;

      uint32_t a_size = ab[i/2].size_bytes;
      uint32_t b_size = ab[i/2 + 1].size_bytes;

      if (0) {
        a_ptr = ab[i/2].data;
        b_ptr = ab[i/2 + 1].data;
      } else {
        memcpy(a_ptr, ab[i/2].data, a_size);
        memcpy(b_ptr, ab[i/2 + 1].data, b_size);
      }

      //mat_print_u16(a_ptr, a_size / inw_bytes);
      //mat_print_u16(b_ptr, b_size / inw_bytes);

      cmdq_ptr = sa_addq(cmdq_ptr,
                          a_ptr,
                          b_ptr,
                          a_size,
                          b_size,
                          i == batch_cnt-1, i);

      tcdm_data_ptr = b_ptr + b_size;
      sa_data_cnt += c_size;
    }

    sa_write_ctrl(1);
  }

  snrt_cluster_hw_barrier();
  sa_memread(sa_prop, cmdq_ptr, &sa_data_cnt);
  snrt_cluster_hw_barrier();

  return cmdq_ptr;
}