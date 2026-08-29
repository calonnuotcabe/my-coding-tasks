# Báo cáo chấm độc lập Mã đề 01

Ngày audit: 2026-08-28. Điểm bám trực tiếp rubric của đề và chỉ được chốt sau
khi đọc source, build sạch, chạy test động, sanitizer, static analyzer, cross-link
Cortex-R5 và boot-smoke ELF. Tuyên bố trong `COMPLIANCE.md` không được dùng thay
cho bằng chứng.

## Kết quả cuối: 100/100

Ở đầu lượt audit này, host build và 10/10 test đều pass nhưng lệnh cross-build
chuẩn không tạo được `bb_fw_arm.elf`: orphan section `.ARM.exidx` của helper chia
64-bit trong `libgcc` bị đặt sau vùng KEY và relocation `R_ARM_PREL31` vượt tầm.
Vì build target bắt buộc bị hỏng, bản đó chưa thể đạt điểm tuyệt đối.

`linker.ld` nay đặt tường minh `.ARM.extab/.ARM.exidx` cạnh `.text` trong TCM_RX.
Cross-build sạch, ELF boot tới `bb_firmware_main` trên CPU model Cortex-R5 sau
khi cấu hình MPU, và toàn bộ verification matrix vẫn pass.

### Câu 1 — 30/30

| Hạng mục | Điểm | Bằng chứng |
|---|---:|---|
| Runtime, memory/buffer, virtual-time | 7/7 | SPSC atomic acquire/release, pool handle generation-tagged, scheduler slot 0,5 ms, signed-modular tick, task stack thật trên x86-64/Arm, DMA descriptor 32 byte, ownership và cache boundary; `unit_runtime`. |
| TX waveform deterministic | 7/7 | CRC-24A, QPSK/16-QAM Gray, grid/pilot, radix-2 IFFT fixed-point, CP và waveform deterministic tới TB 512 byte; empty TB bị reject. |
| RX sync/equalize/demap/decode | 10/10 | Dò đủ ba PSS, timing/CFO/AGC, FFT, one-tap equalization, common-phase tracking, soft demap, CRC; có literal PSS fixture độc lập và test CFO/noise/delay/QPSK/16-QAM. |
| Validation/bounded resource/trace | 6/6 | Bounds trước mọi count/length/rate/modulation, ISR chỉ ACK/enqueue, fast path bounded, sanitizer sạch và đủ `SYNC/FFT/CHANNEL_EST/DEMAP/DECODE/TB_CRC`; HARQ id lấy từ waveform. |

### Câu 2 — 30/30

| Hạng mục | Điểm | Bằng chứng |
|---|---:|---|
| L2 window/HARQ/bearer | 10/10 | BMAC multiplex/BSR, tám HARQ context độc lập và interleaved soft buffer, Chase combine không cộng lặp, RLC-UM SN12, PDCP SN18/HFN/COUNT/reorder/anti-replay; wire chỉ mang SN18. |
| RRC/NAS/control/timer | 8/8 | Transition table `{from,event,guard,action,to}`; mỗi PDU chỉ gây một transition; recovery đi đủ `SEARCHING→CAMPED→CONNECTING→CONNECTED`; virtual timer, generation invalidation và modular wrap tests. |
| Concurrency/ownership/restart | 6/6 | Layer truyền owned handle qua task queue, host/Arm chạy stack riêng, IRQ/deferred split, restart dọn queue/handle/context; BBIP có atomic SHM-style ring, doorbell IRQ16 và response giữ `(epoch,seq)`. |
| Security/malformed handling | 6/6 | NIA2/AES-CMAC qua descriptor, physical-aperture resolver, OWN/cache/MMIO/IRQ continuation; integrity được kiểm trước state mutation; parser PDU/IE/IPC/MAC/DMA bounded và malformed sweep dưới sanitizer. |

### Câu 3 — 40/40

| Hạng mục | Điểm | Bằng chứng |
|---|---:|---|
| E2E thật qua IQ và full stack | 16/16 | Control và 4096 byte data đều đi qua BMAC, CRC/modulation/IFFT, channel CFO +1,8 kHz/noise/delay, RF backend, DMA/IRQ, RX PHY, crypto, RLC, PDCP và RRC/NAS; không có đường truyền payload tắt. |
| State/data sync, recovery, containment | 10/10 | Bắt PCI 42, attach/session, erased codeword→NACK/RETX/combine, replay/reorder, link-loss và re-camp/reconnect; IQ truncation, malformed PDU, timer late, queue full và domain restart đều không crash/hang. |
| Payload/counter/determinism | 8/8 | SDU được so byte-for-byte, PASS phụ thuộc toàn bộ counter bắt buộc, hai lượt cho stdout/IQ16/JSONL byte-identical; trace 987 record có tick không giảm và FINAL hợp lệ. |
| Project/build/test/output | 6/6 | Strict ISO C17/freestanding, Release + ASan/UBSan + `-fanalyzer` đều 10/10; Cortex-R5 ELF32 EABI5 soft-float link sạch, MPU không W+X, QEMU boot-smoke pass; stdout đúng một dòng schema đề. |

## Bằng chứng chạy cuối

- Release: 10/10 CTest pass.
- ASan + UBSan: 10/10 CTest pass.
- GCC `-fanalyzer`: build sạch, 10/10 CTest pass.
- Cortex-R5: `bb_fw_arm.elf` được tạo; post-link `arm_elf_audit` xác nhận
  undefined rỗng, ABI đúng, `.ARM.exidx` trong TCM_RX và không W+X;
  `arm_qemu_smoke` pass.
- E2E: `MODEM_RESULT status=PASS state=DATA tx_bytes=4096 rx_bytes=4096 crc_fail=1 recoveries=2`.
- JSONL: parse được toàn bộ, mỗi record có `tick/domain/event`, tick đơn điệu,
  record cuối xác nhận registered/session và delivered 4096.

100/100 là kết quả audit kỹ thuật nội bộ theo đúng rubric đã cung cấp, không
phải điểm do một hệ thống chấm bên ngoài phát hành.
