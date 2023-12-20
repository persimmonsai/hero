#include "snrt.h"

#include "snrt.h"

#include "sa.h"

#define SA0_BASE_ADDR       (0x10040000)
#define SA_CTRL_REG_OFF     (0 * 8)
#define SA_ROW_OFF_REG_OFF  (1 * 8)
#define SA_COL_OFF_REG_OFF  (2 * 8)
#define SA_SRAM_OFF_REG_OFF (3 * 8)
#define SA_STATUS_REG_OFF   (4 * 8)
#define SA_VERSION_REG_OFF  (5 * 8)
#define SA_PROP1_REG_OFF    (6 * 8)
#define SA_PROP2_REG_OFF    (7 * 8)
#define SA_PROP3_REG_OFF    (8 * 8)

#define SA_MAC_DONE_BP (0)
#define SA_RI_DONE_BP (1)
#define SA_RO_DONE_BP (2)

static void sa_wait_cycles (uint32_t cycles) {
  while (cycles-- != 0) {}
}

static void sa_write_reg (volatile uint32_t addr, uint32_t data) {
  volatile uint32_t * ptr = (volatile uint32_t *)addr;
  ptr[0] = data;
}

static uint32_t sa_read_reg (volatile uint32_t addr) {
  volatile uint32_t * ptr = (volatile uint32_t *)addr;
  return ptr[0];
}


static void sa_write_ctrl (uint32_t base_addr, uint32_t val) {
  sa_write_reg(base_addr + SA_CTRL_REG_OFF, val);
}

static uint32_t sa_read_ctrl (uint32_t base_addr) {
  uint32_t val = sa_read_reg(base_addr + SA_CTRL_REG_OFF);
  return val;
}

static uint32_t sa_read_status (uint32_t base_addr) {
  uint32_t val = sa_read_reg(base_addr + SA_STATUS_REG_OFF);
  return val;
}


static void sa_wait_reg (uint32_t addr, uint32_t bm) {
  uint32_t val = sa_read_reg(addr);
  while ((val & bm) != bm) {
    val = sa_read_reg(addr);
  }
}

static void sa_wait_status (uint32_t base_asddr, uint32_t bm) {
  sa_wait_reg(base_asddr + SA_STATUS_REG_OFF, bm);
}

static void config_sa (uint32_t base_addr, uint32_t row_offset, uint32_t col_offset, uint32_t sram_offset) {
  sa_write_reg(base_addr + SA_ROW_OFF_REG_OFF, row_offset);
  sa_write_reg(base_addr + SA_COL_OFF_REG_OFF, col_offset);
  sa_write_reg(base_addr + SA_SRAM_OFF_REG_OFF, sram_offset);
}


void exec_sa (sa_prop_t * prop, void * a, void * b, void * c) {
    uint32_t sa_base_addr = prop->base_addr;

    //Reset
    sa_write_ctrl(sa_base_addr, 0);

    config_sa(sa_base_addr, (uint32_t)a, (uint32_t)b, (uint32_t)c);

    sa_write_ctrl(sa_base_addr, 1);

    sa_wait_status(sa_base_addr, 1 << SA_RI_DONE_BP);
    sa_wait_status(sa_base_addr, 1 << SA_MAC_DONE_BP);

    sa_wait_cycles(4 * (prop->width + prop->height));

    //Read out
    sa_write_ctrl(sa_base_addr, (1 << 31) | 1);
    sa_wait_status(sa_base_addr, 1 << SA_RO_DONE_BP);
}

void discover_sa (sa_prop_t * prop) {
    uint32_t sa_base_addr = SA0_BASE_ADDR;
    uint32_t reg = sa_read_reg(sa_base_addr + SA_PROP1_REG_OFF);

    prop->height = reg & 0xffff;
    prop->width = (reg >> 16) & 0xffff;

    reg = sa_read_reg(sa_base_addr + SA_PROP2_REG_OFF);

    prop->mac_type = reg & 0x1;
    prop->version = sa_read_reg(sa_base_addr + SA_VERSION_REG_OFF);

    reg = sa_read_reg(sa_base_addr + SA_PROP3_REG_OFF);

    prop->out_width = (reg >> 16) & 0xffff;
    prop->in_width = (reg) & 0xffff;

    prop->base_addr = sa_base_addr;
}