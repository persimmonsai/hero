#include <string.h>

#include "printf.h"
#include "snrt.h"
#include "accel_if.h"

// #include "../include/snitch_common.h"
#include "snitch_hero_support.h"

/***********************************************************************************
 * DATA
 ***********************************************************************************/

extern const uint32_t scratch_reg;  // In start_snitch.S
static volatile uint32_t *const soc_scratch0 = (uint32_t *)(0x02000014);
static volatile uint32_t *soc_scratch = soc_scratch0;
extern volatile struct ring_buf *g_a2h_rb;
extern volatile struct ring_buf *g_a2h_mbox;
extern volatile struct ring_buf *g_h2a_mbox;
static volatile int32_t print_lock = 0;
static volatile uint8_t *l3;
static volatile uint32_t *dma = NULL;
static volatile sa_prop_t sa_prop;

#define FILE_SIZE 128
uint8_t file_content[FILE_SIZE];

static void print_f32_result (int *lock, float *result) {
  uint32_t u32 = fp32_to_u32(*result);
  snrt_mutex_lock(lock);
  printf("result = 0x%x\n", u32);
  snrt_mutex_release(lock);
}

static void mat_print_u32 (uint32_t * src, uint32_t w, uint32_t h) {
  printf("[\n");
  for (uint32_t i = 0; i < w; i++) {
    for (uint32_t j = 0; j < h; j++) {
      uint32_t ij = src[(i * h) + j];
      printf("%x, ", ij);
    }
    printf("\n");
  }
  printf("]\n\n");
}

static void mat_print_u16 (uint16_t * src, uint32_t w, uint32_t h) {
  printf("[\n");
  for (uint32_t i = 0; i < w; i++) {
    for (uint32_t j = 0; j < h; j++) {
      uint32_t ij = src[(i * h) + j];
      printf("%x, ", ij);
    }
    printf("\n");
  }
  printf("]\n\n");
}

static void mat_to_bfloat16 (uint16_t * dst, uint32_t * src, uint32_t w, uint32_t h) {
  uint32_t size = w * h;

  for (uint32_t i = 0; i < size; i++) {
    //Cut the lower half
    dst[i] = (src[i] >> 16) & 0xffff;
  }
}

static void mat_from_bfloat16 (uint32_t * dst, uint16_t * src, uint32_t w, uint32_t h) {
  uint32_t size = w * h;

  for (uint32_t i = 0; i < size; i++) {
    dst[i] = (src[i] << 16);
  }
}

static int check_mat_prop (SnitchCoreData_t * core_data, sa_prop_t * sa_prop) {
  snrt_mutex_lock(&print_lock);
  printf("[SA] data: w = %d, h = %d, dtype = %d\n", core_data->w, core_data->h, core_data->dtype);
  snrt_mutex_release(&print_lock);

  if ((core_data->w != sa_prop->width) || (core_data->h != sa_prop->height)) {
    snrt_mutex_lock(&print_lock);
    printf("[SA] Dimension doesn't match\n");
    snrt_mutex_release(&print_lock);
    h2a_put_dummy_data(dma);
    return 1;
  }
#if 0
  if (core_data.dtype != sa_prop.mac_type) {
    snrt_mutex_lock(&print_lock);
    printf("[SA] Data type doesn't match\n");
    snrt_mutex_release(&print_lock);
    h2a_put_dummy_data(dma);
    break;
  }
#endif
  return 0;
}

const uint16_t m_a_ex[] = {
0, 0, 0, 0, 0, 0, 0, 0,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
};

const uint16_t m_b_ex[] = {
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
0x3f80, 0x4000, 0x4040, 0x4080, 0x40a0, 0x40c0, 0x40e0, 0x4100,
};

static int a2h_handle_request (void) {

  unsigned core_idx = snrt_global_core_idx();
  SnitchCoreData_t core_data = {0};
  h2a_get_data(dma, &core_data);

  snrt_mutex_lock(&print_lock);
  printf("[TASK] (core idx %u) a2h request received:  %d\n", core_idx, core_data.op);
  //uint32_t *data = (uint32_t *)core_data.data;
  //for (unsigned i = 0 ;  i < core_data.size; i++) {
  //  printf("0x%x\n", data[i]);
  //}
  snrt_mutex_release(&print_lock);


  switch (core_data.op) {
    case 0x1: {
      float result = task_fp32_mul_fact(core_data.data, core_data.size);
      print_f32_result(&print_lock, &result);
      h2a_put_data(dma, &result, 1);
      break;
    }
    case 0x2: {
      float result = task_fp32_max(core_data.data, core_data.size);
      print_f32_result(&print_lock, &result);
      h2a_put_data(dma, &result, 1);
      break;
    }
    case 0x3: {
      task_expf(core_data.data, core_data.size);
      h2a_put_data(dma, core_data.data, core_data.size);
      break;
    }
    case 0x4: {
      task_logf(core_data.data, core_data.size);
      h2a_put_data(dma, core_data.data, core_data.size);
      break;
    }
    case 0x6: {
      float result = task_fp32_div(core_data.data, core_data.size);
      h2a_put_data(dma, &result, 1);
      break;
    }
    case 0x20: {
      task_softmax(core_data.data, core_data.size);
      h2a_put_data(dma, core_data.data, core_data.size);
      break;
    }
    case 0x30: {
      if (check_mat_prop(&core_data, &sa_prop)) {
        break;
      }

      uint32_t * a_ptr = core_data.data;
      uint32_t * b_ptr = a_ptr + (core_data.w * core_data.h);
      uint32_t * c_ptr = task_mat_mul(&sa_prop, a_ptr, b_ptr);

      printf("A:\n");
      mat_print_u32(a_ptr, core_data.w, core_data.h);
      printf("B:\n");
      mat_print_u32(b_ptr, core_data.w, core_data.h);

      h2a_put_data_2d(dma, c_ptr, core_data.w * core_data.h * sizeof(uint32_t), sa_prop.width, sa_prop.height);

      break;
    }
    //bfloat16
    case 0x31: {
      if (check_mat_prop(&core_data, &sa_prop)) {
        break;
      }

      const uint32_t * a_ptr = core_data.data;
      const uint32_t * b_ptr = a_ptr + (core_data.w * core_data.h);

      mat_to_bfloat16(a_ptr, a_ptr, core_data.w, core_data.h);
      mat_to_bfloat16(b_ptr, b_ptr, core_data.w, core_data.h);

      printf("input A 16\n");
      mat_print_u16(a_ptr, core_data.w, core_data.h);

      printf("input B 16\n");
      mat_print_u16(b_ptr, core_data.w, core_data.h);

      uint32_t * c_ptr = task_mat_mul16(&sa_prop, a_ptr, b_ptr);

      //printf("output A 32\n");
      //mat_print_u32(c_ptr, core_data.w, core_data.h);

      h2a_put_data_2d(dma, c_ptr, core_data.w * core_data.h * sizeof(uint32_t), sa_prop.width, sa_prop.height);

      break;
    }
    case 0x32: {
      const uint32_t * a_ptr = m_a_ex;
      const uint32_t * b_ptr = m_a_ex;

      uint32_t * c_ptr = task_mat_mul16(&sa_prop, a_ptr, b_ptr);

      h2a_put_data_2d(dma, c_ptr, sa_prop.width * sa_prop.height * sizeof(uint32_t), sa_prop.width, sa_prop.height);
      break;
    }
    case 0x40: {
      uint32_t data[4];
      data[0] = sa_prop.version;
      data[1] = sa_prop.mac_type;
      data[2] = (sa_prop.width << 16) | sa_prop.height;
      data[3] = (sa_prop.out_width << 16) | sa_prop.in_width;

      h2a_put_data(dma, data, 4);
      break;
    }
    case 0x100: {
      asm volatile("fcvt.d.w f0, zero");
      h2a_put_dummy_data(dma);
      break;
    }
    default: {
      return 1;
    }
  }
  snrt_mutex_lock(&print_lock);
  printf("[TASK] (core idx %u) DONE!\n", core_idx);
  snrt_mutex_release(&print_lock);
  return 0;
}

/***********************************************************************************
 * MAIN
 ***********************************************************************************/

int main(void) {
  struct l3_layout l3l;
  int ret;
  volatile struct ring_buf priv_rb;
  print_lock = 1;

  unsigned cluster_idx = snrt_cluster_idx();
  unsigned core_idx = snrt_global_core_idx();
  unsigned core_num = snrt_global_core_num();
  unsigned is_setup_core = snrt_is_dm_core();
  unsigned is_compute_core = core_num == 1 ? 1 : !is_setup_core;

  // First core sets up the mailboxes and stuff
  if (is_setup_core) {
    // Read memory layout from scratch2 (L3)
    memcpy(&l3l, (void *)soc_scratch[2], sizeof(struct l3_layout));
    dma = (uint32_t *)soc_scratch[3];

    // Setup mailboxes (in L3)
    g_a2h_rb = (struct ring_buf *)l3l.a2h_rb;
    g_a2h_mbox = (struct ring_buf *)l3l.a2h_mbox;
    g_h2a_mbox = (struct ring_buf *)l3l.h2a_mbox;
    // Setup shared heap (in L3)
    l3 = (uint8_t *)l3l.heap;
    // Setup print lock

    discover_sa(&sa_prop);

    printf("(cluster %u, idx %u/%u, is_dma = %i) Finished setting up mailboxes\n", cluster_idx,
            core_idx, core_num - 1, snrt_is_dm_core());
    printf("Coherent memory phys addr = 0x%p\n", dma);
    printf("[SA]: version = 0x%x, w = %d, h = %d, mac_type = %d, in width = %d, out width = %d\n",
          sa_prop.version, sa_prop.width, sa_prop.height, sa_prop.mac_type, sa_prop.in_width, sa_prop.out_width);

    snrt_mutex_release(&print_lock);
  }

  snrt_cluster_hw_barrier();

  snrt_mutex_lock(&print_lock);
  printf("(cluster %u, idx %u/%u, is_dma = %i) Hello from snitch hartid %d\n", cluster_idx,
         core_idx, core_num - 1, snrt_is_dm_core(), snrt_hartid());
  snrt_mutex_release(&print_lock);

  snrt_cluster_hw_barrier();

  int do_exit = 0;
  if (is_compute_core) {
    while (!do_exit) {
      if (h2a_has_request(dma)) {
        do_exit = a2h_handle_request();
      }
    }
  }

  snrt_cluster_hw_barrier();
  syscall(SYS_exit, 0, 0, 0, 0, 0);
  return 0;
}
