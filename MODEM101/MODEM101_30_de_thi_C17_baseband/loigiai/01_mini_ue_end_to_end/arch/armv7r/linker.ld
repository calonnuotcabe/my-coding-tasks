ENTRY(_vectors)

MEMORY
{
    ROM     (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    TCM_RX  (rx)  : ORIGIN = 0x00100000, LENGTH = 256K
    TCM_RW  (rw)  : ORIGIN = 0x00140000, LENGTH = 256K
    SRAM    (rw)  : ORIGIN = 0x10000000, LENGTH = 2M
    /* 64 MiB DDR aperture, less the final accelerator-owned 64 KiB page. */
    DDR     (rw)  : ORIGIN = 0x40000000, LENGTH = 0x03FF0000
    IPC_SHM (rw)  : ORIGIN = 0x50000000, LENGTH = 4M
    KEY     (r)   : ORIGIN = 0x43FF0000, LENGTH = 64K
}

SECTIONS
{
    .vectors : ALIGN(32) { KEEP(*(.vectors)) } > ROM
    .text : ALIGN(16) { *(.text.startup) *(.text*) *(.rodata*) } > TCM_RX

    /* libgcc's 64-bit division helpers may carry EHABI unwind indexes even in
       a freestanding C build.  Keep their PREL31 targets close to .text;
       leaving these orphan sections to the linker can place them after the
       accelerator-owned KEY aperture at 0x43ff0000 and make the Arm link fail
       with a truncated R_ARM_PREL31 relocation. */
    .ARM.extab : ALIGN(4) { *(.ARM.extab* .gnu.linkonce.armextab.*) } > TCM_RX
    .ARM.exidx : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > TCM_RX

    .data : ALIGN(8)
    {
        __data_start = .;
        *(.data*)
        __data_end = .;
    } > TCM_RW AT > ROM
    __data_load = LOADADDR(.data);

    .bss (NOLOAD) : ALIGN(8)
    {
        __bss_start = .;
        *(.bss*) *(COMMON)
        __bss_end = .;
    } > SRAM

    .dma_nc (NOLOAD) : ALIGN(64) { *(.dma_nc*) } > DDR
    .ipc_shm (NOLOAD) : ALIGN(64) { *(.ipc_shm*) } > IPC_SHM
    .key : ALIGN(64) { KEEP(*(.key*)) } > KEY

    .stacks (NOLOAD) : ALIGN(16)
    {
        . += 4096; __und_stack_top = .;
        . += 4096; __abt_stack_top = .;
        . += 4096; __irq_stack_top = .;
        . += 4096; __fiq_stack_top = .;
        . += 16384; __svc_stack_top = .;
        . += 16384; __sys_stack_top = .;
    } > SRAM

    /DISCARD/ : { *(.comment) *(.note*) }

    __rom_used = LOADADDR(.data) + SIZEOF(.data) - ORIGIN(ROM);
    __tcm_rx_used = SIZEOF(.text) + SIZEOF(.ARM.extab) + SIZEOF(.ARM.exidx);
    __tcm_rw_used = SIZEOF(.data);
    __sram_used = __sys_stack_top - ORIGIN(SRAM);
    __dma_nc_used = SIZEOF(.dma_nc);
    __ipc_shm_used = SIZEOF(.ipc_shm);
    __key_used = SIZEOF(.key);
}
