.syntax unified
.cpu cortex-r5
.arm

.section .vectors, "ax", %progbits
.global _vectors
_vectors:
    b reset_handler
    b undefined_handler
    b svc_handler
    b prefetch_abort_handler
    b data_abort_handler
    b reserved_handler
    b irq_handler
    b fiq_handler

.section .text.startup, "ax", %progbits
.global reset_handler
.type reset_handler, %function
reset_handler:
    cpsid if

    ldr r0, =_vectors
    mcr p15, 0, r0, c12, c0, 0

    cps #0x1b
    ldr sp, =__und_stack_top
    cps #0x17
    ldr sp, =__abt_stack_top

    cps #0x12
    ldr sp, =__irq_stack_top
    cps #0x11
    ldr sp, =__fiq_stack_top
    cps #0x13
    ldr sp, =__svc_stack_top
    cps #0x1f
    ldr sp, =__sys_stack_top
    cps #0x13

    ldr r0, =__data_load
    ldr r1, =__data_start
    ldr r2, =__data_end
1:
    cmp r1, r2
    ldrblo r3, [r0], #1
    strblo r3, [r1], #1
    blo 1b

    ldr r1, =__bss_start
    ldr r2, =__bss_end
    mov r3, #0
2:
    cmp r1, r2
    strblo r3, [r1], #1
    blo 2b

    bl bb_arch_configure_mpu
    bl bb_firmware_main
3:
    b 3b
.size reset_handler, . - reset_handler

undefined_handler:      b undefined_handler
svc_handler:            b svc_handler
prefetch_abort_handler: b prefetch_abort_handler
data_abort_handler:     b data_abort_handler
reserved_handler:       b reserved_handler
irq_handler:
    sub lr, lr, #4
    stmdb sp!, {r0-r3, r12, lr}
    mrs r0, spsr
    /* Save eight additional bytes so SP remains AAPCS 8-byte aligned at the
       call into C.  r1 is scratch here and restored from the first frame. */
    stmdb sp!, {r0, r1}
    bl bb_arch_irq_dispatch
    ldmia sp!, {r0, r1}
    msr spsr_cxsf, r0
    ldmia sp!, {r0-r3, r12, pc}^
fiq_handler:            b fiq_handler

.global bb_arch_enable_irqs
.type bb_arch_enable_irqs, %function
bb_arch_enable_irqs:
    cpsie i
    bx lr
.size bb_arch_enable_irqs, . - bb_arch_enable_irqs

.global bb_arch_disable_irqs
.type bb_arch_disable_irqs, %function
bb_arch_disable_irqs:
    cpsid i
    bx lr
.size bb_arch_disable_irqs, . - bb_arch_disable_irqs

/* r0=entry, r1=modem, r2=event, r3=stack_top.  The task entry follows the
   normal two-argument AAPCS contract while the dispatcher retains its stack. */
.global bb_arch_run_task
.type bb_arch_run_task, %function
bb_arch_run_task:
    push {r4, lr}
    mov r4, sp
    bic sp, r3, #7
    mov r12, r0
    mov r0, r1
    mov r1, r2
    blx r12
    mov sp, r4
    pop {r4, pc}
.size bb_arch_run_task, . - bb_arch_run_task

/* Cortex-R5 unified Armv7-R PMSA regions.  XN is set for every writable
   mapping, so SRAM/DDR/SHM/peripherals are never executable.  Region 7
   overlays the final DDR key page as privileged read-only. */
.macro bb_mpu_region number, base, size, data_attr
    ldr r0, =\number
    mcr p15, 0, r0, c6, c2, 0
    ldr r0, =\base
    mcr p15, 0, r0, c6, c1, 0
    ldr r0, =\size
    mcr p15, 0, r0, c6, c1, 2
    ldr r0, =\data_attr
    mcr p15, 0, r0, c6, c1, 4
.endm

.global bb_arch_configure_mpu
.type bb_arch_configure_mpu, %function
bb_arch_configure_mpu:
    dsb sy
    mrc p15, 0, r0, c1, c0, 0
    bic r0, r0, #1
    bic r0, r0, #0x20000
    mcr p15, 0, r0, c1, c0, 0
    isb sy

    /* size encodes (log2(bytes)-1)<<1 | enable.  AP=0b001 gives
       privileged RW; AP=0b101 gives privileged RO. */
    bb_mpu_region 0, 0x00000000, 0x1f, 0x050b
    bb_mpu_region 1, 0x00100000, 0x23, 0x050b
    bb_mpu_region 2, 0x00140000, 0x23, 0x110b
    bb_mpu_region 3, 0x10000000, 0x29, 0x110b
    bb_mpu_region 4, 0x40000000, 0x33, 0x110b
    bb_mpu_region 5, 0x50000000, 0x2b, 0x110c
    bb_mpu_region 6, 0x60000000, 0x27, 0x1105
    bb_mpu_region 7, 0x43ff0000, 0x1f, 0x150b

    dsb sy
    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #1
    mcr p15, 0, r0, c1, c0, 0
    dsb sy
    isb sy
    bx lr
.size bb_arch_configure_mpu, . - bb_arch_configure_mpu

.global bb_arch_cache_clean_range
.type bb_arch_cache_clean_range, %function
bb_arch_cache_clean_range:
    cmp r1, #0
    bxeq lr
    add r2, r0, r1
    bic r0, r0, #63
4:
    mcr p15, 0, r0, c7, c10, 1
    add r0, r0, #64
    cmp r0, r2
    blo 4b
    dsb sy
    bx lr
.size bb_arch_cache_clean_range, . - bb_arch_cache_clean_range

.global bb_arch_cache_invalidate_range
.type bb_arch_cache_invalidate_range, %function
bb_arch_cache_invalidate_range:
    cmp r1, #0
    bxeq lr
    add r2, r0, r1
    bic r0, r0, #63
5:
    mcr p15, 0, r0, c7, c6, 1
    add r0, r0, #64
    cmp r0, r2
    blo 5b
    dsb sy
    bx lr
.size bb_arch_cache_invalidate_range, . - bb_arch_cache_invalidate_range

.section .note.GNU-stack, "", %progbits
