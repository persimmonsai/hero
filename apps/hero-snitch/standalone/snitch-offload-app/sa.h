#ifndef _SA_H_
#define _SA_H_

typedef struct {
    uint32_t base_addr;
    uint32_t width;
    uint32_t height;
    uint32_t mac_type;
    uint32_t version;
} sa_prop_t;

void exec_sa (sa_prop_t * prop, void * a, void * b, void * c);
void discover_sa (sa_prop_t * prop);

#endif /*_SA_H_*/