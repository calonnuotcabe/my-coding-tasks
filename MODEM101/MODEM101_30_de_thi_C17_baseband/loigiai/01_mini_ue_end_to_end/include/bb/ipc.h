#ifndef BB_IPC_H
#define BB_IPC_H

#include <stddef.h>
#include <stdint.h>

#define BB_IPC_HEADER_SIZE 28U
#define BB_IPC_MAX_FRAME_SIZE 256U
#define BB_IPC_FLAG_RESPONSE 0x80000000U

struct bb_modem;

struct bb_ipc_frame {
    uint16_t version;
    uint16_t type;
    uint32_t sequence;
    uint32_t epoch;
    uint32_t flags;
    const uint8_t *tlvs;
    uint32_t tlv_length;
};

uint32_t bb_crc32c(const uint8_t *data, size_t length);
int bb_ipc_encode(const struct bb_ipc_frame *frame, uint8_t *output,
                  size_t capacity, size_t *length);
int bb_ipc_decode(const uint8_t *input, size_t length,
                  struct bb_ipc_frame *frame);
int bb_modem_ipc_submit(struct bb_modem *modem, const uint8_t *request,
                        size_t length);
int bb_modem_ipc_receive(struct bb_modem *modem, uint8_t *response,
                         size_t capacity, size_t *length);

#endif
