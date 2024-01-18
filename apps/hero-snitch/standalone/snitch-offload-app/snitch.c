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
static volatile uint32_t print_lock = 0;
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

      void * c_ptr = task_mat_mul(&sa_prop, &core_data);

      if (0 == core_idx) {
        if (NULL == c_ptr) {
          printf("task_mat_mul: Failed\n");
          h2a_put_dummy_data(dma);
        }
        h2a_put_data(dma, c_ptr, sizeof(uint32_t) * sa_prop.width * sa_prop.height);
      }
      break;
    }
    case 0x40: {
      uint32_t data[4];
      data[0] = sa_prop.version;
      data[1] = sa_prop.mac_type;
      data[2] = (sa_prop.width << 16) | sa_prop.height;
      data[3] = (sa_prop.out_width << 16) | sa_prop.in_width;

      h2a_put_data(dma, data, 4 * sizeof(uint32_t));
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
  snrt_cluster_hw_barrier();
  if (0 == core_idx) {
    snrt_mutex_lock(&print_lock);
    printf("[TASK] (core idx %u) DONE!\n", core_idx);
    snrt_mutex_release(&print_lock);
  }
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

    sa_discover(&sa_prop);

    printf("(cluster %u, idx %u/%u, is_dma = %i) Finished setting up mailboxes\n", cluster_idx,
            core_idx, core_num - 1, snrt_is_dm_core());
    printf("Coherent memory phys addr = 0x%p\n", dma);
    printf("[SA]: version = 0x%x, w = %d, h = %d, mac_type = %d, in width = %d, out width = %d, num events: %d\n",
          sa_prop.version, sa_prop.width, sa_prop.height, sa_prop.mac_type, sa_prop.in_width, sa_prop.out_width,
          sa_prop.num_events);

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