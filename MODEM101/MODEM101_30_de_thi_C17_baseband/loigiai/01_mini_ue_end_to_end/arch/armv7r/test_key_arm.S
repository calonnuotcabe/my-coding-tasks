.syntax unified
.cpu cortex-r5
.section .key, "a", %progbits
.balign 16
.global bb_accelerator_test_key
.type bb_accelerator_test_key, %object
bb_accelerator_test_key:
    .byte 0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6
    .byte 0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c
.size bb_accelerator_test_key, . - bb_accelerator_test_key
.section .note.GNU-stack, "", %progbits
