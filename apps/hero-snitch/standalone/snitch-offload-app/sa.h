#ifndef _SA_H_
#define _SA_H_

typedef struct {
    uint32_t base_addr;
    uint32_t width;
    uint32_t height;
    uint32_t in_width;
    uint32_t out_width;
    uint32_t mac_type;
    uint32_t num_events;
    uint32_t batch_id_width;
    uint32_t version;
} sa_prop_t;

void sa_write_ctrl (uint32_t val);
void sa_memread (sa_prop_t *sa_prop, void *_dst, uint32_t *data_cnt);
void sa_discover (sa_prop_t * prop);
void sa_config (void *cmdq_ofsfet);
void sa_read_events (sa_prop_t * prop);

uint64_t *sa_addq (
  uint64_t *qptr, const void *a_ptr, const void *b_ptr,
  uint32_t a_size, uint32_t b_size, uint32_t last, uint32_t id);

#endif /*_SA_H_*/