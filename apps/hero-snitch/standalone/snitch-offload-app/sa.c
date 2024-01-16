#include "snrt.h"
#include "printf.h"
#include "snitch_hero_support.h"

#include "snitch_config.h"
#include "sa.h"

#define SA_CTRL_REG_OFF (0 * 8)
#define SA_CMDQ_OFF_REG_OFF (1 * 8)
#define SA_STATUS_REG_OFF (4 * 8)
#define SA_VERSION_REG_OFF (5 * 8)
#define SA_PROP1_REG_OFF (6 * 8)
#define SA_PROP2_REG_OFF (7 * 8)
#define SA_PROP3_REG_OFF (8 * 8)
#define SA_FIFO_DATA_REG_OFF (11 * 8)
#define SA_FIFO_META_REG_OFF (12 * 8)
#define SA_EVENTS_REG_OFF (20 * 8)

static inline void sa_write_reg (volatile uint32_t addr, uint32_t data) {
  volatile uint32_t * ptr = (volatile uint32_t *)addr;
  ptr[0] = data;
}

static inline uint32_t sa_read_reg (volatile uint32_t addr) {
  volatile uint32_t * ptr = (volatile uint32_t *)addr;
  return ptr[0];
}

void sa_write_ctrl (uint32_t val) {
  sa_write_reg(SA_BASE_ADDR + SA_CTRL_REG_OFF, val);
}

static inline uint32_t sa_read_ctrl () {
  uint32_t val = sa_read_reg(SA_BASE_ADDR + SA_CTRL_REG_OFF);
  return val;
}

void sa_config (void *cmdq_ofsfet) {
  sa_write_reg(SA_BASE_ADDR + SA_CMDQ_OFF_REG_OFF, (uint32_t)cmdq_ofsfet);
}

#define AddrBits 48UL
#define AddrMask (((uint64_t)1 << AddrBits) - 1UL)

uint64_t *sa_addq (
  uint64_t *qptr, const void *a_ptr, const void *b_ptr,
  uint32_t a_size, uint32_t b_size, uint32_t last, uint32_t id) {

  printf("sa_addq: a_ptr = %p, b_ptr = %p, a_size=%d, b_size=%d, last=%d\n", a_ptr, b_ptr, a_size, b_size, last);

  qptr[1] = ((uint64_t)a_ptr & AddrMask) | ((uint64_t)a_size << AddrBits);
  qptr[2] = ((uint64_t)b_ptr & AddrMask) | ((uint64_t)b_size << AddrBits);

  qptr[0] = ((uint64_t)(&qptr[3]) & AddrMask) | ((uint64_t)id << AddrBits) | ((uint64_t)last << 63);

  return &qptr[3];
}

void sa_discover (sa_prop_t * prop) {
    uint32_t sa_base_addr = SA_BASE_ADDR;
    uint32_t reg = sa_read_reg(sa_base_addr + SA_PROP1_REG_OFF);

    prop->height = reg & 0xffff;
    prop->width = (reg >> 16) & 0xffff;

    reg = sa_read_reg(sa_base_addr + SA_PROP2_REG_OFF);

    prop->mac_type = reg & 0x1;
    prop->batch_id_width = (reg >> 8) & 0xff;
    prop->num_events = (reg >> 16 ) & 0xff;
    prop->version = sa_read_reg(sa_base_addr + SA_VERSION_REG_OFF);

    reg = sa_read_reg(sa_base_addr + SA_PROP3_REG_OFF);

    prop->out_width = (reg >> 16) & 0xffff;
    prop->in_width = (reg) & 0xffff;

    prop->base_addr = sa_base_addr;
}

void sa_read_events (sa_prop_t * prop) {
  printf("sa_read_events:\n");
  for (uint32_t i = 0; i < prop->num_events; i++) {
    uint32_t reg = sa_read_reg(prop->base_addr + SA_EVENTS_REG_OFF + (i * 8));
    printf("event %d = 0x%x\n", i, reg);
  }
}

static inline void _wait (uint32_t cnt) {
  while (cnt-- != 0) {}
}

static volatile uint32_t fifo_lock = 0;
static volatile uint32_t data_lock = 0;

void sa_memread (sa_prop_t *sa_prop, void *_dst, uint32_t *data_cnt) {
    uint32_t fifo_data = 0, fifo_meta = 0;
    uint8_t *dst = (uint8_t *)_dst;
    uint32_t outw_bytes = sa_prop->out_width/8;
    uint32_t batch_offset_bytes = sa_prop->width * sa_prop->height * outw_bytes;

    const uint32_t magic_mask = (1 << 31);

    printf("sa_memread: data_cnt = %d\n", *data_cnt);

    _wait(256);

    sa_read_events(sa_prop);

    while (1) {
        if (0 == *data_cnt) {
          break;
        }

        snrt_mutex_lock(&fifo_lock);
        fifo_meta = sa_read_reg(sa_prop->base_addr + SA_FIFO_META_REG_OFF);

        if (!(fifo_meta & magic_mask)) {
            snrt_mutex_release(&fifo_lock);
            continue;
        }
        fifo_data = sa_read_reg(sa_prop->base_addr + SA_FIFO_DATA_REG_OFF);
        snrt_mutex_release(&fifo_lock);

        //printf("sa_memread: got data : %x\n", fifo_data);

        uint32_t batch_id = (fifo_meta >> 16) & 0xff;
        uint32_t offset = (fifo_meta & 0xffff) * outw_bytes;

        memcpy(dst + offset + batch_offset_bytes * batch_id, &fifo_data, outw_bytes);

        snrt_mutex_lock(&data_lock);
        (*data_cnt)--;
        snrt_mutex_release(&data_lock);
    }
}