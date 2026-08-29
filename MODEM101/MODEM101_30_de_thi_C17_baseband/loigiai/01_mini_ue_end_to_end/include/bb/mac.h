#ifndef BB_MAC_H
#define BB_MAC_H

#include <stddef.h>
#include <stdint.h>

#define BB_MAC_HEADER_SIZE 8U
#define BB_MAC_SUBHEADER_SIZE 4U
#define BB_MAC_MAX_SDUS 8U

enum bb_mac_lcid {
    BB_MAC_LCID_CONTROL = 1,
    BB_MAC_LCID_DATA = 2
};

struct bb_mac_sdu {
    uint8_t lcid;
    const uint8_t *data;
    uint16_t length;
};

int bb_mac_multiplex(const struct bb_mac_sdu *sdus, size_t count,
                     uint8_t *output, size_t capacity, size_t *length);
int bb_mac_demultiplex(const uint8_t *input, size_t length,
                       struct bb_mac_sdu *sdus, size_t capacity,
                       size_t *count);

#endif
