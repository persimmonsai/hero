
#include "accel_if.h"

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

uint32_t fp32_mul (uint32_t a, uint32_t b) {
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