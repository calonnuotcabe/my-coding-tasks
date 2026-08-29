# Báo cáo build và memory region

Số liệu được đo lại ngày 2026-08-28 bằng
`arm-none-eabi-gcc 10.3.1 20210621`, target Cortex-R5, ARM state, soft-float,
Release, freestanding và `-nostdlib`.

```text
$ arm-none-eabi-size build-audit-arm/bb_fw_arm.elf
   text   data      bss      dec      hex
  53023      0  1097744  1150767   118f2f  bb_fw_arm.elf
```

Chi tiết section hiện hành:

| Section/region | Dùng | Capacity | Quyền |
|---|---:|---:|---|
| `.vectors` / ROM | 32 B | 64 KiB | RX |
| `.text` | 52.967 B | 256 KiB TCM_RX | RX |
| `.ARM.exidx` | 8 B | cùng TCM_RX, đặt tường minh cạnh `.text` | R |
| `.data` / TCM_RW | 0 B | 256 KiB | RW |
| `.bss` / SRAM | 1.048.592 B | 2 MiB | RW |
| `.stacks` / SRAM | 49.152 B | cùng SRAM | RW |
| `.dma_nc` / DDR | 0 B static | 64 MiB trừ trang KEY | RW/NC |
| `.ipc_shm` | 0 B static | 4 MiB | RW/NC |
| `.key` | 16 B | 64 KiB tại `0x43ff0000` | accelerator-owned |

Tổng SRAM dùng là 1.097.744 B (`0x10c010`), dưới 2 MiB. Key test nằm ở
region riêng ngoài DDR descriptor hợp lệ; modem context chỉ giữ `key_slot=1`.
Host model cung cấp accelerator NIA2 để kiểm descriptor/IRQ, không đưa key lên
wire hay AP serialization.

`arm-none-eabi-nm -u` không in symbol nào. `readelf -l` cho hai LOAD code là
`R E`, SRAM là `RW`, KEY là `R`, và `GNU_STACK` là `RW`; không có segment W+X.
Linker sinh `bb_fw_arm.map`, đặt `.ARM.extab/.ARM.exidx` trong TCM_RX để PREL31
luôn tới được `.text`, và ép lỗi nếu region vượt capacity. Target
`arm_qemu_smoke` boot ELF trên CPU model Cortex-R5 và xác nhận đã đi qua cấu
hình MPU tới `bb_firmware_main`.

## Kết quả host có thể tái lập

- Release: 10/10 CTest pass.
- ASan + UBSan: 10/10 CTest pass, gồm sweep 2.000 malformed inputs qua protocol,
  IE, IPC, MAC, crypto và DMA parser.
- GCC `-fanalyzer`: build sạch và 10/10 CTest pass.
- E2E: stdout đúng một dòng; JSONL 987 record, mọi record có
  `tick/domain/event`, tick không giảm.
- SHA-256 lượt chuẩn sau audit: stdout
  `3c2b61f5ccf190c92fc696991c103e9d511d178b8d7b35fc41fee4fe3ba96457`,
  IQ16 `40a6ee8646ccaea5a6b0b314b109219e5610c75bdde0d3a84c3b820ee4a235a9`,
  JSONL `b79800572c20c87f625c18ae34005ef4c86b6a386f9277c64fd36b430a8b270a`.
