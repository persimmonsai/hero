
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include "common.h"
#include "fesrv.h"
#include "libsnitch.h"
#include "snitch_common.h"

#define SYS_exit 60
#define SYS_write 64
#define SYS_read 63
#define SYS_wake 1235
#define SYS_cycle 1236

#define SnitchOpCompute 0xce000000
#define SnitchOpTerminate 0xffffffff

#define HostOpRequestGet(_v) (((_v) >> 24) & 0xff)

#define HostOpRequestCompute 0x01
#define HostOpRequestMul 0x02
#define HostOpRequestTerminate 0xff
//Host ack
#define HostOpResponse 0xd05e0000
//Snitch Ack
#define SnitchOpResponse 0x5e550000

typedef enum {
  SnitchStateProc = 0,
  SnitchStateIdle = 1,
} SnitchSrvState_e;

#define DFLT_CLUSTER_IDX 0

#define max(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })
#define min(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })

#define ALIGN_UP(x, p) (((x) + (p)-1) & ~((p)-1))
#define PUTCHAR_BUF_SIZE 200

#define pr_info(args ...) printf(args)
#define pr_error(args ...) printf(args)
#define pr_warn(args ...) printf(args)
#define dbg(args ...)

static void _fesrv_run(fesrv_t *fs);
static void _handleSyscall(fesrv_t *fs, uint64_t magicMem[6]);

static volatile int g_interrupt = 0;

static void intHandler(int dummy) {
  g_interrupt = 1;
  pr_info("[fesrv] Caught signal, aborting\n");
}

int isolate_all(snitch_dev_t **clusters, uint32_t nr_dev, uint32_t iso) {
  int ret = 0;
  int status;
  for (uint32_t i = 0; i < nr_dev; ++i) {
    status = snitch_isolate(clusters[i], iso);
    if (status != 0) {
      printf("%sisolation failed for cluster %d: %s\n", iso == 0 ? "de-" : "", i, strerror(ret));
      ret -= 1;
    }
  }
  return ret;
}

void set_direct_tlb_map(snitch_dev_t *snitch, uint32_t idx, uint32_t low, uint32_t high) {
  struct axi_tlb_entry tlb_entry;
  tlb_entry.loc = AXI_TLB_NARROW;
  tlb_entry.flags = AXI_TLB_VALID;
  tlb_entry.idx = idx;
  tlb_entry.first = low;
  tlb_entry.last = high;
  tlb_entry.base = low;
  if (snitch_tlb_write(snitch, &tlb_entry)) printf("Wrong tlb write");
  tlb_entry.loc = AXI_TLB_WIDE;
  if (snitch_tlb_write(snitch, &tlb_entry)) printf("Wrong tlb write");
}

void reset_tlbs(snitch_dev_t *snitch) {
  struct axi_tlb_entry tlb_entry;
  tlb_entry.flags = 0;
  tlb_entry.first = 0;
  tlb_entry.last = 0;
  tlb_entry.base = 0;
  for (unsigned idx = 0; idx < 32; ++idx) {
    tlb_entry.idx = idx;
    tlb_entry.loc = AXI_TLB_NARROW;
    snitch_tlb_write(snitch, &tlb_entry);
    tlb_entry.loc = AXI_TLB_WIDE;
    snitch_tlb_write(snitch, &tlb_entry);
  }
}

int main(int argc, char *argv[]) {
  snitch_dev_t *snitch;
  snitch_dev_t **clusters;
  // snitch_perf_t perf;
  void *shared_l3_v;
  int size;
  uint32_t cluster_idx, nr_dev, wake_up_core = 0;
  char *file_to_send = NULL;
  void *addr, *a2h_rb_addr;
  int ret;
  uint32_t mask;
  struct axi_tlb_entry tlb_entry;

  printf("This is %s\n", argv[0]);
  printf("Usage: %s snitch_binary [file_to_send] [cluster_idx]\n", argv[0]);
  cluster_idx = DFLT_CLUSTER_IDX;
  if (argc >= 3) {
    file_to_send = argv[2];
    printf("  File to send %s\n", file_to_send);
  }
  if (argc >= 4)
    cluster_idx = atoi(argv[3]);
  if (argc >= 5)
    wake_up_core = atoi(argv[4]);

  printf("  Wake up core = %d\n", wake_up_core);
  printf("  Running on cluster %d\n", cluster_idx);

  // No app specified discover and exit
  snitch_set_log_level(LOG_INFO);
  fflush(stdout);
  if (argc < 2) {
    snitch_discover(NULL, NULL, NULL);
    exit(0);
  }

  // Map clusters to user-space and pick one for tests
  snitch_set_log_level(LOG_MAX);
  clusters = snitch_mmap_all(&nr_dev);
  // Restrict to local cluster and remote on qc
  nr_dev = 1;
  printf("clusters : %lx\n", (uint64_t)&clusters);
  snitch = clusters[cluster_idx];

  // Use L3 layout struct from the cluster provided as argument and set it's pointer in scratch[2]
  ret = snitch_scratch_reg_write(snitch, 2, (uint32_t)(uintptr_t)snitch->l3l_p);
  ret = snitch_scratch_reg_write(snitch, 3, (uint32_t)(uintptr_t)snitch->dma.p_addr);

  snitch_test_read_regions(snitch, 0);

  // clear all interrupts
  snitch_ipi_clear(snitch, 0, ~0U);

  // Add TLB entry for required ranges
  // reset_tlbs(snitch);
  set_direct_tlb_map(snitch, 0, 0x20000,    0x40000);  // BOOTROM
  set_direct_tlb_map(snitch, 1, 0x02000000, 0x02000fff);  // SoC Control
  set_direct_tlb_map(snitch, 2, 0x04000000, 0x040fffff);  // CLINT
  set_direct_tlb_map(snitch, 3, 0x10000000, 0x105fffff);  // Quadrants
  set_direct_tlb_map(snitch, 4, 0x80000000, 0xffffffff);  // HBM0/1

  for (unsigned i = 0; i < 5; ++i) {
    memset(&tlb_entry, 0, sizeof(tlb_entry));
    tlb_entry.loc = AXI_TLB_WIDE;
    tlb_entry.idx = i;
    snitch_tlb_read(snitch, &tlb_entry);
    printf("TLB readback Wide: idx %ld first %012lx last %012lx base %012lx flags %02x\n",
           tlb_entry.idx, tlb_entry.first, tlb_entry.last, tlb_entry.base, tlb_entry.flags);
    fflush(stdout);
    memset(&tlb_entry, 0, sizeof(tlb_entry));
    tlb_entry.loc = AXI_TLB_NARROW;
    tlb_entry.idx = i;
    snitch_tlb_read(snitch, &tlb_entry);
    printf("TLB readback Narrow: idx %ld first %012lx last %012lx base %012lx flags %02x\n",
           tlb_entry.idx, tlb_entry.first, tlb_entry.last, tlb_entry.base, tlb_entry.flags);
    fflush(stdout);
  }

  // De-isolate quadrant
  isolate_all(clusters, nr_dev, 1);
  ret = isolate_all(clusters, nr_dev, 0);
  if (ret) {
    isolate_all(clusters, nr_dev, 1);
    exit(-1);
  }

  // setup front-end server. Do this here so that the host communication data is before any other
  // data in L3 to prevent overwriting thereof
  fesrv_t *fs = malloc(sizeof(fesrv_t));
  fesrv_init(fs, snitch, &a2h_rb_addr, 1024);
  snitch->l3l->a2h_rb = (uint32_t)(uintptr_t)a2h_rb_addr;

  // fill memory with known pattern
  if (memtest(snitch->l1.v_addr, snitch->l1.size, "TCDM", 'T')) return -1;

  // and some test scratch l3 memory
  // For largest axpy problem: (2*N+1)*sizeof(double), N=3*3*6*2048
  // For largest conv2d problem: (64*112*112+64*64*7*7+64*112*112)*sizeof(double) = 14112*1024
  // 2x for good measure
  shared_l3_v = snitch_l3_malloc(snitch, 2 * 14112 * 1024, &addr);
  assert(shared_l3_v);
  snitch->l3l->heap = (uint32_t)(uintptr_t)addr;
  printf("alloc l3l_v->heap: %08x\r\n", snitch->l3l->heap);
  if (memtest(shared_l3_v, 1024, "L3", '3')) return -1;

  snprintf(shared_l3_v, 1024, "this is linux");
  fflush(stdout);

  /* READ FILE START */
  ret = access(argv[2], R_OK);
  if (ret) {
    printf("Can't access file %s: %s\n", argv[2], strerror(ret));
    return ret;
  }
  printf("\nFile opened %s\n", argv[2]);

  FILE *fp = fopen(argv[2], "rb");
  if (!fp) {
    printf("fopen file error");
    return EXIT_FAILURE;
  }

  // Read file buffer
  uint8_t buffer[1000];

  unsigned int n_data_read = fread(buffer, sizeof(uint8_t), 1000, fp);
  for (unsigned int i = 0; i < n_data_read; i++) {
    ((uint8_t*)shared_l3_v)[i] = buffer[i];
  }

  printf("Copied %u bytes from %s to L3\n\n", n_data_read, argv[2]);
  /* READ FILE ENDS */

  if (argc >= 2) {
    size = snitch_load_bin(snitch, argv[1]);
    if (size < 0) goto exit;

    printf("Set interrupt on core %u\n", wake_up_core);
    //snitch_ipi_set(snitch, 0, 1 << (wake_up_core + cluster_idx * 9 + 1));
    snitch_ipi_set(snitch, 0, 0x3FE);
    fflush(stdout);

    printf("Waiting for program to terminate..\n");
    fflush(stdout);
    fs->abortAfter = 0;
    _fesrv_run(fs);
    // sleep(3);

    snitch_ipi_get(snitch, 0, &mask);
    printf("clint after completion: %08x\n", mask);
  }

exit:
  snitch_ipi_clear(snitch, 0, ~0U);
  ret = isolate_all(clusters, nr_dev, 1);

  printf("Exiting\n");
  return ret;
}


/**
 * @brief Runs the front end server. Best to run this as a thread
 * @details
 *
 * @param fs pointer to the front end server struct
 */
static void _fesrv_run(fesrv_t *fs) {
  uint64_t magicMem[6];
  bool autoAbort;
  useconds_t tstart;
  int ret;
  uint32_t prev_tail = 0;
  const char *abort_reason;


  g_interrupt = 0;
  signal(SIGINT, intHandler);

  pr_info("[fesrv] Start polling\n");

  // check if we need to autoabort
  autoAbort = (fs->abortAfter > 0) ? true : false;

  /**
   * Polling loop for tohost variable
   */
  bool abort = false;
  bool wait = true;
  SnitchSrvState_e snitch_state = SnitchStateIdle;
  tstart = time(NULL);

  while (!abort) {

    uint32_t host_req_data = snitch_host_req_get(fs->dev, 1);
    uint32_t host_req_op = HostOpRequestGet(host_req_data);

    switch (snitch_state) {
      case SnitchStateIdle: {
        if (host_req_op) {
          printf("[fesrv] Snitch op requested: %d\n", host_req_op);
          switch (host_req_op) {
            case HostOpRequestCompute: {
            //Copy data from host to snitch memory
            #if 0
                  uint32_t *host_ptr = (uint32_t *)fs->dev->dma.v_addr;
                  uint32_t *snitch_ptr = (uint32_t *)fs->dev->l3l->heap;

                  uint32_t length = host_ptr[0];

                  for (uint32_t i = 0; i < length; i++) {
                    snitch_ptr[i] = host_ptr[i];
                  }
            #endif
                snitch_mbox_write(fs->dev, SnitchOpCompute);
                snitch_state = SnitchStateProc;
                break;
            }
            case HostOpRequestTerminate: {
                snitch_mbox_write(fs->dev, SnitchOpTerminate);
                break;
            }
            default: {
            }
          }
        }
      }
      case SnitchStateProc: {
        if (host_req_op == HostOpResponse) {
          snitch_state = SnitchStateIdle;
          snitch_host_req_set(fs->dev, 0);
          printf("[fesrv] Got response from host\n");
        }
      }
    }

    // try to pop a value
    // snitch_flush(fs->dev);
    ret = rb_host_get(fs->a2h_rb, magicMem);

    if (ret == 0) {
      // reset abort timer
      tstart = time(NULL);

      // check for tail-skip
      if (((prev_tail + 1) % fs->a2h_rb->size) != fs->a2h_rb->tail) {
        pr_error("ERROR: tail skipped from %d to %d!!!\n", prev_tail, fs->a2h_rb->tail);
        pr_error("This usually means the buffer area was overwritten. Perhaps you didn't allocate "
               "enough\n");
        pr_error("memory in the last call to snitch_l3_malloc before fesrv_init?\n");
      }
      prev_tail = fs->a2h_rb->tail;
      dbg("[fesrv]   0: %#lx 1: %#lx", magicMem[0], magicMem[1]);
      dbg(" 2: %#lx (%c) 3: %#lx", magicMem[2], (char)magicMem[2], magicMem[3]);
      dbg(" 4: %#lx 5: %#lx\n", magicMem[4], magicMem[5]);
      // dont wait this round
      wait = false;

      // process
      fs->nCalls++;
      _handleSyscall(fs, magicMem);

#ifdef DBG_LOG
      struct timespec tv;
      clock_gettime(CLOCK_REALTIME, &tv);
      fprintf(fs->logfile, "%lu,%ld,%lu,%lu,%lu,%lu,%lu\n",
              (tv.tv_sec) * 1000 + (tv.tv_nsec) / 1000000, magicMem[0], magicMem[1], magicMem[2],
              magicMem[3], magicMem[4], magicMem[5]);
#endif
    }
    // only wait if nothing received
    if (wait)
      usleep(fs->pollInterval);
    wait = true;

    // check for abort
    if (fs->abort) {
      abort = true;
      abort_reason = "external";
    }
    if (autoAbort && (time(NULL) > (tstart + fs->abortAfter))) {
      abort = true;
      abort_reason = "timeout";
    }
    if (fs->coreExited) {
      abort = true;
      abort_reason = "exit code";
    }
    if (g_interrupt) {
      abort = true;
      abort_reason = "interrupt";
    }
  }

  fclose(fs->stdout_file);
#ifdef DBG_LOG
  fclose(fs->logfile);
#endif
  signal(SIGINT, SIG_DFL);
  pr_info("[fesrv] Exiting (%s). Syscalls processed: %ld Exit code received? %s: %d\n", abort_reason,
         fs->nCalls, fs->coreExited ? "Yes" : "No", fs->exitCode);
}

/**
 * @brief processes the system call
 * @details
 *
 * @param magicMem magic memory passed from snitch to host
 */
static void _handleSyscall(fesrv_t *fs, uint64_t magicMem[6]) {
  bool handled = false;

  switch (magicMem[0]) {
  case SYS_write:
    // _putchar
    if ((magicMem[1] == 1) && (magicMem[3] == 1)) {
      handled = true;
      dbg("[fesrv]   putchar - %c\n", (char)magicMem[2]);
      fs->putCharBuf[fs->putCharIdx++] = magicMem[2];
      if (magicMem[2] == '\n' || (fs->putCharIdx == PUTCHAR_BUF_SIZE - 1)) {
        if (fs->putCharIdx == PUTCHAR_BUF_SIZE - 1)
          pr_warn("[fesrv] Warning: putchar buffer limit reached!\n");
        // null-terminate and remove \r and/or \n
        fs->putCharBuf[fs->putCharIdx] = '\0';
        fs->putCharBuf[strcspn(fs->putCharBuf, "\r\n")] = 0;
        fprintf(fs->stdout_file, "%s\n", fs->putCharBuf);
        printf("%s\n", fs->putCharBuf);
        fflush(stdout);

        // reset pointer
        fs->putCharIdx = 0;
      }
    }
    break;
  case SYS_read:

    break;
  case SYS_exit:
    handled = true;

    int exit_code = (int)magicMem[1];
    if (exit_code == 0) {
      pr_info("[fesrv] Sending response to host\n");
      snitch_host_req_set(fs->dev, SnitchOpResponse);
    } else {
      pr_info("[fesrv]   Exited with code %d (%#lx)\n", (int)magicMem[1], magicMem[1]);
      fs->coreExited = 1;
      fs->exitCode = exit_code;
    }
    break;
  case SYS_wake:
    handled = true;
    pr_info("[fesrv]   wake %#x\n", (uint32_t)magicMem[1]);
    break;
  case SYS_cycle:
    // handled = true;
    // fs->cyclesReported[core] = magicMem[1];
    // printf("[fesrv]   reports %lu cycles\n", fs->cyclesReported[core]);
    break;
  default:
    break;
  }

  if (!handled) {
    pr_error("[fesrv] Unknown syscall\n");
    pr_error("[fesrv]   0: %016lx 1: %016lx\n", magicMem[0], magicMem[1]);
    pr_error("[fesrv]   2: %016lx 3: %016lx\n", magicMem[2], magicMem[3]);
    pr_error("[fesrv]   4: %016lx 5: %016lx\n", magicMem[4], magicMem[5]);
  }
}