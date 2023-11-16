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

#include "fesrv_local.h"
#include "libsnitch.h"
#include "snitch_common.h"

#define PUTCHAR_BUF_SIZE 200

#define pr_info(args ...) printf(args)
#define pr_error(args ...) printf(args)
#define pr_warn(args ...) printf(args)
#define dbg(args ...)

#define SYS_exit 60
#define SYS_write 64
#define SYS_read 63
#define SYS_wake 1235
#define SYS_cycle 1236

static volatile int g_interrupt = 0;

static void intHandler(int dummy) {
  g_interrupt = 1;
  pr_info("[fesrv] Caught signal, aborting\n");
}

static void handleSyscall(fesrv_t *fs, uint64_t magicMem[6]);

/**
 * @brief Runs the front end server. Best to run this as a thread
 * @details
 *
 * @param fs pointer to the front end server struct
 */
void fesrv_local_run(fesrv_t *fs) {
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
  tstart = time(NULL);

  while (!abort) {
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
      handleSyscall(fs, magicMem);

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
static void handleSyscall(fesrv_t *fs, uint64_t magicMem[6]) {
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
    fs->coreExited = 1;
    fs->exitCode = exit_code;
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

  fflush(stdout);
}