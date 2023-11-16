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

#define FILE_SIZE 128
uint8_t file_content[FILE_SIZE];

static void a2h_handle_request (unsigned core_idx) {

  SnitchAccelData_t accel_data = {0};
  h2a_get_data(dma, &accel_data);

  SnitchCoreData_t *core_data = &accel_data.coreData[core_idx];

  if (!core_data->is_valid) {
    return;
  }

  snrt_mutex_lock(&print_lock);
  printf("[TASK] (core idx %u, is_dma = %i) a2h request received:  %d\n", core_idx, snrt_is_dm_core(), core_data->op);
  snrt_mutex_release(&print_lock);

  switch (core_data->op) {
    case 0x1: {
      uint32_t result = core_data->data[0];
      for (uint32_t i = 1; i < core_data->size; i++) {
        result = task_fp32_mul_1(result, core_data->data[i]);
      }
      h2a_put_data_1(dma, result);
      //snrt_cluster_hw_barrier();
      break;
    }
    case 0x2: {
      uint32_t result = task_fp32_max(core_data->data, core_data->size);
      h2a_put_data_1(dma, result);
      break;
    }
    default: {
      //snrt_cluster_hw_barrier();
      syscall(SYS_exit, 0, 0, 0, 0, 0);
      while (1) {}
    }
  }
  snrt_mutex_lock(&print_lock);
  printf("[TASK] (core idx %u, is_dma = %i) DONE!\n", core_idx, snrt_is_dm_core());
  snrt_mutex_release(&print_lock);
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

  // First core sets up the mailboxes and stuff
  if (snrt_is_dm_core()) {
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

    printf("(cluster %u, idx %u/%u, is_dma = %i) Finished setting up mailboxes\n", cluster_idx,
            core_idx, core_num - 1, snrt_is_dm_core());
    printf("Coherent memory phys addr = 0x%p\n", dma);
    snrt_mutex_release(&print_lock);
  }

  snrt_cluster_hw_barrier();

  snrt_mutex_lock(&print_lock);
  printf("(cluster %u, idx %u/%u, is_dma = %i) Hello from snitch hartid %d\n", cluster_idx,
         core_idx, core_num - 1, snrt_is_dm_core(), snrt_hartid());
  snrt_mutex_release(&print_lock);

  snrt_cluster_hw_barrier();

  if (!snrt_is_dm_core()) {
    while (1) {
      if (h2a_has_request(dma)) {
        a2h_handle_request(core_idx);
      }
    }
  }

  if (snrt_is_dm_core()) {
    while (1) {}
  }

  return 0;
}
