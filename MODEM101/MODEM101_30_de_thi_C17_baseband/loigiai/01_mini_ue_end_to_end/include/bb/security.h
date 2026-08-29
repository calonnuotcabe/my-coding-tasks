#ifndef BB_SECURITY_H
#define BB_SECURITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int bb_security_cmac(uint16_t key_slot, const uint8_t *data, size_t length,
                     uint8_t output[16]);
bool bb_security_verify(uint16_t key_slot, const uint8_t *data, size_t length,
                        const uint8_t tag[8]);
int bb_security_nia2(uint16_t key_slot, uint32_t count, uint8_t bearer,
                     uint8_t direction, const uint8_t *message, size_t length,
                     uint8_t output[16]);

#endif
