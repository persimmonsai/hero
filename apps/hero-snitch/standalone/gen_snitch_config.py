#!/usr/bin/env python3

import argparse
import hjson

if __name__ == '__main__':
    parser = argparse.ArgumentParser("")
    parser.add_argument('--cfg', help='Provide config.hjson to generate proper test data', type=str, required=True)

    args = parser.parse_args()

    hdr_path = 'snitch_config.h'

    with open(args.cfg) as f:
        hjson_dict = hjson.loads(f.read())

    cluster_config = hjson_dict['cluster']

    cluster_base_addr = cluster_config['cluster_base_addr']
    tcdm_size = cluster_config['tcdm']['size'] * 1024

    periph_size = cluster_config['cluster_periph_size'] * 1024
    sa_base_address = cluster_base_addr + tcdm_size + periph_size

    with open(hdr_path, '+w') as f:
        f.write(f'#define SA_BASE_ADDR {hex(sa_base_address)}\n')
        f.write(f'#define TCDM_BASE_ADDR {hex(cluster_base_addr)}\n')
        f.close()