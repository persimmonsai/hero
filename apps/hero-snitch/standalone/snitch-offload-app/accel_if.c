#include "math.h"

#include "accel_if.h"

int h2a_has_request (void *_payload) {
    meta_t * meta = (meta_t *)_payload;
    return meta[1] != 0;
}

int h2a_get_data (void *_payload, SnitchAccelData_t *data) {
    unsigned core_num = snrt_global_core_num();
    meta_t * meta = (meta_t *)_payload;
    data_t * payload = (data_t *)(meta + META_SIZE);

    uint32_t total_size = meta[0];
    uint32_t payload_size = (total_size - META_SIZE_BYTES) / sizeof(data_t);

    data->coreData[0].op = meta[1];
    data->coreData[0].data = payload;
    data->coreData[0].size = payload_size;
    data->coreData[0].is_valid = 1;

    return 0;
}

int h2a_put_data_1 (void *_payload, data_t result) {
    meta_t * meta = (meta_t *)_payload;
    data_t * data = meta + META_SIZE;
    meta[0] = META_SIZE_BYTES + sizeof(data_t);
    meta[1] = 0;
    meta[2] = 1;
    meta[3] = 1;
    data[0] = result;
    return 0;
}

static inline uint32_t fp32_to_u32 (float _f) {
    float f = _f;
    uint32_t * uptr = (uint32_t *)(&f);
    return *uptr;
}

static inline uint32_t fp32_mul (uint32_t a, uint32_t b) {
    uint32_t res;

    asm volatile(
    "fmv.w.x ft1, %0\n"
    "fmv.w.x ft2, %1\n"
    : "+r"(a), "+r"(b));

    asm volatile(
    "fmul.s ft0, ft1, ft2\n"
    "fmv.x.w %0, ft0\n"
    : "+r"(res));

    return res;
}

static inline uint32_t fp32_add (uint32_t a, uint32_t b) {
    uint32_t res;

    asm volatile(
    "fmv.w.x ft1, %0\n"
    "fmv.w.x ft2, %1\n"
    : "+r"(a), "+r"(b));

    asm volatile(
    "fadd.s ft0, ft1, ft2\n"
    "fmv.x.w %0, ft0\n"
    : "+r"(res));

    return res;
}

static inline uint32_t fp32_max (uint32_t a, uint32_t b) {
    uint32_t res;
    asm volatile(
    "fmv.w.x ft1, %0\n"
    "fmv.w.x ft2, %1\n"
    : "+r"(a), "+r"(b));

    asm volatile(
    "fmax.d ft0, ft1, ft2\n"
    "fmv.x.w %0, ft0\n"
    : "+r"(res));

    return res;
}

//true if a <= b
static inline uint32_t fp32_le (uint32_t a, uint32_t b) {
    uint32_t res;
    asm volatile(
    "fmv.w.x ft1, %0\n"
    "fmv.w.x ft2, %1\n"
    : "+r"(a), "+r"(b));

    asm volatile(
    "fle.s %0, ft1, ft2\n"
    : "+r"(res));

    return res;
}

uint32_t task_fp32_mul_1 (uint32_t a, uint32_t b) {
    return fp32_mul(a, b);
}

uint32_t task_fp32_max (uint32_t *input, uint32_t input_len) {
  uint32_t m = fp32_to_u32(-INFINITY);
  for (size_t i = 0; i < input_len; i++) {
    if (fp32_le(m, input[i])) {
      m = input[i];
    }
  }
  return m;
}

void softmax(uint32_t *input, uint32_t input_len) {

  uint32_t m = task_fp32_max(input, input_len);

  uint32_t sum = fp32_to_u32(0.0);
  for (size_t i = 0; i < input_len; i++) {
    sum += expf(input[i] - m);
  }

  float offset = m + logf(sum);
  for (size_t i = 0; i < input_len; i++) {
    input[i] = expf(input[i] - offset);
  }
}